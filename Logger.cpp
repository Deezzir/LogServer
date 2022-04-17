
#include "Logger.h"

#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

#include "Utils.h"

using namespace std;

void *recv_func(void *arg);

bool is_running;                              // status
int master_fd;                                // master socket file descriptor
pthread_t tid;                                // receive thread id
struct sockaddr_in serv_addr;                 // global sockaddr of the server
socklen_t serv_addr_len = sizeof(serv_addr);  // global client address length
pthread_mutex_t lock_x;                       // global mutex
LOG_LEVEL log_filter = LOG_LEVEL::DEBUG;      // default filter

/*Socket Setup function is responsible for*/
/*creating a non-blocking socket for UDP */
/*return file descriptor of the creaeted socket*/
int InitializeLog() {
    // Create the socket
    if ((master_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error((char *)"Error while creating the socket", true);
    }

    // Set file descriptor to non-blocking
    if (fcntl(master_fd, F_SETFL, (fcntl(master_fd, F_GETFL) | O_NONBLOCK)) < 0) {
        close(master_fd);
        print_error((char *)"Error while setting FD to O_NONBLOCK", true);
    }

    // Set port and IP
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    is_running = true;

    // Create receive thread
    if (pthread_create(&tid, NULL, recv_func, &master_fd) != 0) {
        close(master_fd);
        print_error((char *)"Error while creating a recv thread", true);
    }

    return master_fd;
}
/*Function sets the filter log level and store in a variable global*/
void SetLogLevel(LOG_LEVEL level) {
    log_filter = level;
}
/*Function is responsible for sending logs to the server*/
void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message) {
    if (level >= log_filter) {
        char buffer[BUF_LEN];
        int len;
        time_t now = time(0);
        char *dt = ctime(&now);

        memset(buffer, 0, BUF_LEN);
        char levelStr[][16] = {"DEBUG", "WARNING", "ERROR", "CRITICAL"};
        len = sprintf(buffer, "%s %s %s:%s:%d %s\n", dt, levelStr[level], prog, func, line, message) + 1;
        buffer[len - 1] = '\0';

        pthread_mutex_lock(&lock_x);
        sendto(master_fd, (char *)buffer, len, 0, (struct sockaddr *)&serv_addr, serv_addr_len);
        pthread_mutex_unlock(&lock_x);
        memset(buffer, 0, BUF_LEN);
    }
}
/* Function that is responsible for releasing locks */
/* and dynamically allocated memory */
void ExitLog() {
    is_running = false;
    pthread_join(tid, NULL);
    pthread_mutex_destroy(&lock_x);
    close(master_fd);
}
/*Receive thread function responisble for communication with the server*/
void *recv_func(void *arg) {
    int fd = *(int *)arg;
    struct sockaddr_in serv_addr;
    char buffer[BUF_LEN];
    int len;

    while (is_running) {
        memset(buffer, 0, BUF_LEN);
        // mutex lock
        pthread_mutex_lock(&lock_x);
        len = recvfrom(fd, (char *)buffer, BUF_LEN, 0, (struct sockaddr *)&serv_addr, &serv_addr_len);
        pthread_mutex_unlock(&lock_x);

        if (len > 0) {
            buffer[len] = '\0';
            int level = atoi(strrchr(buffer, '=') + 1);
            SetLogLevel(static_cast<LOG_LEVEL>(level));
        } else {
            sleep(1);
        }
    }

    // release fd and memory
    memset(buffer, 0, BUF_LEN);
    pthread_exit(NULL);
}