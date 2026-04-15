/*
    @file commandQueue.c
    @Author Zane Mcmorris
    @date March 30, 2026
*/

#include "commandQueue.h"
#include <string.h>

CommandQueue_t CommandQueue;

static inline uint32_t queue_next_index(uint32_t index)
{
    return (index + 1U) % MAX_QUEUE_SIZE;
}

static inline bool queue_is_full_locked(void)
{
    return (!CommandQueue.isQueueEmpty &&
            (CommandQueue.enqueueIndex == CommandQueue.dequeueIndex));
}

static inline bool queue_is_empty_locked(void)
{
    return CommandQueue.isQueueEmpty;
}

static int validate_command(const command_t *cmd)
{
    if (cmd == NULL) {
        return ERROR_INPUT;
    }

    switch (cmd->cmdType) {
        case adminCommand:
        case userCommand:
        case perfCommand:
            break;
        default:
            return ERROR_INPUT;
    }

    switch (cmd->stateNeeded) {
        case idle:
        case userMode:
        case perfMode:
        case error:
            break;
        default:
            return ERROR_INPUT;
    }

    return ERROR_NONE;
}

static int execute_user_command(const userCommandInfo_t *cmd)
{
    if (cmd == NULL) {
        return ERROR_INPUT;
    }

    switch (cmd->userCommandType) {
        case io:
            switch (cmd->ioType) {
                case status:
                    return nvmeGetStatus();
                case read_to_buffer:
                    if (cmd->buffer == NULL || cmd->bufferLength == 0U) {
                        return ERROR_INPUT;
                    }
                    return nvmeReadPartitionSector(1, cmd->sector, (char *)cmd->buffer,
                                                   cmd->bufferLength);
                case write_to_buffer:
                    if (cmd->buffer == NULL || cmd->bufferLength == 0U) {
                        return ERROR_INPUT;
                    }
                    return nvmeWritePartitionSector(1, cmd->sector, (char *)cmd->buffer,
                                                    cmd->bufferLength);
                case read_to_file:
                case write_from_file:
                default:
                    return ERROR_NOT_SUPPORTED;
            }

        case list_ns:
            return nvmeListNamespace();

        case create_ns: {
            uint8_t newPartNumber = 0;
            return nvmeCreatePartition(cmd->sector, &newPartNumber);
        }

        case remove_ns:
            return nvmeDeletePartition((int)cmd->sector);

        case get_status:
            return nvmeGetStatus();

        default:
            return ERROR_INPUT;
    }
}

static int execute_perf_command(const command_t *cmd)
{
    (void)cmd;
    return ERROR_NOT_SUPPORTED;
}

static int execute_admin_command(const command_t *cmd)
{
    (void)cmd;
    return ERROR_NOT_SUPPORTED;
}

/**
 * @brief Initialize NVME command queue.
 * @return ERROR_NONE on success
 */
int commandQueueInit()
{
    memset(&CommandQueue, 0, sizeof(CommandQueue));

    if (pthread_mutex_init(&CommandQueue.mutex, NULL) != 0) {
        return ERROR_GENERAL;
    }

    CommandQueue.enqueueIndex = 0U;
    CommandQueue.dequeueIndex = 0U;
    CommandQueue.isQueueEmpty = true;
    CommandQueue.isInitComplete = true;
    CommandQueue.state = idle;

    return ERROR_NONE;
}

/**
 * @brief Kicks off processing of next command in queue.
 */
int processNextCommandInQueue()
{
    command_t nextCommand;
    int ret = ERROR_NONE;

    if (!CommandQueue.isInitComplete) {
        return ERROR_NO_INIT;
    }

    pthread_mutex_lock(&CommandQueue.mutex);

    if (queue_is_empty_locked()) {
        pthread_mutex_unlock(&CommandQueue.mutex);
        return ERROR_EMPTY_QUEUE;
    }

    nextCommand = CommandQueue.queue[CommandQueue.dequeueIndex];
    memset(&CommandQueue.queue[CommandQueue.dequeueIndex], 0, sizeof(command_t));
    CommandQueue.dequeueIndex = queue_next_index(CommandQueue.dequeueIndex);

    if (CommandQueue.dequeueIndex == CommandQueue.enqueueIndex) {
        CommandQueue.isQueueEmpty = true;
    }

    pthread_mutex_unlock(&CommandQueue.mutex);

    if (nextCommand.stateNeeded != CommandQueue.state && nextCommand.stateNeeded != idle) {
        return ERROR_STATE;
    }

    switch (nextCommand.cmdType) {
        case userCommand:
            ret = execute_user_command(&nextCommand.userCmdInfo);
            break;
        case perfCommand:
            ret = execute_perf_command(&nextCommand);
            break;
        case adminCommand:
            ret = execute_admin_command(&nextCommand);
            break;
        default:
            ret = ERROR_INPUT;
            break;
    }

    return ret;
}

int enqueue_command(command_t *cmd)
{
    int ret;

    if (!CommandQueue.isInitComplete) {
        return ERROR_NO_INIT;
    }

    ret = validate_command(cmd);
    if (ret != ERROR_NONE) {
        return ret;
    }

    pthread_mutex_lock(&CommandQueue.mutex);

    if (queue_is_full_locked()) {
        pthread_mutex_unlock(&CommandQueue.mutex);
        return ERROR_FULL_QUEUE;
    }

    CommandQueue.queue[CommandQueue.enqueueIndex] = *cmd;
    CommandQueue.enqueueIndex = queue_next_index(CommandQueue.enqueueIndex);
    CommandQueue.isQueueEmpty = false;

    pthread_mutex_unlock(&CommandQueue.mutex);

    return ERROR_NONE;
}

int get_queue_status()
{
    int ret;

    if (!CommandQueue.isInitComplete) {
        return ERROR_NO_INIT;
    }

    pthread_mutex_lock(&CommandQueue.mutex);

    if (queue_is_empty_locked()) {
        ret = ERROR_EMPTY_QUEUE;
    } else if (queue_is_full_locked()) {
        ret = ERROR_FULL_QUEUE;
    } else {
        ret = ERROR_NONE;
    }

    pthread_mutex_unlock(&CommandQueue.mutex);

    return ret;
}
