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
    

}nvmeStatus_t;

nvmeStatus_t nvmeGetStatus();

nvmeStatus_t nvmeCreateNamespace();
nvmeStatus_t nvmeDeleteNamespace();
nvmeStatus_t nvmeListNamespace();

nvmeStatus_t nvmeWriteNamespaceLba(uint32_t ns, lbaRange_t lbaRange);
nvmeStatus_t nvmeReadNamespaceLba(uint32_t ns, lbaRange_t lbaRange);


#endif