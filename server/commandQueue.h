/*
    @file commandQueue.h
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "interfaceCommon.h"

#define MAX_QUEUE_SIZE (10)
typedef enum smState_e{
    idle = 0,
    userMode,
    perfMode,
    error,
}smState_t;

enum commandType_e{
    adminCommand = 0,
    userCommand,
    perfCommand
};

typedef enum ioType_e{
    read_to_file = 0,
    write_from_file,
    read_to_buffer,
    write_to_buffer,
    status,
}ioType_t;

typedef enum userCommandType_e{
    io = 0,
    list_ns,
    create_ns,
    remove_ns,
    get_status,
}userCommandType_t;

typedef struct userCommandInfo_s{
    userCommandType_t userCommandType;
    ioType_t ioType;
    uint32_t namespace;
    uint32_t lba;
    // File used to either write to the drive, or to store retrieved data. Optional Argument
    char * pathToFile;
    uint32_t pathToFileLength;
    // Buffer used to write data to the drive, or store data from drive. Optional arg.
    uint8_t * buffer;
    uint32_t bufferLength;
}userCommandInfo_t;

typedef struct command_s{
    // State the machine needs to be in for the command to be applicable. Ex. User commands require user state. If perfState, then return an error
    smState_t stateNeeded; 

    // Cmd type is either userCmd or perfCommand
    enum commandType_e cmdType;

    // info related to command -- We need either of them, never both.
    // TODO: Replace two structs with a union
    userCommandInfo_t userCmdInfo;
    userCommandInfo_t perCmdInfo;
    
}command_t;

typedef struct CommandQueue_s{
    command_t queue [MAX_QUEUE_SIZE];
    uint32_t enqueueIndex;
    uint32_t dequeueIndex;
    pthread_mutex_t mutex;
    smState_t state;
    bool isQueueEmpty;
    bool isInitComplete;
}CommandQueue_t;

/*
*/
typedef enum perfCommandType_e{
    seqWrite = 0,
    seqRead,
    randWrite,
    randRead,
}perfCommandType_t;

/*
    use_custom: uses our custom IO functions
    use_fio: uses FIO to do IO
*/
typedef enum perfMechanism_e{
    use_custom = 0,
    use_fio,
}perfMechanism_t;

/*
*/
typedef struct perfCommandInfo_s{
    perfCommandType_t perfCommandType;
    perfMechanism_t perfMechanism;
    lbaRange_t lbaRange;
}perfCommandInfo_t;

// A single commandQueue instance will exist in the commandQueue.c file
// Therefore, a pointer to "which" command queue is not needed

int commandQueueInit();
int processNextCommandInQueue();
int enqueue_command(command_t * cmd);
int get_queue_status();


#endif