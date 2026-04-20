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
    NVME_STATUS_PART_DNE = -4,
    

}nvmeStatus_t;

nvmeStatus_t nvmeGetStatus();

nvmeStatus_t nvmeListNamespace();
nvmeStatus_t nvmeCreatePartition(uint32_t size, uint8_t * newPart);
nvmeStatus_t nvmeDeletePartition(int partNum);
nvmeStatus_t nvmeListPartitions();

nvmeStatus_t nvmeWritePartitionSector(uint32_t part, uint32_t sectorNumber, char* buffer, size_t length);
nvmeStatus_t nvmeReadPartitionSector(uint32_t part, uint32_t sectorNumber, char* buffer, size_t length);

nvmeStatus_t nvmeCheckLbaRangeInPart(uint8_t part, lbaRange_t range);
#endif