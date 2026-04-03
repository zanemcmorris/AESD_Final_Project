/*
    @file commandQueue.c
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#include "commandQueue.h"
#include <string.h>

CommandQueue_t CommandQueue; 

/**
 * @brief Initialize NVME command queue.
 * @return ERROR_NONE on success
 */
int commandQueueInit(){
    if(CommandQueue.isInitComplete){
        return ERROR_RE_INIT;
    }

    pthread_mutex_init(&CommandQueue.mutex, NULL);

    memset(CommandQueue.queue, 0, sizeof(command_t) * MAX_QUEUE_SIZE);

    CommandQueue.isInitComplete = true;
    return ERROR_NONE;
}

/**
 * @brief Kicks off processing of next command in queue. Should be called periodically to 
 */
int processNextCommandInQueue(){

    // Get node at head of fifo
    command_t * nextCommand = NULL; // = getNextNode
    

    if(nextCommand->cmdType == userCommand){
        

    }
    else if (nextCommand->cmdType == perfCommand)
    {

    }
    else if (nextCommand->cmdType == adminCommand){

    } else {
        return ERROR_INPUT; // Unknown command type
    }

    return ERROR_NONE;
}   
int enqueue_command(command_t * cmd){

    return ERROR_NONE;
}

int get_queue_status(){

    return ERROR_NONE;
}