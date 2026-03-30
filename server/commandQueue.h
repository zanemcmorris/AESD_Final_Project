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

#define MAX_QUEUE_SIZE (10)

enum errorTypes{
    ERROR_NONE = 0,
    ERROR_STATE = -1,
    ERROR_INPUT = -2,
    ERROR_MEMORY = -3,
    ERROR_EMPTY_QUEUE = -4,
    ERROR_RE_INIT = -5,
    ERROR_NO_INIT = -6,
};

enum smState{
    idle = 0,
    userMode,
    perfMode,
    error,
};

enum commandType{
    adminCommand = 0,
    userCommand,
    perfCommand
};

enum ioType{
    read_to_file = 0,
    write_from_file,
    read_to_buffer,
    write_to_buffer,
    status,
};

enum userCommandType{
    io = 0,
    list_ns,
    create_ns,
    remove_ns,
    get_status,
};

struct userCommandInfo{
    enum userCommandType;
    enum ioType;
    uint32_t namespace;
    uint32_t lba;
    // File used to either write to the drive, or to store retrieved data. Optional Argument
    char * pathToFile;
    uint32_t pathToFileLength;
    // Buffer used to write data to the drive, or store data from drive. Optional arg.
    uint8_t * buffer;
    uint32_t bufferLength;
};

/*
*/
enum perfCommandType{
    seqWrite = 0,
    seqRead,
    randWrite,
    randRead,
};

/*
    use_custom: uses our custom IO functions
    use_fio: uses FIO to do IO
*/
enum perfMechanism{
    use_custom = 0,
    use_fio,
};

struct lbaRange{
    uint32_t startlba;
    uint32_t endlba;
};

/*
*/
struct perfCommandInfo{
    enum perfCommandType;
    enum perfMechanism;
    struct lbaRange;
    
};

struct command{
    // State the machine needs to be in for the command to be applicable. Ex. User commands require user state. If perfState, then return an error
    enum smState stateNeeded; 
    // Cmd type is either userCmd or perfCommand
    enum commandType cmdType;
    // info related to command -- We need either of them, never both.
    union{
        struct userCommandInfo;
        struct perfCommandInfo;
    }cmdInfo;
};

struct CommandQueue_s{
    struct command queue [MAX_QUEUE_SIZE];
    uint32_t enqueueIndex;
    uint32_t dequeueIndex;
    pthread_mutex_t mutex;
    enum smState state;
    bool isQueueEmpty;
    bool isInitComplete;
};

// A single commandQueue instance will exist in the commandQueue.c file
// Therefore, a pointer to "which" command queue is not needed

int commandQueueInit();
int processNextCommandInQueue();
int enqueue_command(struct command * cmd);
int get_queue_status();


#endif