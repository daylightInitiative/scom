
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

/* returns nbytes read on success, on failure or client hangup, returns -1 */
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

    if (nbytes < 0) {
        perror("recv");
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    return nbytes;
}

/* returns nbytes sent on success, on failure returns -1 */
size_t send_socket(int sockfd, char *in, size_t in_size, int flags) {
    
    size_t nbytes = 0;

    // TODO: checking if a socket is still open
    if (in_size != MAX_MSG) {
        fprintf(stderr, "send_socket: *in must be equal to %d bytes, sized: %ld\n", MAX_MSG, in_size);
        return -1;
    }

    in[MAX_MSG] = '\0'; // should this be here? or should *in be const
    nbytes = send(sockfd, in, in_size, flags);

    /* handle send errors here for specific cases */
    if (nbytes < 0) {
        perror("send");
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

    /* close may fail if something goes horribly wrong */
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

    /* lets implment this sqii

        1. loop over the server struct, until null
        2. send fixed amount of data to each socket (512)
        
        TODO: displaying sender name or server if null
        TODO: const char *msg fix in read and send


    */

    printf("broadcasting\n");

    struct Node *client = NULL;
    struct Node *head = srv->clients->head;
    
    //assert(head != NULL && "srv->clients->head is NULL, this should never happen");

    /* send a message from x to everybody else excluding x */
    for (client = head; client != NULL; client = client->next) {
        printf("connection %d on %s\n", client->connfd, client->nickname);

        if (client == sender)   // sender may be null to indicate "Server: msg"
            continue; 
            
        size_t len = strlen(msg);

        //send(client->connfd, msg, len, 0);
        // TODO: send sender name, along with message here
        send_socket(client->connfd, msg, MAX_MSG, 0);

    }

}


/* returns the sockaddr_in pointer of the specified sa_family */
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

	// TODO:
	// created and initialized socket, now to create the
	// serveraddr struct that specifies our bind interface
	// 		If svopts->family == AF_INET (manually set AF_INET only)
	//		If svopts->family == AF_INET6 
	// server can be both ipv4 and ipv6, otherwise if set manually do not

/*void unused() {
    switch (svopts->family) {

		case AF_INET:
			svopts->saddr = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in));
			svopts->saddr->sin_family = AF_INET;
			svopts->saddr->sin_port = htons(svopts->port);
			svopts->saddr->sin_addr.s_addr = htonl(INADDR_ANY);

		case AF_INET6:	// IPV6ONLY 
			break;

		default:	// AF_UNSPEC use getaddrinfo() 
			break;
	}*/

	/*
	svopts->saddr = (struct sockaddr_storage *)calloc(1, sizeof(struct sockaddr_storage));
	svopts->saddr->sin_family = AF_INET;
	svopts->saddr->sin_port = htons(svopts->port);
	svopts->saddr->sin_addr.s_addr = INADDR_ANY; // htonl? (<hostname> OR 127.0.0.1 OR localhost)

    //struct sockaddr_in *info = (struct sockaddr_in *)&client->caddr;
    //struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr((struct sockaddr *)&client->caddr);

    // create srv->saddr depending on internet protocol version
    switch (svopts->family) {

        case AF_INET:       // IPv4 only 

            //struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr(
                (struct sockaddr *)srv->saddr); // &srv->saddr

            addrinfo->sin_family = AF_INET;
            addrinfo->sin_port = htons(svopts->port);
            addrinfo->sin_addr.s_addr = htonl(INADDR_ANY);

            // allocate heap memory for saddr
            srv->saddr = (struct sockaddr_storage *)calloc(1, sizeof(srv->saddr));

            if (srv->saddr == NULL) {
                // TODO: errorcodes in server.h, define them
                fprintf(stderr, "Failure to allocate server address\n");
                return -1;
            }

            // sockstorage is simply a union containing all sockaddr types
            // sockstorage must be cast to a sockaddr * and then fed to get_in_addr 
            //   to finally return the sockaddr_in ipv4 address

            (struct sockaddr *)srv->saddr->ss_family = AF_INET;     // for sure this is ipv4.
            struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr(
                (struct sockaddr *)srv->saddr); // &srv->saddr

            addrinfo->sin_family = AF_INET;
            addrinfo->sin_port = htons(svopts->port);
            addrinfo->sin_addr.s_addr = htonl(INADDR_ANY);

            break;

        case AF_INET6:
            fprintf(stderr, "IPv6 currently not supported.\n");
            exit(1);
            break;

        default:
            break;
    }}*/

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

        /* used later for remove */
        //client->srv = srv;

        /*
        char sent[MAX_MSG] = {0};
        sent[MAX_MSG - 1] = '\0';

        strncat(sent, "\x0c3IDENT", MAX_MSG - 1);

        printf("connfd: %d\n", client->connfd);
        ssize_t sbytes = send(client->connfd, sent, MAX_MSG, 0);
        if (sbytes < 0) {
            perror("send");
            return;
        }*/

        // note: we cannot use recv here because the socket is not ready to read.
        // note: we must call all reading functions in & EPOLLIN

        // TODO: send connected message
        // TODO: implement colors with xmacros

        return;
    }

    /* Data arriving on an already-connected socket */
    if (ee.events & EPOLLIN) {
        struct Node *client = ee.data.ptr;
        assert(client != NULL && "read requested client was NULL");

        char msg_buffer[MAX_MSG] = {0}; // array of characters [0] -> first character
        
        /* an error occured */
        if (read_socket(client->connfd, msg_buffer, sizeof(msg_buffer), 0) < 0) {

            #define CLIENT_DISCON "client has left\n"     // predefined format?
            // Server: "Jim" has left
            // TODO: use vfprintf, or fprintf on a buffer??

            // because we are crafting froma string literal, a literal needs a literal
            // newline.

            char message[MAX_MSG] = {0};
            message[MAX_MSG] = '\0';   // overwrite the \n character with a null

            strncat(message, CLIENT_DISCON, MAX_MSG - 1); // copy up until the last char

            broadcast(srv, NULL, message);
            if (close_socket(client, srv) < 0)
                fprintf(stderr, "Failure to close client socket: %s\n", strerror(errno));
            
        } else {
            printf("\nsized: %ld\nServer: %s\n", sizeof(msg_buffer), msg_buffer);

            // TODO: package getting the human readable address and port in function
            
            if (svopts->verbose)    // vvvv turn this into a macro
            fprintf((svopts->logfile == NULL) ? (stdout) : (svopts->logfile), 
                    "<host %s:%u sent %ld byte(s): [%s]>\n",
                    "hostname",
                     1234,
                     sizeof(msg_buffer),
                     msg_buffer);

            broadcast(srv, client, msg_buffer);
            
        }


    }






}


void shutdown_server(struct server *srv) { 

    #define SERVER_SHUTDOWN "Server shutting down...\n"

    if (srv->clients->head != NULL && srv->clients->capacity > 0) {

        // TODO: display shutdown message
        char message[MAX_MSG] = {0};
        message[MAX_MSG] = '\0';

        // streamline this buffering process in broadcast?
        strncat(message, SERVER_SHUTDOWN, MAX_MSG - 1); // copy up until the last char
        broadcast(srv, NULL, message);

    }

    // if there are ( this is more suited to be an internal optimization to list.c
    /*
    if (srv->clients->capacity > 0) {
        
    }*/

    delete_list(srv->clients);

    // man 3p shutdown 

    close(srv->sockfd);
    close(srv->epollfd);

}


