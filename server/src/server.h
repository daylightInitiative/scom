
#ifndef SCOM_SERVER_H
#define SCOM_SERVER_H

#include <netinet/in.h>
#include "list.h"

#define HOSTADDR "127.0.0.1"
#define HOSTPORT 5655

#define MAX_MSG      512
#define MAX_CLIENTS  24

/* serves as the user config from main */
struct serveropts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     

    int backlog;            // maximum queued connections

    FILE *logfile;          // can be stdin, file, or if UNSPEC: (syslog)
    int verbose;            // verbosity enabled
};

/* the main server object, holds server information */
struct server {
    
    int sockfd;
    int epollfd;

    struct sockaddr_storage saddr;
    struct List *clients;
};

int init_server(struct server *srv, struct serveropts *svopts);
void poll_server(struct server *srv, struct serveropts *svopts, int wait);

#endif
