
#ifndef SCOMD_SERVER_H
#define SCOMD_SERVER_H

#include <netinet/in.h>
#include "list.h"
#include "log.h"

#define SERVER_EPOLL_DELAY 1000 //ms
#define HOSTADDR "127.0.0.1" //"127.0.0.1" // localhost INADDR_LOOPBACK
#define HOSTPORT 4444

// protocol codes
#define SERVER_GREETING "Welcome to the server\n"
#define SERVER_SHUTDOWN "SERVER_SHUTDOWN\n"

#define MAX_MSG      512
#define MAX_CLIENTS  24

struct serveropts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     

    int backlog;            // maximum queued connections

    LoggerConfig *loggerConfig;    // instead of using a file, lets set the config here
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

ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags);
ssize_t send_socket(int sockfd, char *in, int flags);
int close_socket(struct Node *client, struct server *srv);

int broadcast(struct server *srv, struct Node *sender, char *msg);
void add_epoll_watch(int epollfd, int fd, void *data, int events);

#endif
