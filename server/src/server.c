
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

#include <netdb.h>

#include "server.h"
#include "list.h"

// https://github.com/yzziizzy/git_webstack/blob/master/src/net.c#L15
void add_epoll_watch(int epollfd, int fd, void* data, int events) {
    struct epoll_event ee = {0};

    ee.events = events;
    ee.data.ptr = data;
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ee);
    if (ret < 0) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}



ssize_t read_socket(int sockfd, char *out, size_t size, int flags) {

    ssize_t nbytes;

    if (size != MAX_MSG) {
        fprintf(stderr, "read_socket: *out must be equal 512 bytes, sized: %ld\n", size);
        return -1;
    }

    out[MAX_MSG] = '\0'; // set limits before reading
    nbytes = recv(sockfd, out, (MAX_MSG - 1), flags);
 
    // TODO: ident rfc 512 - 2 = '\0' '\n' reserved
    // both on error and hangup, return -1 to close socket

    if (nbytes < 0) {
        perror("recv");
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    return nbytes;
}



int close_socket(struct Node *client, struct server *srv) {

    fprintf(stdout, "closing socket %d\n", client->connfd);
    
    struct epoll_event ee = {0};
    int ret = epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, client->connfd, &ee); // we need to attach srv to client struct

    if (ret < 0) {
        perror("epoll_ctl");
        return -1;
    }

    
    if (close(client->connfd) < 0) {
        perror("close");
        return -1;
    }

    remove_node(srv->clients, client);
    echo_list(srv->clients);

    return 0;
}


int broadcast(struct server *srv, const char *msg) {

    
    
}



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int init_server(struct server *srv, struct serveropts *svopts) {

    fprintf(stdout, "initializing server...\n");

    srv->clients = create_list();

    
    srv->epollfd = epoll_create(16);

    if (srv->epollfd < 0) {
        perror("epoll_create");
        return -1;
    }

    
    srv->sockfd = socket(svopts->family, SOCK_STREAM, 0);
    fcntl(srv->sockfd, F_SETFL, O_NONBLOCK);

    if (srv->sockfd < 0) {
        perror("socket");  
        return -1;
    }

    
    const int enabled = 1;
    if (setsockopt(srv->sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int)) < 0) {
        perror("setsockopt");
        return -1;
    }

    // TODO:
    // created and initialized socket, now to create the
    // serveraddr struct that specifies our bind interface
    //      If svopts->family == AF_INET (manually set AF_INET only)
    //      If svopts->family == AF_INET6 
    // server can be both ipv4 and ipv6, otherwise if set manually do not



    

    //saddr.sin_family = AF_INET;
    //saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //saddr.sin_port = htons(HOSTPORT);


    memset(&srv->saddr, 0, sizeof(struct sockaddr_storage));

    switch (svopts->family) {

        case AF_INET:

            // get sockaddr_in
            // fill values
            struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr(
                (struct sockaddr *)&srv->saddr); 

            addrinfo->sin_family = AF_INET;
            addrinfo->sin_addr.s_addr = htonl(INADDR_ANY);
            addrinfo->sin_port = htons(svopts->port);

            // what type is srv->saddr
            // may be logically wrong, need to get sockaddr_in for this size
            if (bind(srv->sockfd, (struct sockaddr *)addrinfo, sizeof(*addrinfo)) < 0) {
                perror("bind");
                return -1;
            }

            char address[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET,
                addrinfo,
                address,
                sizeof(address)
            ); 

            fprintf(stdout, "Server Listening on %s:%u\n", inet_ntoa(addrinfo->sin_addr), svopts->port);

            break;

        case AF_INET6:
            fprintf(stderr, "IPv6 is currently not supported.\n");
            exit(-1);

            break;  // Never going to be supported loool!

    }

    printf("sized: %ld", sizeof(srv->saddr)); 

    if (listen(srv->sockfd, svopts->backlog) < 0) {
        perror("listen");
        return -1;
    }

    // getaddrinfo is the most retarded function ever created.
    //add_epoll_watch(epollfd, sockfd, sockfd, EPOLLIN);
    add_epoll_watch(srv->epollfd, srv->sockfd, srv->sockfd, EPOLLIN);

    return 0;


}



void poll_server(struct server *srv, struct serveropts *svopts, int wait) {

    struct epoll_event ee = {0};
  
    //printf("epoll tick:\n");

    int ret = epoll_wait(srv->epollfd, &ee, 1, wait);

    if (ret == 0) return;

    // New connection
    if (ee.data.fd == srv->sockfd) {
        struct Node *client = insert_node(srv->clients);
        echo_list(srv->clients);

        memset(&client->caddr, '\0', sizeof(client->caddr));
        socklen_t addrlen = sizeof(client->caddr);  // I didn't realize this was important read manual harder

        client->connfd = accept(srv->sockfd, (struct sockaddr *)&client->caddr, &addrlen);
        fcntl(client->connfd, F_SETFL, O_NONBLOCK);

        printf("new connection established: %d, %p\n", client->connfd, &client->caddr);

        struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr((struct sockaddr *)&client->caddr); 
        char address[INET_ADDRSTRLEN] = {0};
        
        inet_ntop(client->caddr.ss_family,
            //get_in_addr((struct sockaddr *)&client->caddr),
            addrinfo,
            address,
            sizeof address); 

        // simple addrinfo
        struct sockaddr_in *info = (struct sockaddr_in *)&client->caddr;
        int port = ntohs(info->sin_port);        // converting network to our host byte order (16 for port)
        printf("Received new connection from %s:%d\n", address, port);
        
        add_epoll_watch(srv->epollfd, client->connfd, client, (EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP));

        
        //client->srv = srv;

        

        // note: we cannot use recv here because the socket is not ready to read.
        // note: we must call all reading functions in & EPOLLIN

        return;
    }

    
    if (ee.events & EPOLLIN) {
        struct Node *client = ee.data.ptr;

        char msg_buffer[MAX_MSG] = {0}; // array of characters [0] -> first character
        
        
        if (read_socket(client->connfd, msg_buffer, sizeof(msg_buffer), 0) < 0) {

            if (close_socket(client, srv) < 0)
                fprintf(stderr, "Failure to close client socket: %s\n", strerror(errno));
            
        } else {
            printf("\nsized: %ld\nServer: %s\n", sizeof(msg_buffer), msg_buffer);
            
            if (svopts->verbose)    // vvvv turn this into a macro
            fprintf((svopts->logfile == NULL) ? (stdout) : (svopts->logfile), 
                    "<host %s:%u sent %ld byte(s): [%s]>\n",
                    "hostname",
                     1234,
                     sizeof(msg_buffer),
                     msg_buffer);
            
        }


    }






}





