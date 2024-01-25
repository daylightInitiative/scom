
#ifndef SCOM_SERVER_H
#define SCOM_SERVER_H

#include <netinet/in.h>
#include "list.h"

#define HOSTADDR "127.0.0.1" //"127.0.0.1" // localhost INADDR_ANY
#define HOSTPORT 4444


#define MAX_PORT_LEN 10 // 65535

struct ipstr {
    char address[INET_ADDRSTRLEN];  // inet_pton
    char port[MAX_PORT_LEN + 1];        // snprintf
};

// TODO: error handle connection reset by peer (what triggers this error?)
// TODO: how to get the real ipv4 address
// TODO: how can we listen on 192.168.1.69
    // serve on 127.0.0.1 nmap
    // serve on 192.168.1.69 nmap

#define MAX_MSG      512
#define MAX_CLIENTS  24

struct serveropts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     

    int backlog;            // maximum queued connections

    FILE *logfile;          // can be stdin, file, or if UNSPEC: (syslog)
    int verbose;            // verbosity enabled
};

struct server {
    
    int sockfd;
    int epollfd;

    struct sockaddr_storage saddr;
    struct List *clients;
};

int init_server(struct server *srv, struct serveropts *svopts);
void poll_server(struct server *srv, struct serveropts *svopts, int wait);
void shutdown_server(struct server *srv);

#endif
