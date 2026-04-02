/*
    @file commandQueue.c
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#include "commandQueue.h"
#include <string.h>

CommandQueue_t CommandQueue; 

int commandQueueInit(){
    if(CommandQueue.isInitComplete){
        return ERROR_RE_INIT;
    }

    pthread_mutex_init(&CommandQueue.mutex, NULL);

    memset(CommandQueue.queue, 0, sizeof(struct command) * MAX_QUEUE_SIZE);

    CommandQueue.isInitComplete = true;
    return ERROR_NONE;
}

/**
 * @brief Kicks off processing of next command in queue. Should be called periodically to 
 */
int processNextCommandInQueue(){

    // Get node at head of fifo
    struct command * nextCommand; // = getNextNode

    if(nextCommand->cmdType == userCommand){
        switch(nextCommand->userCmdInfo.)

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
int enqueue_command(struct command * cmd){

    return ERROR_NONE;
}

int get_queue_status(){

    return ERROR_NONE;
}