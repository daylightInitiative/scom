
#include <assert.h>

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <signal.h>
#include <time.h>
#include <netdb.h>

#include "client.h"
#include "log.h"



// goal: connect to the server
// setup commands

volatile sig_atomic_t stop;

void sigint_handler(int signo) {
    stop = 1;

    // TBD fclose or non blocking fgets
    // fclose(stdin);

    write(STDOUT_FILENO, "\n[!] SIGINT received. Exiting...\n", 33);
}

void usage(int status) {

    puts("\nUsage: scom [ OPTIONS... ] [ FILE... ]\n");
    puts("\nOptions: ");
    puts(" -4\tforces scomd to use IPv4 addresses only.");
    puts(" -6\tforces scomd to use IPv6 addresses only.");
    puts(" -e\twrite debug logs to standard error instead of the system log.");
    puts(" -p\tspecify what port to listen on");
    puts(" -h\tdisplay this help message");
    puts(" -v\tproduce more verbose output");

    exit(status);
}

int parse_network_args(int argc, char **argv, struct clientopts *svopts) {

    memset(svopts, 0, sizeof(struct clientopts));

    int c;

    /* getopts:

        -4      support only ipv4 connections
        -6      support only ipv6 connections
        -E      write debug logs to selected file instead of stderr
        -e      write debug logs to standard error instead of the system log.
        -p      set server to listen on <PORT>
        -v      set higher verbosity
        -h      displays this help message


    */

    svopts->verbose = false;
    svopts->family = AF_INET; // change to AF_UNSPEC later?
    svopts->port = HOSTPORT;
    svopts->logfile = NULL;

    while ((c = getopt(argc, argv, "46vheE:p:")) != -1) {

        switch (c) {

        case '4':
            svopts->family = AF_INET;
            break;

        case '6':
            svopts->family = AF_INET6;
            break;

        case 'E':
            fprintf(stdout, "Logging to %s\n", optarg);
                // TODO check access(), if not stdin, close(logfile) [verify file]

                /*  TODO add logging library, set log mode by verbosity
                // TODO echo to stdout and logfile bitmask
                if (!file_exists) {
                    svopts->logfile = (FILE *)fopen(optarg, "w");

                    if (svopts->logfile 
                }*/

                // lets say i want to log to stderr and STREAM, its quite a common task isnt it

            break;

        case 'e':
            svopts->logfile = stderr;
            break;

        case 'v':
            svopts->verbose = 1;
            break;

        case 'h':
            usage(EXIT_SUCCESS);
            break;

        case 'p':

            printf("custom port selected\n");
                // TODO atoi fails, returns 0, no way to distingush err
                // use strtol for increased error output

            int port = atoi(optarg);
            printf("port: %u\n", port);

            // TODO binding on port 0 returns a random port number, make this an option?
            if (port == 0) {
                fprintf(stderr, "Invalid port number supplied\n");
                return -1;
            }

            // connecting doesnt necessarily need root
            svopts->port = port;

            break;

        case '?':
            usage(EXIT_FAILURE);
            break;

        default:
            exit(EXIT_FAILURE);
        }
    }

    /* remove getopt options */
    argc -= optind;
    argv += optind;

    /* . . . */

    return 0;
}


int main(int argc, char **argv) {

    signal(SIGINT, sigint_handler);

    struct clientopts cliopts = {0};

    int ret = parse_network_args(argc, argv, &cliopts);

    if (ret < 0) {
        fprintf(stderr, "Failure to parse network arguments\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = -1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        logfmt(stderr, ERROR, "Failure to create socket\n");
        perror("socket");
        exit(1);
    }

    /* set socket options */
    struct timeval tv;
    tv.tv_sec = 3;  // seconds
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in srvaddr;
    memset(&srvaddr, '\0', sizeof(srvaddr));

    // allocate client opts to be filled in by cla
    srvaddr.sin_family = AF_INET; // for right now ipv4 is hard coded later will be getopts
    srvaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srvaddr.sin_port = htons(cliopts.port);

    memcpy(&cliopts.caddr, &srvaddr, sizeof(srvaddr));
    cliopts.caddr.ss_family = AF_INET;

    // manually fill the options

    struct ipstr ipinfo = get_ip_str(&cliopts.caddr);
    logfmt(stdout, INFO, "Attempting to connect to %s:%s", ipinfo.address, ipinfo.port);

    socklen_t addrlen = sizeof(srvaddr);
    int status = -1;
    status = connect(sockfd, (struct sockaddr *)&srvaddr, addrlen);
    if (status < 0) {
        logfmt(stderr, ERROR, "Failed to connect\n");
        perror("connect");
        exit(1);
    }

    logfmt(stdout, DEBUG, "Connected successfully.");
    // now that we have a connected socket we need to loop

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 2;  // Set timeout value
    timeout.tv_usec = 0;

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    //Set stdin flag;s to non blocking, so we can break the select call

    while (!stop) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds); // Set socket fd for reading
        FD_SET(STDIN_FILENO, &readfds); // Set stdin for reading

        // Wait for data on either socket or stdin
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR && errno != EAGAIN) {
            perror("select");
            stop = 1;
            break;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            // Handle reading from the socket (server data)
            char msg_buffer[MAX_MSG] = {0};
            int n = read_socket(sockfd, msg_buffer, MAX_MSG, 0);
            if (n > 0) {
                printf("Server: %s\n", msg_buffer);
            }
            else if (n == 0) {
                printf("Server closed the connection.\n");
                break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // Handle user input from stdin
            char send_buffer[MAX_MSG] = {0};
            if (fgets(send_buffer, MAX_MSG, stdin) != NULL) {
                ssize_t bytes_sent = send_socket(sockfd, send_buffer, 0);
                if (bytes_sent < 0) {
                    logfmt(stderr, ERROR, "Failure to send_socket");
                }
            }
        }

        // Handle timeout
        if (activity == 0) {
            //printf("No activity, timeout reached.\n");
        }
    }

    logfmt(stdout, INFO, "Closing socket...");

    close(sockfd);

    return 0;
}