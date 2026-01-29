#ifndef SCOM_CLIENT_H
#define SCOM_CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "log.h"

#define MAX_PORT_LEN 10 // 65535
#define HOSTPORT 4444



struct Clientopts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     

    LoggerConfig *loggerConfig;          // can be stdin, file, or if UNSPEC: (syslog)
    int verbose;            // verbosity enabled
};

struct Client {

    int serverfd; // the current server socket we are connected on
    struct sockaddr_storage caddr; // TODO: migrate to a caddr for ipv4/ipv6 full support
    struct timespec last_ping; // storing the time we sent the ping
    struct timespec last_pong; // for storing the last pong received

};

// func prototypes
void *get_in_addr(struct sockaddr *sa);
static inline void invalidate_timespec(struct timespec *ts) {
    ts->tv_sec  = -1;
    ts->tv_nsec = 0;
}

ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags);
ssize_t send_socket(int sockfd, char *in, int flags);

#endif