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

    nvmeListNamespace();

    return NVME_STATUS_OK;
}

/**
 * @brief Create a new NVME namespace
 */
nvmeStatus_t nvmeCreateNamespace(){
    return NVME_STATUS_OK;
}

/**
 * @brief Delete an existing NVME namespace. 
 * @return Returns NVME_STATUS_ERROR if namespace doesn't exist or cannot be deleted
 *         Otherwise returns NVME_STATUS_OK if operation could be completed
 */
nvmeStatus_t nvmeDeleteNamespace(){
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
nvmeStatus_t nvmeWriteNamespaceLba(uint32_t ns, lbaRange_t lbaRange){
    return NVME_STATUS_OK;
}
nvmeStatus_t nvmeReadNamespaceLba(uint32_t ns, lbaRange_t lbaRange){
    return NVME_STATUS_OK;
}