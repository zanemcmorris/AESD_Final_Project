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
                    printf("No namespace found\n");
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

    // nvmeCreatePartition(1024);
    // nvmeDeletePartition(7);
    // nvmeListPartitions();
    // nvmeListNamespace();

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
 * @return nvmeStatus_t
 */
nvmeStatus_t nvmeCreatePartition(uint32_t size){

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
     
    PedPartition *newPart = ped_partition_new(disk, PED_PARTITION_NORMAL, fsType, sectorStart, sectorStart + size);
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

    rc = ped_disk_commit_to_dev(disk);
    if(rc == 0){
        perror("ped_disk_commit_to_dev");
        return NVME_STATUS_ERROR;
    }

    rc = ped_disk_commit_to_os(disk);
    if(rc == 0){
        perror("ped_disk_commit_to_os");
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
 * @brief Usercommand to write an LBA range in a namespace with some information
 * TODO: Needs more inputs and implementation.
 */
nvmeStatus_t nvmeWritePartitionLba(uint32_t ns, lbaRange_t lbaRange){
    return NVME_STATUS_OK;
}
nvmeStatus_t nvmeReadPartitionLba(uint32_t ns, lbaRange_t lbaRange){
    return NVME_STATUS_OK;
}

