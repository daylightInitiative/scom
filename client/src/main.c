
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

#include <signal.h>
#include <time.h>
#include <netdb.h>

#define MAX_MSG 512

// goal: connect to the server
// setup commands

volatile sig_atomic_t stop;

void sigint_handler(int signo) {
    fprintf(stdout, "\n caught sigint\n exiting.\n");
    stop = 1;
}

/* recv's into *out, returns recv'd nbytes on success, upon failure to read of 0, returns -1 */
ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags) {

    ssize_t nbytes = 0;
    
    if (out_size != MAX_MSG) {
        fprintf(stderr, "read_socket: *out must be equal %d bytes, sized: %ld\n", MAX_MSG, out_size);
        return -1;
    }

    memset(out, '\0', MAX_MSG);
    nbytes = recv(sockfd, out, (MAX_MSG - 1), flags);
 
    // TODO: reserve -1 and -2 for the \n and \0
    // both on error and hangup, return -1 to close socket

    // ECONNRESET Connection reset by peer

    if (nbytes < 0) {
        perror("recv");
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    printf("received %ld bytes on %d\n", nbytes, sockfd);

    return nbytes;
}



/* copies string literal *in and sends it, returns sent nbytes on success, on failure returns -1 */
ssize_t send_socket(int sockfd, char *in, int flags) {
    
    ssize_t nbytes = 0;
    char sent[MAX_MSG];
    
    memset(sent, '\0', MAX_MSG);
    strncat(sent, in, MAX_MSG - 1);        // TODO: MAX_MSG - 2 Enforce \n
                                            // TODO: strip user added \n's
    size_t sent_size = strlen(sent);
    nbytes = send(sockfd, sent, sent_size, flags);

    if (nbytes < 0) {

        if (nbytes == ECONNRESET) { 
            printf("conn %d: forcibly closed the connection\n", sockfd);
        } else {
            perror("send");
            return -1;
        }
    } else if (nbytes == 0) {
        return -1;
    }

    printf("sent %ld bytes to %d\n", nbytes, sockfd);
    printf("raw: %s", sent);

    return nbytes;
}

// https://www.beej.us/guide/bgnet/html/#cb79-32
int recvtimeout(int s, char *buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return recv(s, buf, len, 0);
}

struct clientopts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     

    FILE *logfile;          // can be stdin, file, or if UNSPEC: (syslog)
    int verbose;            // verbosity enabled
};


int main(int argc, char **argv) {

    signal(SIGINT, sigint_handler);

    int sockfd = -1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if (sockfd < 0) {
        fprintf(stderr, "Failure to create socket\n");
        exit(1);
    }

    struct sockaddr_in srvaddr;
    memset(&srvaddr, '\0', sizeof(srvaddr));

    // allocate client opts to be filled in by cla
    struct clientopts cliopts;
    memset(&cliopts, '\0', sizeof(srvaddr));
    cliopts.port = 4444;

    // manually fill the options

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srvaddr.sin_port = htons(cliopts.port);

    // connection read write loop here

    socklen_t addrlen = sizeof(srvaddr);

    int status = -1;
    status = connect(sockfd, (struct sockaddr *)&srvaddr, addrlen);
    if (status < 0) {
        fprintf(stderr, "Failed to connect\n");
        exit(1);
    }

    // now that we have a connected socket we need to loop

    while (!stop) {
        char msg_buffer[MAX_MSG] = {0};
        memset(&msg_buffer, '\0', sizeof(msg_buffer));

        int n = recvtimeout(sockfd, msg_buffer, MAX_MSG-1, 2); // 4 second timeout

        if (n == -1) { // on interrupts ignore this 
            // error occurred
            //perror("recvtimeout");
            stop = 1;

            if (errno != EINTR)
                perror("recvtimeout");
            //printf("Interrupt\n");
        } else if (n == -2) {
            // timeout occurred
            //printf("Missed recvtimeout\n");
        } else {
            // got some data in buf
            printf("%s\n", msg_buffer);
        }

        // now if the user has input anything send it
        char send_buffer[MAX_MSG];
        memset(send_buffer, '\0', sizeof(send_buffer));

        // read into with fgets
        char *read = NULL;

        if (!stop) {
            if ((read = fgets(send_buffer, MAX_MSG, stdin)) != NULL) {
                // send to the server socket

                if (!stop) {
                    ssize_t bytes_sent = send_socket(sockfd, send_buffer, 0);
                    if (bytes_sent < 0) {
                        printf("Failure to send_socket\n");
                    }
                }
            }
        }


        
    }

    printf("closing socket\n");

    close(sockfd);

    return 0;
}