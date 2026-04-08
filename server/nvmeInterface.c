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
 * @brief Print discovery log
 */
static void print_discover_log(struct nvmf_discovery_log *log)
{
    uint64_t numrec = le64toh(log->numrec);
    int i;

    printf(".\n");
    printf("|-- genctr:%llx\n", (unsigned long long)log->genctr);
    printf("|-- numrec:%llu\n", (unsigned long long)numrec);
    printf("`-- recfmt:%x\n", log->recfmt);

    for (i = 0; i < (int)numrec; i++) {
        struct nvmf_disc_log_entry *e = &log->entries[i];

        printf("  %c-- Entry:%d\n", (i < (int)numrec - 1) ? '|' : '`', i);
        printf("  %c   |-- trtype:%x\n",  (i < (int)numrec - 1) ? '|' : ' ', e->trtype);
        printf("  %c   |-- adrfam:%x\n",  (i < (int)numrec - 1) ? '|' : ' ', e->adrfam);
        printf("  %c   |-- subtype:%x\n", (i < (int)numrec - 1) ? '|' : ' ', e->subtype);
        printf("  %c   |-- treq:%x\n",    (i < (int)numrec - 1) ? '|' : ' ', e->treq);
        printf("  %c   |-- portid:%x\n",  (i < (int)numrec - 1) ? '|' : ' ', e->portid);
        printf("  %c   |-- cntlid:%x\n",  (i < (int)numrec - 1) ? '|' : ' ', e->cntlid);
        printf("  %c   |-- asqsz:%x\n",   (i < (int)numrec - 1) ? '|' : ' ', e->asqsz);
        printf("  %c   |-- trsvcid:%s\n", (i < (int)numrec - 1) ? '|' : ' ', e->trsvcid);
        printf("  %c   |-- subnqn:%s\n",  (i < (int)numrec - 1) ? '|' : ' ', e->subnqn);
        printf("  %c   `-- traddr:%s\n",  (i < (int)numrec - 1) ? '|' : ' ', e->traddr);
    }
}

/**
 * @brief Gets information about the interface, drive(s), and namespaces
 */
nvmeStatus_t nvmeGetStatus(){
    struct nvmf_discovery_log *log = NULL;
	nvme_root_t r;
	nvme_host_t h;
	nvme_ctrl_t c;
	int ret;
	struct nvme_fabrics_config cfg;

	nvmf_default_config(&cfg);

	r = nvme_scan(NULL);
	h = nvme_default_host(r);
	if (!h) {
		fprintf(stderr, "Failed to allocated memory\n");
		return ENOMEM;
	}
	c = nvme_create_ctrl(r, NVME_DISC_SUBSYS_NAME, "loop",
			     NULL, NULL, NULL, NULL);
	if (!c) {
		fprintf(stderr, "Failed to allocate memory\n");
		return ENOMEM;
	}
	ret = nvmf_add_ctrl(h, c, &cfg);
	if (ret < 0) {
		fprintf(stderr, "no controller found\n");
		return errno;
	}

	ret = nvmf_get_discovery_log(c, &log, 4);
	nvme_disconnect_ctrl(c);
	nvme_free_ctrl(c);

	if (ret)
		fprintf(stderr, "nvmf-discover-log:%x\n", ret);
	else
		print_discover_log(log);

	nvme_free_tree(r);
	free(log);

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