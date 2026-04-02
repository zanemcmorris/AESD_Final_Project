/*
    @file perfInterface.c
    @Author Zane Mcmorris & Likhita Jonnakuti
    @date April 1, 2026
*/

#include "perfInterface.h"

/**
 * @brief returns some sort of status/state of perf business. 
 * Probably something along the lines of (ready, fault, running (if running, print some stats?))
 */
int getPerfStatus(){
    return 0;
}

/**
 * @brief Set up perf system. Whatever would need to be run once before we call into it repeatedly
 */
status_t initPerfSystem(){

    return STATUS_OK;
}

/**
 * @brief Starts listed operation, taking in info about the job
 */
status_t perfStartSeqWrite(perfJobInfo_t* info){

    return STATUS_OK;
}

status_t perfStartSeqRead(perfJobInfo_t* info){

    return STATUS_OK;
}
status_t perfStartRandWrite(perfJobInfo_t *info){

    return STATUS_OK;
}

status_t perfStartRandRead(perfJobInfo_t* info){
    return STATUS_OK;
}

/**
 * @brief Stops running job. Returns STATUS_NO_JOB if nothing is running
 */
status_t perfStopJob(){

    return STATUS_OK;
}