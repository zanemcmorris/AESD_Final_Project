/*
    @file nvmeInterface.c
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#include "nvmeInterface.h"
#include "stdlib.h"
#include <libnvme.h>
#include <nvme/ioctl.h>
#include <nvme/tree.h>
#include <nvme/types.h>
#include <parted/parted.h>
#include <math.h>

#include <endian.h>

/**
 * @brief Gets information about the interface, drive(s), and namespaces
 */
static void print_namespace_info(nvme_ns_t n)
{
    // TODO: add verbose option, default to just print ns name
    const char *ns_name = nvme_ns_get_name(n);
    int nsid = nvme_ns_get_nsid(n);
    int lba_size = nvme_ns_get_lba_size(n);
    uint64_t lba_count = nvme_ns_get_lba_count(n);
    unsigned long long size_bytes = (unsigned long long)lba_size *
                                    (unsigned long long)lba_count;

    printf("    namespace: %s\n", ns_name ? ns_name : "(unknown)");
    printf("      nsid: %d\n", nsid);
    printf("      lba size: %d\n", lba_size);
    printf("      lba count: %llu\n", (unsigned long long)lba_count);
    printf("      approx size bytes: %llu\n", size_bytes);
}

/**
 * @brief Get status of nvme device. Prints to stdout
 */
nvmeStatus_t nvmeGetStatus(void)
{
    nvme_root_t r;
    nvme_host_t h;
    nvme_subsystem_t s;
    nvme_ctrl_t c;
    nvme_ns_t n;
    bool found = false;

    r = nvme_scan(NULL);
    if (!r) {
        perror("nvme_scan");
        return NVME_STATUS_ERROR;
    }

    nvme_for_each_host(r, h) {
        nvme_for_each_subsystem(h, s) {
            nvme_subsystem_for_each_ctrl(s, c) {
                const char *ctrl_name = nvme_ctrl_get_name(c);
                const char *model = nvme_ctrl_get_model(c);
                const char *serial = nvme_ctrl_get_serial(c);
                const char *fw = nvme_ctrl_get_firmware(c);
                const char *transport = nvme_ctrl_get_transport(c);
                const char *addr = nvme_ctrl_get_address(c);
                const char *subsys_name = nvme_subsystem_get_name(s);
                const char *subnqn = nvme_subsystem_get_nqn(s);

                found = true;

                printf("controller: %s\n", ctrl_name ? ctrl_name : "(unknown)");
                printf("  subsystem: %s\n", subsys_name ? subsys_name : "(unknown)");
                printf("  subnqn: %s\n", subnqn ? subnqn : "(unknown)");
                printf("  model: %s\n", model ? model : "(unknown)");
                printf("  serial: %s\n", serial ? serial : "(unknown)");
                printf("  firmware: %s\n", fw ? fw : "(unknown)");
                printf("  transport: %s\n", transport ? transport : "(unknown)");

                if (addr && *addr)
                    printf("  address: %s\n", addr);

                n = nvme_subsystem_first_ns(s);
                if (!n){
                    printf("No namespace found in subsystem\n");
                }

                nvme_ctrl_for_each_ns(c, n){
                    print_namespace_info(n);
                }
                
                nvme_subsystem_for_each_ns(s, n) {
                    print_namespace_info(n);
                }

                printf("\n");
            }
        }
    }

    nvme_free_tree(r);

    if (!found) {
        fprintf(stderr, "No local NVMe controllers found\n");
        return NVME_STATUS_NOT_FOUND;
    }

    return NVME_STATUS_OK;
}

/**
 * @brief List partitions in nvme drive
 */
nvmeStatus_t nvmeListPartitions(){

    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    
    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    printf("Parted device name: %s\n", dev->model);

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    PedPartition *part = NULL;
    for(int i = 1; i < 16; i++){
        part = ped_disk_get_partition(disk, i);
        if(part == NULL){
            continue;
        }

        const char* partName = ped_partition_get_name(part);
        printf("part %d name: %s\n", i, partName);
    }
    

    ped_disk_destroy(disk);
    ped_device_close(dev);

    return NVME_STATUS_OK;
}

/**
 * @brief Create a new NVME namespace
 * @param size number of sectors in partition
 * @param newPartNumber pointer to return newpartitionnumber
 * @return nvmeStatus_t
 */
nvmeStatus_t nvmeCreatePartition(uint32_t size, uint8_t* newPartNumber){

    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedConstraint *constraint;
    PedFileSystemType *fsType;
    PedAlignment *align;
    
    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    printf("Parted device name: %s\n", dev->model);

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    fsType = ped_file_system_type_get("ext4"); // TODO: replace literal with better mechanism for fstype
    if(fsType == NULL){
        perror("ped_file_system_type_get");
        return NVME_STATUS_ERROR;
    }
    align = ped_disk_get_partition_alignment(disk);
    printf("Found %lld alignment\n", align->grain_size);

    uint32_t sectorSize = dev->sector_size;
    printf("sector size: %d\n", sectorSize);

    // Need to find a sector range that can fit the requested new partition
    // TODO: Update with a gap-filling method instead of appending.
    PedSector sectorStart = 0;

    int lastPartNum = ped_disk_get_last_partition_num(disk);
    if(lastPartNum == -1){
        perror("ped_disk_get_last_partition_num");
        return NVME_STATUS_ERROR;
    }

    sectorStart = ped_disk_get_partition(disk, lastPartNum)->geom.end;

    PedSector maxSector = dev->length-1;
    if(sectorStart + size > maxSector){
        return NVME_STATUS_INPUT;
    }
     
    PedPartition *newPart = ped_partition_new(disk, PED_PARTITION_NORMAL, fsType, sectorStart, sectorStart + size - 1);
    if(newPart == NULL){
        perror("ped_partition_new");
        goto createParitionError;
    } else {
        printf("created new part!\n");
    }

    constraint = ped_device_get_optimal_aligned_constraint(dev);
    

    rc = ped_disk_add_partition(disk, newPart, constraint);
    if(rc == 0){
        perror("ped_disk_add_partition");
        goto createParitionError;
    }

    rc = ped_disk_commit_to_dev(disk);
    if(rc == 0){
        perror("ped_disk_commit_to_dev");
        goto createParitionError;
    }

    rc = ped_disk_commit_to_os(disk);
    if(rc == 0){
        perror("ped_disk_commit_to_os");
        goto createParitionError;
    }

    *newPartNumber = newPart->num;

    return NVME_STATUS_OK;

    createParitionError:
    ped_disk_destroy(disk);
    ped_device_close(dev);
    ped_constraint_destroy(constraint);
    return NVME_STATUS_ERROR;
}

/**
 * @brief Delete an existing NVME namespace. 
 * @return Returns NVME_STATUS_ERROR if namespace doesn't exist or cannot be deleted
 *         Otherwise returns NVME_STATUS_OK if operation could be completed
 */
nvmeStatus_t nvmeDeletePartition(int partNumber){

    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * partToDelete = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    partToDelete = ped_disk_get_partition(disk, partNumber);
    if(partToDelete == NULL){
        perror("ped_disk_get_partition");
        return NVME_STATUS_INPUT;
    }

    rc = ped_disk_delete_partition(disk, partToDelete);
    if(rc == 0){
        perror("ped_disk_delete_partition");
    }

    rc = ped_disk_commit(disk);
    if(rc == 0){
        perror("ped_disk_commit_to_dev");
        return NVME_STATUS_ERROR;
    }

    return NVME_STATUS_OK;
}

/**
 * @brief Lists currently available namespaces and information about them
 */
nvmeStatus_t nvmeListNamespace(){
    nvme_root_t r;
    nvme_host_t h;
    nvme_subsystem_t s;
    nvme_ctrl_t c;
    nvme_ns_t n;
    bool found = false;

    r = nvme_scan(NULL);
    if (!r) {
        perror("nvme_scan");
        return NVME_STATUS_ERROR;
    }

    nvme_for_each_host(r, h) {
        nvme_for_each_subsystem(h, s) {
            nvme_subsystem_for_each_ctrl(s, c) {
                nvme_ctrl_for_each_ns(c, n){
                    print_namespace_info(n);
                }
                printf("\n");
            }
        }
    }

    nvme_free_tree(r);

    if (!found) {
        fprintf(stderr, "No local NVMe controllers found\n");
        return NVME_STATUS_NOT_FOUND;
    }

    return NVME_STATUS_OK;
}

/**
 * @brief Usercommand to write to a sector(s) with data contained in buffer
 * @param partNumber Partition number to write to. Must exist already
 * @param sectorNumber Sector number within the partition to write to. Must be within its range.
 * @param buffer Data to write to partition[sector]
 * @param length Length of the data to write to the sector. Must be a multiple of sector size.
 * @return nvmeStatus_t indicating success or failure of operation.
 */
nvmeStatus_t nvmeWritePartitionSector(uint32_t partNumber, uint32_t sectorNumber, char* buffer, size_t length){
    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * partToWrite = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    partToWrite = ped_disk_get_partition(disk, partNumber);
    if(partToWrite == NULL){
        printf("Specified partition does not exist.\n");
        ped_disk_destroy(disk);
        ped_device_close(dev);
        return NVME_STATUS_INPUT;
    }    
    
    printf("writing geom %lld %lld %lld\n", partToWrite->geom.start, partToWrite->geom.length, partToWrite->geom.end);
    int sectorCount = 1;

    rc = ped_geometry_write(&(partToWrite->geom), buffer, 0, sectorCount);
    if(rc == 0)
    {
        perror("ped_geometry_write");
        return NVME_STATUS_ERROR;
    }

    rc = ped_geometry_sync(&(partToWrite->geom));
    if(rc == 0)
    {
        perror("ped_geometry_sync");
        return NVME_STATUS_ERROR;
    }


    // ped_partition_destroy(partToWrite);
    // ped_disk_destroy(disk);
    // ped_device_close(dev);
    return NVME_STATUS_OK;
}

/**
 * @brief Usercommand to read from a sector(s) and return data in buffer
 * @param partNumber Partition number to read from. Must exist already
 * @param sectorNumber Sector number within the partition to read from. Must be within its range.
 * @param buffer Return location for data read from part.
 * @param length Length of the data to read from the sector in bytes. Must be a multiple of sector size.
 * @return nvmeStatus_t indicating success or failure of operation.
 */
nvmeStatus_t nvmeReadPartitionSector(uint32_t partNumber, uint32_t sectorNumber, char* buffer, size_t length){

    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * partToRead = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    partToRead = ped_disk_get_partition(disk, partNumber);
    if(partToRead == NULL){
        printf("Specified partition does not exist.\n");
        ped_disk_destroy(disk);
        ped_device_close(dev);
        return NVME_STATUS_PART_DNE;
    }

    // TODO: Verify length and sectorcount allign. 
    int sectorCount = (length / dev->sector_size);
    printf("nvmeRead: reading %d sectors\n", sectorCount);
    

    rc = ped_geometry_read(&(partToRead->geom), buffer, 0, sectorCount);
    if(rc == 0)
    {
        perror("ped_geometry_write");
        return NVME_STATUS_ERROR;
    }

    // rc = ped_geometry_sync(&(partToRead->geom));
    // if(rc == 0)
    // {
    //     perror("ped_geometry_sync");
    //     return NVME_STATUS_ERROR;
    // }

    // ped_partition_destroy(partToRead);
    // ped_disk_destroy(disk);
    // ped_device_close(dev);
    return NVME_STATUS_OK;
}

nvmeStatus_t nvmeCheckLbaRangeInPart(uint8_t partNumber, lbaRange_t range){
    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * part = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    part = ped_disk_get_partition(disk, partNumber);
    if(part == NULL){
        printf("Specified partition does not exist.\n");
        ped_disk_destroy(disk);
        ped_device_close(dev);
        return NVME_STATUS_PART_DNE;
    }

    // Continue here

    bool inBounds = true;
    if(range.startlba + part->geom.start > part->geom.end){
        inBounds = false;
    }
    if(range.endlba + part->geom.start > part->geom.end){
        inBounds = false;
    }
    if(range.endlba - range.startlba > part->geom.length){
        inBounds = false;
    }

    if(inBounds){
        return NVME_STATUS_OK; // lbarange is entirely within partition
    } else {
        return NVME_STATUS_ERROR; // lbarange is partially or wholly outside of the partition.
    }
}
/**
 * @brief Get the starting LBA of a partition, if it exists. 
 * @param partNumber Partition number of interest
 * @param lba Pointer to return start address
 * @return nvmeStatus_t Status of operation. Returns NVME_STATUS_OK if successful.
 */
nvmeStatus_t nvmeGetStartLbaInPart(uint8_t partNumber, size_t * lba){
    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * part = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    part = ped_disk_get_partition(disk, partNumber);
    if(part == NULL){
        printf("Specified partition does not exist.\n");
        ped_disk_destroy(disk);
        ped_device_close(dev);
        return NVME_STATUS_PART_DNE;
    }

    // Continue here

    *lba = part->geom.start;


    return NVME_STATUS_OK;
}

/**
 * @brief Get the ending LBA of a partition, if it exists. 
 * @param partNumber Partition number of interest
 * @param lba Pointer to return end address
 * @return nvmeStatus_t Status of operation. Returns NVME_STATUS_OK if successful.
 */
nvmeStatus_t nvmeGetEndLbaInPart(uint8_t partNumber, size_t * lba){
    int rc = 0;
    PedDevice * dev = NULL;
    const char *devicePath = "/dev/nvme0n1"; // TODO: Update with known/gathered path.
    PedPartition * part = NULL;

    dev = ped_device_get(devicePath);
    if(dev == NULL){
        perror("ped_device_get failed");
        return NVME_STATUS_NOT_FOUND;
    }

    rc = ped_device_open(dev);
    if(rc == 0){
        perror("ped_device_open");
        return NVME_STATUS_ERROR;
    }

    PedDisk* disk = ped_disk_new(dev);
    if(!disk){
        perror("ped_disk_new");
        return NVME_STATUS_ERROR;
    }

    part = ped_disk_get_partition(disk, partNumber);
    if(part == NULL){
        printf("Specified partition does not exist.\n");
        ped_disk_destroy(disk);
        ped_device_close(dev);
        return NVME_STATUS_PART_DNE;
    }

    // Continue here

    *lba = part->geom.end;


    return NVME_STATUS_OK;
}

