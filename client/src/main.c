
#include <assert.h>

#include <errno.h>
#include <stdio.h>

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

    // Close stdin to force fgets to return
    fclose(stdin);

    logfmt(stderr, WARN, "[!] SIGINT received. Exiting...\n");
}




int main(int argc, char **argv) {

    signal(SIGINT, sigint_handler);

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
    struct clientopts cliopts;
    memset(&cliopts, '\0', sizeof(cliopts));
    cliopts.port = 4444;

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

    while (!stop) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds); // Set socket fd for reading
        FD_SET(STDIN_FILENO, &readfds); // Set stdin for reading

        // Wait for data on either socket or stdin
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
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