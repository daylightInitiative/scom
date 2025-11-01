
#ifndef SCOM_SERVER_H
#define SCOM_SERVER_H

#include <netinet/in.h>
#include "list.h"

#define HOSTADDR "127.0.0.1" //"127.0.0.1" // localhost INADDR_LOOPBACK
#define HOSTPORT 4444

// protocol codes
#define SERVER_SHUTDOWN "SERVER_SHUTDOWN\n"

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
