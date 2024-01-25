
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

/* reads and returns read nbytes on success, on failure or read of 0, returns -1 */
ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags) {

    ssize_t nbytes = 0;
    
    if (out_size != MAX_MSG) {
        fprintf(stderr, "read_socket: *out must be equal %d bytes, sized: %ld\n", MAX_MSG, out_size);
        return -1;
    }

    out[MAX_MSG] = '\0'; // set limits before reading
    nbytes = recv(sockfd, out, (MAX_MSG - 1), flags);
 
    // TODO: ident rfc 512 - 2 = '\0' '\n' reserved
    // both on error and hangup, return -1 to close socket

    // ECONNRESET Connection reset by peer

    if (nbytes < 0) {
        perror("recv");
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    printf("received %ld bytes\n", nbytes);

    return nbytes;
}




/* copies *in and sends it, returns sent nbytes on success, on failure returns -1 */
ssize_t send_socket(int sockfd, char *in, int flags) {
    
    ssize_t nbytes = 0;
    char sent[MAX_MSG] = {0};
    sent[MAX_MSG] = '\0';

    strncat(sent, in, MAX_MSG - 1);        // TODO: MAX_MSG - 2 Enforce \n
                                            // TODO: strip user added \n's
    size_t sent_size = strlen(sent);
    nbytes = send(sockfd, sent, sent_size, flags);

    if (nbytes < 0) {
        perror("send");
        return -1;
    }

    printf("sent %ld bytes\n", nbytes);

    return nbytes;
}




int close_socket(struct Node *client, struct server *srv) {

    fprintf(stdout, "closing socket %d\n", client->connfd);
    
    struct epoll_event ee = {0};
    int ret = epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, client->connfd, &ee);

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

/* broadcasts to all active sockets, except to sender, sender may be NULL */
int broadcast(struct server *srv, struct Node *sender, char *msg) {


    /* 

        TODO: displaying sender name or server if null
        TODO: const char *msg fix in read and send
        TODO: what does broadcast return??

    */

    printf("broadcasting\n");

    struct Node *client = NULL;
    struct Node *head = srv->clients->head;
    
    assert(head != NULL && "srv->clients->head is NULL, this should never happen");

    for (client = head; client != NULL; client = client->next) {
        printf("connection %d on %s\n", client->connfd, client->nickname);

        if (client == sender) 
            continue; 

        // TODO: send sender name, along with message here
        send_socket(client->connfd, msg, 0);

    }

    return 0;
}


/* returns the sockaddr_in pointer of the specified sa_family */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/* returns an ipstr struct that contains the presentation string converted ipaddress and port */
struct ipstr get_ip_str(struct sockaddr *sa) {
     
    
    struct sockaddr_in *addr_in = (struct sockaddr_in *)get_in_addr((struct sockaddr *)sa);
    struct ipstr str = {0};
     
    const char *paddress = NULL;
    int status = -1;     

    // TODO: add ipv6 support here, or in another function?

    assert(addr_in != NULL);

    paddress = inet_ntop(
        AF_INET, 
        &(addr_in->sin_addr),
        str.address,
        INET_ADDRSTRLEN
    );

    if (paddress == NULL) {
        perror("inet_ntop");
        exit(1);
    }

    // PORT_MAX will be -1 the sizeof
    int port = ntohs(addr_in->sin_port);
    status = snprintf(str.port, MAX_PORT_LEN, "%d", port);

    if (status < 0) {
        perror("snprintf");
    }

    return str;
}

// TODO: refactor to give svopts a more descriptive name,
//          srvconf,    srvopts,    srvsett,    srv
int init_server(struct server *srv, struct serveropts *svopts) {

    fprintf(stdout, "initializing server...\n");

    srv->clients = create_list();

	/* create epoll */
    srv->epollfd = epoll_create(16);

	if (srv->epollfd < 0) {
		perror("epoll_create");
        return -1;
    }

	/* create socket */
    srv->sockfd = socket(svopts->family, SOCK_STREAM, 0);
	fcntl(srv->sockfd, F_SETFL, O_NONBLOCK);

	if (srv->sockfd < 0) {
		perror("socket");
        return -1;
    }

	/* set socket options */
	const int enabled = 1;
	if (setsockopt(srv->sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int)) < 0) {
		perror("setsockopt");
        return -1;
    }

    /* It may be important to zero out the padding bytes using memset, instead of = {0};
         when writing to external storage, network or comparing with memcmp */
    memset(&srv->saddr, 0, sizeof(struct sockaddr_storage));

    switch (svopts->family) {

        case AF_INET:

            // get sockaddr_in from srv->saddr (sockstorage)
            struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr(
                (struct sockaddr *)&srv->saddr); 

            addrinfo->sin_family = AF_INET;
            addrinfo->sin_addr.s_addr = htonl(INADDR_ANY);
            addrinfo->sin_port = htons(svopts->port);

            int bind_status = bind(srv->sockfd, (struct sockaddr *)addrinfo, sizeof(*addrinfo));

            if (bind_status < 0) {
                perror("bind");
                return -1;
            }
        
            struct ipstr ipaddr = get_ip_str((struct sockaddr *)&srv->saddr);
            fprintf(stdout, "Server Listening on %s:%s\n", ipaddr.address, ipaddr.port);

            break;

        case AF_INET6:
            fprintf(stderr, "IPv6 is currently not supported.\n");
            exit(-1);

            break;  // Never going to be supported loool!

    }

    if (listen(srv->sockfd, svopts->backlog) < 0) {
        perror("listen");
        return -1;
    }

    // getaddrinfo is the most retarded function ever created.
    add_epoll_watch(srv->epollfd, srv->sockfd, srv->sockfd, EPOLLIN);

    return 0;
}



void poll_server(struct server *srv, struct serveropts *svopts, int wait) {

    struct epoll_event ee = {0};

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
  
        struct ipstr ipaddr = get_ip_str((struct sockaddr *)&client->caddr); 
        //struct ipstr ipaddr = get_ip_str((struct sockaddr *)&srv->saddr);

        printf("Received new connection from %s:%s\n", ipaddr.address, ipaddr.port);
        
        add_epoll_watch(srv->epollfd, client->connfd, client, (EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP));
        // TODO: send connected message
        // TODO: implement colors with xmacros

        // EPOLLIN  - ready to read
        // EPOLLOUT - ready to write

        return;
    }

    /* Data arriving on an already-connected socket */
    if (ee.events & EPOLLIN) {

        struct Node *client = ee.data.ptr;
        assert(client != NULL && "read requested client was NULL");

        char msg_buffer[MAX_MSG] = {0};
        
        /* an error occured */
        if (read_socket(client->connfd, msg_buffer, sizeof(msg_buffer), 0) < 0) {

            #define CLIENT_DISCON "client has left\n"

            broadcast(srv, NULL, CLIENT_DISCON);
            if (close_socket(client, srv) < 0)
                fprintf(stderr, "Failure to close client socket: %s\n", strerror(errno));
            
        } else {
            printf("\nServer: %s\n", msg_buffer);
 
            if (svopts->verbose)    // vvvv turn this into a macro
            fprintf((svopts->logfile == NULL) ? (stdout) : (svopts->logfile), 
                    "<host %s:%u sent %ld byte(s): [%s]>\n",
                    "hostname",
                     1234,
                     sizeof(msg_buffer),
                     msg_buffer);

            broadcast(srv, client, msg_buffer);
        }
                
            // read hangup?
    } else if (ee.events & (EPOLLRDHUP | EPOLLHUP)) {
        printf("closing connection triggered\n");

        // whenever SEND_SHUTDOWN or RCV_SHUTDOWN are marked, this is called.
        // which is equal to a call of shutdown(SHUT_WR | SHUT_RD)
        //https://stackoverflow.com/questions/52976152/tcp-when-is-epollhup-generated

        return;
    }







}


void shutdown_server(struct server *srv) { 

    #define SERVER_SHUTDOWN "Server shutting down...\n"

    if (srv->clients->head != NULL && srv->clients->capacity >= 1) {

        broadcast(srv, NULL, SERVER_SHUTDOWN);
    }

    delete_list(srv->clients);  // ALWAYS delete clients

    // man 3p shutdown 

    close(srv->sockfd);
    close(srv->epollfd);

}


