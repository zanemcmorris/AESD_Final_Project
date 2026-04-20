#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "queue.h"
#include <pthread.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <stddef.h>
// #include "../aesd-char-driver/aesd_ioctl.h"
// New includes for final project
#include "commandQueue.h"
#include <linux/limits.h>
#include "perfInterface.h"


#define USE_AESD_CHAR_DEVICE (0)

#if (USE_AESD_CHAR_DEVICE == 1)
#define LOG_PATH ("/dev/aesdchar")
#define LOG_OPTIONS (O_WRONLY | O_RDONLY)
#else
#define LOG_PATH ("/var/tmp/aesdsocketdata")
#define LOG_OPTIONS (O_APPEND | O_CREAT | O_RDWR)
#endif


#define MAX_SOCK_CONNECTIONS (100)
#define SOCKET_PORT ("9000")
#define RECV_BUFFER_LENGTH_BYTES (32)
#define ITIMER_PERIOD_SEC (10)


typedef struct sockaddr sockaddr_t;
typedef struct addrinfo addrinfo_t;

// Globally available addrinfo to clean after program terminates
addrinfo_t *addrinfo = NULL;
int sockfd = -1;
// int logfd = -1;
// int clientfd = -1;
volatile int endProgram = 0;
bool runAsDaemon = false;
pthread_mutex_t logMutex;
timer_t intervalTimerID = 0;

// SLIST.
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    pthread_t thread;      
    _Atomic bool isThreadComplete;
    int clientfd;
    SLIST_ENTRY(slist_data_s) entries;
};

SLIST_HEAD(threadListHead, slist_data_s) head;

int cleanupProgram();

/**
 * @brief Signal Handler. Expects SIGINT and SIGTERM to end program.
 */
void handle_sigint(int sig) {
    syslog(LOG_DEBUG, "Caught signal, exiting");

    if(sig == SIGINT || sig == SIGTERM){
        endProgram = 1;
    } else {
        syslog(LOG_DEBUG, "Caught unknown signal. Exiting.");
    }

    endProgram = 1; // Set flag to cleanup and end the program
}

/**
 * @brief POSIX Interval timer callback thread. Logs timestamps to LOG_PATH
 */
static void timerCallback(union sigval sv)
{

    (void) sv;
    // TODO: Print something to log
    char str[64] = {0};
    time_t rawTime;
    struct tm timeStruct = {0};

    time(&rawTime);
    localtime_r(&rawTime, &timeStruct);

    snprintf(str, sizeof(str), "timestamp:");
    
    int charsWritten = strftime(str+10, sizeof(str) - 10, "%a, %d %b %Y %T %z", &timeStruct);
    if(charsWritten > sizeof(str)){
        printf("Zane made bad size of arr!\n");
        return;
    }

    charsWritten += 10; // Increment by the "timestamp:" chars
    
    str[charsWritten] = '\n';
    charsWritten += 1; // Make room for newline

    int logfd = open(LOG_PATH, LOG_OPTIONS, 0666);
    if(logfd < 0){
        perror("Could not open logfd");
        return;
    }

    pthread_mutex_lock(&logMutex);
    write(logfd, str, charsWritten);
    pthread_mutex_unlock(&logMutex);

    close(logfd);

    return;
}

/**
 * @brief Create and start interval timer for logging service
 */
int startIntervalLoggingTimer()
{
    struct sigevent sev = {0};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timerCallback;
    sev.sigev_notify_attributes = NULL;

    int rc = timer_create(CLOCK_MONOTONIC, &sev, &intervalTimerID);
    if(rc == -1){
        perror("timer create failed");
        return -1;
    }

    struct itimerspec timerData = {0};
    timerData.it_interval.tv_sec = ITIMER_PERIOD_SEC;
    timerData.it_value.tv_sec = ITIMER_PERIOD_SEC;

    rc = timer_settime(intervalTimerID, 0, &timerData, NULL);
    if(rc == -1){
        perror("timer_settime failed");
        return -1;
    }

    // printf("itimer should be set up\n");

    return 0;
}

/**
 * @brief Frees allocated memory and cleans up logfile
 */
int cleanupProgram()
{
    if(addrinfo != NULL){
        freeaddrinfo(addrinfo);
        addrinfo = NULL;
    }

    if(sockfd != -1){
        close(sockfd);
        sockfd = -1;
    }

    // if(logfd != -1){
    //     close(logfd);
    //     logfd = -1;
    // }
#if (USE_AESD_CHAR_DEVICE == 0)
    int rc;
    rc = access(LOG_PATH, F_OK);
    if(rc == 0){
        rc = remove(LOG_PATH);
        if(rc == -1){
            perror("remove failed");
        }
    }
#endif
    printf("Cleaned!\n");
    return 0;
}

/**
 * @brief Helper function to print the connecting client's address and port.
 */
void printClientNameConnected(struct sockaddr *clientaddr, size_t clientAddrSize){
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int rc;

    rc = getnameinfo((struct sockaddr *) clientaddr,
                        clientAddrSize,
                        host,
                        sizeof(host),
                        service,
                        sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV);

    if (rc != 0) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rc));
    } else {
        syslog(LOG_DEBUG, "Accepted connection from %s:%s\n", host, service);
    }
}

/**
 * @brief Helper function to show the disconnected client's address and port 
 * 
 */
void printClientNameDisconnected(struct sockaddr *clientaddr){
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int rc;

    rc = getnameinfo((struct sockaddr *) clientaddr,
                        sizeof(*clientaddr),
                        host,
                        sizeof(host),
                        service,
                        sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV);

    if (rc != 0) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rc));
    } else {
        syslog(LOG_DEBUG, "Closed connection from %s:%s\n", host, service);

    }
}

/**
 * @brief Opens a stream-style socket on port specified
 * @return 0 on success, -1 on failure.
 */
int openSocket(const char* port){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);    
    if(sockfd == -1){
        perror("no sockfd returned from socket()");
        return -1;
    }

    struct addrinfo hints = {.ai_flags = AI_PASSIVE, .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    int rc;
    rc = getaddrinfo(NULL, port, &hints, &addrinfo);
    if(rc != 0){
        perror("getaddrinfo failed");
        return -1;
    }

    int val = 1;
    rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if(rc == -1){
        perror("Failed setsockopt");
        return -1;
    }

    rc = bind(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if(rc == -1){
        perror("bind failed");
        return -1;
    }

    if(runAsDaemon)
    {
        pid_t newpid = fork();
        if(newpid != 0){
            sleep(1); // Wait a moment for the daemon to init, then exit
            exit(0);
        } else {
            // Daemon setup
            if(setsid() < 0){
                return -1;
            }

            int nullfd = open("/dev/null", O_RDWR);
            if (nullfd < 0) return -1;

            // Force stdin/stdout/stderr to /dev/null
            if (dup2(nullfd, STDIN_FILENO)  < 0) return -1;
            if (dup2(nullfd, STDOUT_FILENO) < 0) return -1;
            if (dup2(nullfd, STDERR_FILENO) < 0) return -1;

        }
    }

    return 0;

}

/**
 * @brief Sends the full log of what was received on the socket to newfd
 * @details NOT thread-safe!! Use mutex around me for logdata!
 * @return Returns 0 on success, -1 on failure.
 */
int sendFullLog(int newfd)
{
    int fd = open(LOG_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open log for read");
        return -1;
    }

    char buf[64];
    while (1) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) {
            perror("read log");
            close(fd);
            return -1;
        }

        ssize_t sent = 0;
        while (sent < n) {
            ssize_t m = send(newfd, buf + sent, (size_t)(n - sent), 0);
            if (m < 0) {
                perror("send");
                close(fd);
                return -1;
            }
            sent += m;
        }
    }

    close(fd);
    return 0;
}

/**
 * @brief Sends the full log of what was received on the socket to newfd from logfd
 * @details NOT thread-safe!! Use mutex around me for logdata!
 *          Does not close logfd itself!
 * @return Returns 0 on success, -1 on failure.
 */
int sendFullLogWExistingFD(int newfd, int logfd)
{
    char buf[64];
    while (1) {
        ssize_t n = read(logfd, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) {
            perror("read log");
            return -1;
        }

        ssize_t sent = 0;
        while (sent < n) {
            ssize_t m = send(newfd, buf + sent, (size_t)(n - sent), 0);
            if (m < 0) {
                perror("send");
                return -1;
            }
            sent += m;
        }
    }

    return 0;
}


/**
 * @brief print hex bytes in hexdump style to stdout
 * @param data Pointer to data to print
 * @param len length of buffer in bytes
 * @return void
 */
void hexDump(const void *data, size_t len)
{
    const unsigned char *buf = (const unsigned char *)data;

    if (buf == NULL) {
        printf("(null)\n");
        return;
    }

    for (size_t i = 0; i < len; i += 16) {
        printf("%08zx  ", i);

        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02X ", buf[i + j]);
            } else {
                printf("   ");
            }

            if (j == 7) {
                printf(" ");
            }
        }

        printf(" |");

        for (size_t j = 0; j < 16 && i + j < len; j++) {
            unsigned char c = buf[i + j];
            printf("%c", isprint(c) ? c : '.');
        }

        printf("|\n");
    }
}

/**
 * @brief Send command usage to client
 * @param clientfd Client fd to send information to
 */
static void sendCommandUsage(int clientfd){
    char * str = "----- Start Command Usage -----\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "help/? -- prints this command usage screen\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "status -- prints status of drive\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "listparts/ls -- list active partitions on the drive\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "mkpart <size_in_sectors> \n\0";
    send(clientfd, str, strlen(str), 0);

    str = "rmpart <part_number> \n\0";
    send(clientfd, str, strlen(str), 0);
    
    str = "write <data> <part> <offset> -- writes <data> to <part>\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "read <part> <offset> <num_sectors> -- reads <num_sectors> from <part> \n\0";
    send(clientfd, str, strlen(str), 0);

    str = "\n----- Performance Command Usage -----\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "seq_write <part> <sector_start> <sector_end> <duration_ms>\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "seq_read <part> <sector_start> <sector_end> <duration_ms>\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "rand_write <part> <sector_start> <sector_end> <duration_ms>\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "rand_read <part> <sector_start> <sector_end> <duration_ms>\n\0";
    send(clientfd, str, strlen(str), 0);

    str = "quit --  ends session\n\0";
    send(clientfd, str, strlen(str), 0);

    

    str = "\n----- End Command usage -----\n\n\0";
    send(clientfd, str, strlen(str), 0);
    
}

/**
 * @brief Helper for sending a string to a socket without having to mess with strlen
 */
static inline int sendOnSocketf(int sockfd, const char *fmt, ...)
{
    if (sockfd < 0 || fmt == NULL) {
        return -1;
    }

    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) {
        return -1;
    }

    if (send(sockfd, buffer, strnlen(buffer, sizeof(buffer)), 0) < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Pthread recieving thread for multi-threaded server. Spawned on each new connection
 * @arg Pointer to linkedlist node containing thread-specific information  
 * @return None
 */
void* repsondingThread(void* arg)
{
    slist_data_t *myNodeData = (slist_data_t*) (arg);
    int clientFD = myNodeData->clientfd;
    const char* welcomeString = "Welcome to the NVME command server!\n\n\n";
    send(clientFD, welcomeString, strlen(welcomeString), 0);
    sendCommandUsage(clientFD);
    bool threadActive = true;
    perfJobInfo_t perfJob = {0};

    while(threadActive && !endProgram)
    {
        // Set up to recv from client
        size_t bufferCapacity = RECV_BUFFER_LENGTH_BYTES;
        char *buffer = (char*) malloc(bufferCapacity);
        memset(buffer, 0, bufferCapacity);
        int totalBytesRecvd = 0; // Reset num bytes received for this message
        bool failedToRead = false;
        int rc = 0;

        // Receive all the bytes now
        while(1)
        {
            // If received bytes on last ittr was at buffer capacity, then double capacity and keep going
            if(totalBytesRecvd == bufferCapacity){
                bufferCapacity *= 2;
                char *temp = realloc(buffer, bufferCapacity);
                if(temp == NULL){
                    free(buffer);
                    // toss this message and go next if malloc failed 
                    failedToRead = true;
                    printf("Failed to malloc large enough buffer.\n");
                    break;
                }

                buffer = temp;
                // printf("Doubled buffer size. Now %ld bytes\n", bufferCapacity);
            }

            // Recieve n bytes from the client
            int n = recv(clientFD, buffer + totalBytesRecvd, bufferCapacity - totalBytesRecvd, 0);
            if(n < 0){
                // Error on recv
                free(buffer);
                buffer = NULL;
                syslog(LOG_DEBUG, "recv failed");
                perror("recv failed");
                return NULL;
            }

            if(n == 0){
                // We read all the data out of the socket
                threadActive = false;
                break;
            }

            totalBytesRecvd += n;
            // printf("Just read %d bytes, making total of %d\n", n, totalBytesRecvd);

            // Only stop loop if the final char is a newline
            if(buffer[totalBytesRecvd-1] == '\n'){
                break;
            }
        }

        // bool wasIoctlCommand = false;

        // Trap for failed to read to hit the cleanup and return step at the end of function
        if(!failedToRead){
            // If we read completely, write message to the log, free the buffer and echo back log
            // buffer[totalBytesRecvd] = 0; // Set null-terminator
            // printf("new buffer: %s", buffer);
            syslog(LOG_DEBUG, "Recvd string: %s", buffer);
            char *saveptr = NULL;
            char *cmd = strtok_r(buffer, " \t\r\n", &saveptr);
            char *arg1 = strtok_r(NULL, " \t\r\n", &saveptr);
            char *arg2 = strtok_r(NULL, " \t\r\n", &saveptr);
            char *arg3 = strtok_r(NULL, " \t\r\n", &saveptr);
            char *extra = strtok_r(NULL, " \t\r\n", &saveptr);

            if(cmd == NULL){
                continue;
            }

            printf("cmd: %s | arg1:%s | arg2:%s | arg3:%s | extra:%s\n", cmd, arg1, arg2, arg3, extra);

            // TODO: Call into command server here.
            if(strncmp(cmd, "?", sizeof("?")) == 0 || strncmp(cmd, "help", sizeof("help")) == 0){
                sendCommandUsage(clientFD);
            }
            else if(strncmp(cmd, "status", sizeof("status")) == 0)
            {
                int savedfd = dup(STDOUT_FILENO);
                dup2(clientFD, STDOUT_FILENO);
                nvmeGetStatus();
                dup2(savedfd, STDOUT_FILENO); // hack to not have to re-write the getStatus function
            }
            else if(strncmp(cmd, "listparts", sizeof("listparts")) == 0 || strncmp(cmd, "ls", sizeof("ls")) == 0)
            {
                printf("listing parts\n");

                int savedfd = dup(STDOUT_FILENO);
                dup2(clientFD, STDOUT_FILENO);
                rc = nvmeListPartitions();
                dup2(savedfd, STDOUT_FILENO); // hack to not have to re-write the nvmeListPartitions function

                if(rc != NVME_STATUS_OK){
                    sendOnSocketf(clientFD, "Encountered and error while listing drive partitions.\n");
                }
            }
            else if(strncmp(cmd, "quit", sizeof("quit")) == 0){
                threadActive = false;
            } else if (strncmp(cmd, "mkpart", sizeof("mkpart")) == 0){
                printf("mkpart received\n");
                if(arg1 == NULL){
                    sendOnSocketf(clientFD, "expected second argument for size of new partition.\n");
                }
                int newPartSize = atoi(arg1);
                uint8_t newPartIndex = 0;
                if(newPartSize > 0){
                    rc = nvmeCreatePartition(newPartSize, &newPartIndex);

                    if(rc != NVME_STATUS_OK){
                        sendOnSocketf(clientFD, "\nfailed to create partition.\n");
                    } else {
                        sendOnSocketf(clientFD, "\ncreated new partition with number %d\n", newPartIndex);
                    }
                } else {
                    sendOnSocketf(clientFD, "expected greater than zero argument for new partition size.\n");
                }
                
                
            } else if (strncmp(cmd, "rmpart", sizeof("rmpart")) == 0){
                printf("rmpart received\n");

                if(arg1 == NULL){
                    sendOnSocketf(clientFD, "expected second argument for number of partition to delete\n");
                }
                int partToDel = atoi(arg1);
                if(partToDel > 0){
                    rc = nvmeDeletePartition(partToDel);
                    if(rc != NVME_STATUS_OK){
                        sendOnSocketf(clientFD, "failed to delete partition.\n");
                    } else {
                        sendOnSocketf(clientFD, "deleted part with number %d\n", partToDel);
                    }
                } else {
                    sendOnSocketf(clientFD, "expected greater than zero argument for partition to delete.\n");
                }
            } else if(strncmp(cmd, "write", sizeof("write")) == 0){
                printf("got write\n");
                //    str = "write <data> <part> <offset> -- writes <data> to <part>\n\0";
                int partToWrite = 0;
                if(arg2 == NULL){
                    sendOnSocketf(clientFD, "Expected a partition to write to. Try again.\n");
                    continue;
                } else {
                    partToWrite = atoi(arg2);
                }



                int sectorOffset = 0;
                if(arg3 == NULL){
                    sendOnSocketf(clientFD, "No socket offset received. Assuming 0.\n");
                    sectorOffset = 0;
                } else {
                    sectorOffset = atoi(arg3);
                }
                sendOnSocketf(clientFD, "Writing %lld bytes to part %d at sector %d\n", strlen(arg1), partToWrite, sectorOffset);

                rc = nvmeWritePartitionSector(partToWrite, sectorOffset, arg1, strlen(arg1));
                if(rc != NVME_STATUS_OK){
                    sendOnSocketf(clientFD, "An error occured during the write operation.\n");
                }
            } else if(strncmp(cmd, "read", sizeof("read")) == 0){
                bool printAsHex = false;
                //"read <part> <offset> <num_sectors>

                int partToRead = 0;
                if(arg1 == NULL){
                    sendOnSocketf(clientFD, "Expected a partition to write to. Try again.\n");
                    continue;
                } else {
                    partToRead = atoi(arg1);
                }



                int sectorOffset = 0;
                if(arg2 == NULL){
                    sendOnSocketf(clientFD, "No socket offset received. Assuming 0.\n");
                    sectorOffset = 0;
                } else {
                    sectorOffset = atoi(arg2);
                }

                int sectorsToRead = 0;
                if(arg3 == NULL){
                    sendOnSocketf(clientFD, "Expected a number of sectors to read. Try again.\n");
                    continue;
                } else {
                    sectorsToRead = atoi(arg3);
                    if(sectorsToRead <= 0){
                        sendOnSocketf(clientFD, "Expected a number of sectors to read greater than 0.\n");
                        continue;
                    }
                }

                if(extra != NULL){
                    if (strncmp(extra, "-b", sizeof("-b")) == 0){
                        // -b flag was present
                        // Print as bytes, not ascii
                        printAsHex = true;
                    }
                }
                size_t readBufferSize = sectorsToRead * 512; // TODO: replace 512 with sector size.
                printf("readbuffersize: %ld\n", readBufferSize);
                char *readBuffer = malloc(sizeof(char) * readBufferSize); 
                sendOnSocketf(clientFD, "Reading %lld bytes from part %d at sector %d\n", readBufferSize, partToRead, sectorOffset);
                rc = nvmeReadPartitionSector(partToRead, sectorOffset, readBuffer, readBufferSize);
                
                if(rc == NVME_STATUS_PART_DNE){
                    sendOnSocketf(clientFD, "Specified partition does not exist.\n");
                    free(readBuffer);
                    continue;
                }
                if(rc != NVME_STATUS_OK){
                    sendOnSocketf(clientFD, "Encountered an error during read\n");
                    free(readBuffer);
                    continue;
                }
                readBuffer[readBufferSize - 1] = 0; // Null terminate

                if(printAsHex){
                    int savedfd = dup(STDOUT_FILENO);
                    dup2(clientFD, STDOUT_FILENO);
                    hexDump(readBuffer, readBufferSize);
                    dup2(savedfd, STDOUT_FILENO); // hack to not have to re-write hexdump function

                } else {
                    sendOnSocketf(clientFD, "Read %s from socket\n", readBuffer);
                }

                free(readBuffer);
            } else if(strncmp(cmd, "seq_write", sizeof("seq_write")) == 0 || strncmp(cmd, "sw", sizeof("sw")) == 0) {
                // TODO: Finish perf integration here 
                
                if(arg1 == NULL || arg2 == NULL || arg3 == NULL || extra == NULL){
                    sendOnSocketf(clientFD, "expected all arguments. Try again.\n");
                    continue;
                }
                // "seq_write <part> <sector_start> <sector_end> <duration_ms>\n\0";
                
                int partToPerf = atoi(arg1);
                perfJob.lbaRange.startlba = atoi(arg2);
                perfJob.lbaRange.endlba = atoi(arg3);
                perfJob.duration_ms = atoi(extra);

                
                // add partNumber.Start to lba range
                
                if(perfJob.lbaRange.startlba > perfJob.lbaRange.endlba || perfJob.duration_ms < 0){
                    sendOnSocketf(clientFD, "Bad inputs. Try again.\n");
                }

                // get partNumber start geo sector here
                
                size_t partStartAddr = 0;
                rc = nvmeCheckLbaRangeInPart(partToPerf, perfJob.lbaRange);
                if(rc == NVME_STATUS_ERROR){
                    sendOnSocketf(clientFD, "Lba range is not possible on this partition (%d).\n", partToPerf);
                }

                // TODO: Get partStartAddr too
                perfJob.lbaRange.startlba += partStartAddr;
                perfJob.lbaRange.endlba += partStartAddr;
                
                // perfStartSeqWrite(&perfJob);


                printf("got seq_write\n");
            }else if(strncmp(cmd, "seq_read", sizeof("seq_read")) == 0 || strncmp(cmd, "sr", sizeof("sr")) == 0){
                if(arg1 == NULL || arg2 == NULL || arg3 == NULL || extra == NULL){
                    sendOnSocketf(clientFD, "expected all arguments. Try again.\n");
                    continue;
                }
                printf("got seq read\n");
            } else if(strncmp(cmd, "rand_write", sizeof("rand_write")) == 0 || strncmp(cmd, "rw", sizeof("rw")) == 0) {
                if(arg1 == NULL || arg2 == NULL || arg3 == NULL || extra == NULL){
                    sendOnSocketf(clientFD, "expected all arguments. Try again.\n");
                    continue;
                }
                printf("got rand write\n");
            } else if(strncmp(cmd, "rand_read", sizeof("rand_read")) == 0 || strncmp(cmd, "rr", sizeof("rr")) == 0) {
                if(arg1 == NULL || arg2 == NULL || arg3 == NULL || extra == NULL){
                    sendOnSocketf(clientFD, "expected all arguments. Try again.\n");
                    continue;
                }
                printf("got rand read\n");
            } 
        }        
    }
    shutdown(clientFD, SHUT_RDWR);
    close(clientFD);
    clientFD = -1;
    atomic_store(&myNodeData->isThreadComplete, true);
    syslog(LOG_DEBUG, "Thread cleaned up");
    return NULL;
}

/**
 * @brief Main loop that starts listening on the opened socket and dispatches new threads upon connection
 *          Only returns via upon receiving SIGINT or SIGTERM.
 * @return Returns 1 on successful cleanup, -1 on failure. Does not return without catching a signal to do so.
 */
int listenLoop()
{
    int rc = 0;
    struct sockaddr_storage clientaddr;

    printf("starting to listen...\n");
    rc = listen(sockfd, MAX_SOCK_CONNECTIONS);
    if(rc == -1){
        perror("listen failed");
    }

    uint32_t clientAddrSize = sizeof(clientaddr);
    SLIST_INIT(&head); // Create LL for threadss

    do
    {
        // accept is a blocking call. Execution will wait here for a connection
        // printf("Main thread accepting new connections...\n");
        syslog(LOG_DEBUG, "Main loop ready to accept new connection");
        int clientfd = accept(sockfd, (sockaddr_t*) &clientaddr, &clientAddrSize);
        if(clientfd == -1){
            if(errno == EINTR && endProgram){
                break;
            } else {
                perror("accept failed");
            }
        }
        printClientNameConnected((sockaddr_t *) &clientaddr, clientAddrSize);
        syslog(LOG_DEBUG, "New client - making new thread");

        // Create new LinkedList node for the new pthread and give it the clientfd to act on
        slist_data_t *newNode = NULL;
        newNode = malloc(sizeof(slist_data_t));
        memset(newNode, 0, sizeof(*newNode));
        newNode->clientfd = clientfd;
        SLIST_INSERT_HEAD(&head, newNode, entries);

        // Create thread to respond to server request
        pthread_create(&newNode->thread, NULL, repsondingThread, newNode);
        if(newNode == NULL){
            printf("Failed to make newNode in listenloop\n");
            return -1;
        }

        // Look to clean up any threads that are complete
        slist_data_t* dataptr = NULL, *tmp = NULL;
        SLIST_FOREACH_SAFE(dataptr, &head, entries, tmp){
            if(atomic_load(&dataptr->isThreadComplete)){
                SLIST_REMOVE(&head, dataptr, slist_data_s, entries);
                pthread_join(dataptr->thread, NULL);
                free(dataptr);

            }
        }

    }while(!endProgram);

    syslog(LOG_DEBUG, "Endprogram caught - cleaning up LL");

    // Program is ending. Clean up whole LL
    slist_data_t* dataptr = NULL, *tmp = NULL;
    SLIST_FOREACH_SAFE(dataptr, &head, entries, tmp){
        SLIST_REMOVE(&head, dataptr, slist_data_s, entries);
        pthread_join(dataptr->thread, NULL);
        free(dataptr);
    }

    return 0;

}

/**
 * @brief entry point for program
 */
int main(int argc, char ** argv){

    if(argc == 2)
    {
        if (strcmp(argv[1], "-d") == 0){
            runAsDaemon = true;
        }
    } else if(argc != 1){
        printf("Bad args to aesdsocket\n");
        return -1;
    }

    #if USE_AESD_CHAR_DEVICE
    printf("Using char device for ased socket!\n");
    #else
    printf("Using regular log for aesd socket!\n");
    #endif

    openlog("aesdsocket", LOG_PERROR, LOG_USER);
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // open a socket on port 9000
    int rc = openSocket(SOCKET_PORT);

    // Now init mutex to protect the log data
    rc = pthread_mutex_init(&logMutex, NULL);
    if(rc == -1){
        perror("mutex create failed");
    }    

    commandQueueInit();
    initPerfSystem();

    // Listen for and accept new connection
    if(rc == 0)
        rc = listenLoop();

    cleanupProgram();

    syslog(LOG_DEBUG, "socketserver closing");
    return rc;
}