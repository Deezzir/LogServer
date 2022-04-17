#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>

#include "Logger.h"
#include "Utils.h"

void signal_handler(int signal);
int socket_setup();
void *recv_func(void *arg);
void exit_handler(int ev, void *arg);
void dump_log();
void set_log_level(int fd);
int uesr_menu();

// struct to be sent to exit_handler
struct exit_data {
    pthread_t tid_;  // receive thread id
    int fd_;         // file descriptor
} data;

const char *log_file = "logServer.log";     // log file name
bool is_running;                            // status
pthread_mutex_t lock_x;                     // global mutex
struct sockaddr_in rem_addr;                // global client address
socklen_t rem_addr_len = sizeof(rem_addr);  // global client address length

int main(int argc, char const *argv[]) {
    // Setup signal handler
    struct sigaction action;
    action.sa_handler = &signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        print_error((char *)"Error while setting action for a signal", true);
    }

    // Setup socket
    int master_fd = socket_setup();

    pthread_mutex_init(&lock_x, NULL);
    is_running = true;
    bool exit = false;
    pthread_t tid;
    int opt;

    // Create receive thread
    if (pthread_create(&tid, NULL, recv_func, &master_fd) != 0) {
        is_running = false;
        close(master_fd);
        print_error((char *)"Error while creating a recv thread", true);
    }

    // Transfer data to be freed on exit to the handler
    data.tid_ = tid;
    data.fd_ = master_fd;
    on_exit(exit_handler, &data);

    // User main menu
    do {
        opt = uesr_menu();
        switch (opt) {
            case 1:
                system("clear");
                set_log_level(master_fd);
                pause_screen();
                system("clear");
                break;
            case 2:
                system("clear");
                dump_log();
                pause_screen();
                system("clear");
                break;
            case 0:
                std::cout << "Exit the program? Y/y N/n: ";
                exit = yes();
                is_running = false;
                system("clear");
                break;
        }
    } while (!exit);

    return 0;
}
/*Socket Setup function is responsible for*/
/*creating a non-blocking socket for UDP */
/*return file descriptor of the creaeted socket*/
int socket_setup() {
    int fd;
    struct sockaddr_in master_addr;

    // Create the socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error((char *)"Error while creating the socket", true);
    }

    // Set file descriptor to non-blocking
    if (fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL) | O_NONBLOCK)) < 0) {
        close(fd);
        print_error((char *)"Error while setting FD to O_NONBLOCK", true);
    }

    // Set port and IP
    memset((char *)&master_addr, 0, sizeof(master_addr));
    memset((char *)&rem_addr, 0, sizeof(rem_addr));
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr.s_addr = INADDR_ANY;
    master_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(fd, (struct sockaddr *)&master_addr, sizeof(master_addr)) < 0) {
        close(fd);
        print_error((char *)"Error while binding the socket", true);
    } else {
        std::cout << "LogServer: socket(" << fd << ") bound to address "
                  << inet_ntoa(master_addr.sin_addr) << std::endl;
    }

    return fd;
}
/*Receive thread function responisble for populating log file*/
void *recv_func(void *arg) {
    int fd = *(int *)arg;
    char buffer[BUF_LEN];
    int len, log_fd;

    // Open write only log file with rw-rw-rw-
    int openFlags = O_WRONLY | O_CREAT | O_APPEND;
    mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    if ((log_fd = open(log_file, openFlags, filePerms)) == -1) {
        print_error((char *)"Error while opening log file", true);
    }

    while (is_running) {
        memset(buffer, 0, BUF_LEN);
        // mutex lock
        pthread_mutex_lock(&lock_x);
        len = recvfrom(fd, (char *)buffer, BUF_LEN, 0, (struct sockaddr *)&rem_addr, &rem_addr_len);
        pthread_mutex_unlock(&lock_x);

        if (len > 0) {
            buffer[len] = '\0';
            write(log_fd, buffer, len);
        } else {
            sleep(1);
        }
    }

    // release fd and memory
    memset(buffer, 0, BUF_LEN);
    close(log_fd);
    pthread_exit(NULL);
}
/*Function to dump the log file contents into the terminal*/
void dump_log() {
    std::fstream log_fp;
    std::string line;

    log_fp.open(log_file, std::fstream::in);
    if (!log_fp) {
        print_error((char *)"Error while opening log file");
    } else if (log_fp.peek() == std::fstream::traits_type::eof()) {
        std::cout << log_file << " is empty" << std::endl;
    } else {
        std::cout << "Content of " << log_file << ":" << std::endl;
        while (std::getline(log_fp, line)) {
            std::cout << line << std::endl;
        }
    }
    log_fp.close();
}
/*Function to set the log level*/
void set_log_level(int fd) {
    if (ntohs(rem_addr.sin_port) != 0) {  // check if Logger is connected
        int log_level, len;
        char buffer[BUF_LEN];
        int counter = 0;

        std::cout << "Logger Filter Levels:" << std::endl;
        for (auto i = std::begin(log_level_str); i != std::end(log_level_str); i++) {
            std::cout << counter << ". " << *i << std::endl;
            counter++;
        }
        std::cout << std::endl << counter << ". Cancel" << std::endl;
        std::cout << "Enter an option (0-" << counter << "): ";

        log_level = get_int_in_range(0, counter);
        if (log_level != counter) {
            memset(buffer, 0, BUF_LEN);
            len = sprintf(buffer, "Set Log Level=%d", log_level) + 1;
            buffer[len - 1] = '\0';

            pthread_mutex_lock(&lock_x);
            sendto(fd, (char *)buffer, len, 0, (struct sockaddr *)&rem_addr, rem_addr_len);
            pthread_mutex_unlock(&lock_x);
            std::cout << "The Log level was set to " << log_level_str[log_level] << std::endl;
        } else {
            std::cout << "Log level Set up was canceled" << std::endl;
        }
    } else {
        std::cout << "Logger is not connected. Try again later." << std::endl;
    }
}
/*Function to display the user menu and get choise from the input */
int uesr_menu() {
    std::cout << "1. Set the Log level" << std::endl;
    std::cout << "2. Dump the Log file" << std::endl;
    std::cout << "0. Shut Down" << std::endl;
    std::cout << "Enter an option (0-2): ";

    return get_int_in_range(0, 2);
}
/*Signal Handler is responsible for*/
/*handling SIGINT siganl */
void signal_handler(int signal) {
    switch (signal) {
        case SIGINT:
            is_running = false;
            exit(EXIT_SUCCESS);
            break;

        default:
            break;
    }
}
/* Exit Handler that is responsible for releasing locks */
/* and dynamically allocated memory */
void exit_handler(int ev, void *arg) {
    struct exit_data *data = (struct exit_data *)arg;
    pthread_join(data->tid_, NULL);
    pthread_mutex_destroy(&lock_x);
    close(data->fd_);
}