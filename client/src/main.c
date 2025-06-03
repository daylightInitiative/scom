
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

#define MAX_MSG 512

// goal: connect to the server
// setup commands

volatile sig_atomic_t stop;

void sigint_handler(int signo) {
    //fprintf(stdout, "\n caught sigint\n exiting.\n");
    stop = 1;

    // Close stdin to force fgets to return
    fclose(stdin);

    write(STDOUT_FILENO, "\n[!] SIGINT received. Exiting...\n", 33);
}

#define MAX_PORT_LEN 10 // 65535

struct ipstr {
    char address[INET_ADDRSTRLEN];
    char port[MAX_PORT_LEN + 1];
};

/* returns the sockaddr_in pointer of the specified sa_family */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* returns an ipstr struct that contains the presentation string converted ipaddress and port */
struct ipstr get_ip_str(struct sockaddr_storage *ss) {
    struct sockaddr_in *addr_in = (struct sockaddr_in *)get_in_addr((struct sockaddr *)ss);
    struct ipstr str = {0};
    
    const char *paddress = NULL;
    int status = -1;
    uint16_t port = 0;

    assert(ss != NULL);

    if (ss->ss_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)ss;
        paddress = inet_ntop(AF_INET, &(addr_in->sin_addr), str.address, sizeof(str.address));
        port = ntohs(addr_in->sin_port);
    } else if (ss->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ss;
        paddress = inet_ntop(AF_INET6, &(addr_in6->sin6_addr), str.address, sizeof(str.address));
        port = ntohs(addr_in6->sin6_port);
    } else {
        fprintf(stderr, "Unknown address family: %d\n", ss->ss_family);
        exit(EXIT_FAILURE);
    }

    if (paddress == NULL) {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    printf("coming from port %d\n", port);

    status = snprintf(str.port, MAX_PORT_LEN, "%u", port);
    if (status < 0 || status >= MAX_PORT_LEN) {
        fprintf(stderr, "snprintf error or truncation\n");
    }

    return str;
}

/* recv's into *out, returns recv'd nbytes on success, on failure/disconnect -1, on timeout -2 */
ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags) {
    if (out == NULL || out_size == 0) {
        fprintf(stderr, "read_socket: invalid buffer\n");
        return -1;
    }


    memset(out, 0, out_size);

    ssize_t nbytes = recv(sockfd, out, out_size - 1, flags);
    if (nbytes < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return -2; // timeout
        if (errno == ECONNRESET) return -1; // disconnect
        perror("recv");
        return -1;
    }

    // cut the string off (we read in out_size - 1)
    out[nbytes] = '\0';

    printf("received %ld bytes on socket %d\n", nbytes, sockfd);
    return nbytes;
}



/* copies string literal *in and sends it, returns sent nbytes on success, on failure returns -1 */
ssize_t send_socket(int sockfd, char *in, int flags) {
    
    ssize_t nbytes = 0;
    char sent[MAX_MSG];
    
    memset(sent, '\0', MAX_MSG);
    //strncat(sent, in, MAX_MSG - 1);        // TODO: MAX_MSG - 2 Enforce \n
                                            // TODO: strip user added \n's

    // instead of strncat use a more secure function like snprintf()
    int written = snprintf(sent, MAX_MSG, "%s", in);
    if (written < 0 || written >= MAX_MSG) {
        fprintf(stderr, "snprintf errored or truncation\n");
        return -1;
    }

    size_t sent_size = strlen(sent);
    nbytes = send(sockfd, sent, sent_size, flags);

    if (nbytes < 0) {

        if (errno == ECONNRESET) { 
            printf("conn %d: forcibly closed the connection\n", sockfd);
        } else {
            perror("send");
        }
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    printf("sent %ld bytes to %d\n", nbytes, sockfd);
    printf("raw: %s", sent);

    return nbytes;
}

struct clientopts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     
    struct sockaddr_storage caddr; // TODO: migrate to a caddr for ipv4/ipv6 full support

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

        if (activity < 0) {
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
                    printf("Failure to send_socket\n");
                }
            }
        }

        // Handle timeout
        if (activity == 0) {
            //printf("No activity, timeout reached.\n");
        }
    }

    printf("closing socket\n");

    close(sockfd);

    return 0;
}