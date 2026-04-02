/*
    @file nvmeInterface.c
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#include "nvmeInterface.h"

/**
 * @brief Gets information about the interface, drive(s), and namespaces
 */
nvmeStatus_t nvmeGetStatus(){
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