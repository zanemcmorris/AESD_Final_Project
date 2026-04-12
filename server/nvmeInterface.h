/*
    @file nvmeInterface.h
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#ifndef NVME_INTERFACE_H
#define NVME_INTERFACE_H

#include "interfaceCommon.h"
#include "libnvme.h"


typedef enum{
    NVME_STATUS_OK = 0,
    NVME_STATUS_ERROR = -1,
    NVME_STATUS_NOT_FOUND = -2,
    NVME_STATUS_INPUT = -3,
    

}nvmeStatus_t;

nvmeStatus_t nvmeGetStatus();

nvmeStatus_t nvmeListNamespace();
nvmeStatus_t nvmeCreatePartition(uint32_t size);
nvmeStatus_t nvmeDeletePartition();
nvmeStatus_t nvmeListPartitions();

nvmeStatus_t nvmeWritePartitionLba(uint32_t ns, lbaRange_t lbaRange);
nvmeStatus_t nvmeReadPartitionLba(uint32_t ns, lbaRange_t lbaRange);


#endif