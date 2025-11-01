
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
#include "../shared/net.h"
#include "list.h"

// https://github.com/yzziizzy/git_webstack/blob/master/src/net.c#L15
void add_epoll_watch(int epollfd, int fd, void *data, int events)
{
    struct epoll_event ee = {0};

    ee.events = events;
    ee.data.ptr = data;
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ee);
    if (ret < 0)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

int close_socket(struct Node *client, struct server *srv)
{

    fprintf(stdout, "closing socket %d\n", client->connfd);

    struct epoll_event ee = {0};
    int ret = epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, client->connfd, &ee);

    if (ret < 0)
    {
        perror("epoll_ctl");
        return -1;
    }

    if (close(client->connfd) < 0)
    {
        perror("close");
        return -1;
    }

    remove_node(srv->clients, client);
    echo_list(srv->clients);

    return 0;
}

/* broadcasts to all active sockets, except to sender, sender may be NULL */
int broadcast(struct server *srv, struct Node *sender, char *msg)
{

    /*

        TODO: displaying sender name or server if null
        TODO: const char *msg fix in read and send
        TODO: what does broadcast return??

    */

    printf("broadcasting\n");

    struct Node *client = NULL;
    struct Node *head = srv->clients->head;

    assert(head != NULL && "srv->clients->head is NULL, this should never happen");

    for (client = head; client != NULL; client = client->next)
    {
        printf("connection %d on %s\n", client->connfd, client->nickname);

        // if sender is NULL then we want to broadcast to everybody
        // if sender is NOT null we want to broadcast to everybody except sender

        if (sender == client)
        {
            continue;
        }

        send_socket(client->connfd, msg, 0);

        /*if (sender != NULL && client == sender) {
        printf("Triggered that one edge case\n");
            continue;
    }*/

        // TODO: send sender name, along with message here
        // send_socket(client->connfd, msg, 0);
    }

    return 0;
}

// TODO: refactor to give svopts a more descriptive name,
//          srvconf,    srvopts,    srvsett,    srv
int init_server(struct server *srv, struct serveropts *svopts)
{

    fprintf(stdout, "initializing server...\n");

    srv->clients = create_list();

    /* create epoll */
    srv->epollfd = epoll_create(16);

    if (srv->epollfd < 0)
    {
        perror("epoll_create");
        return -1;
    }

    /* create socket */
    srv->sockfd = socket(svopts->family, SOCK_STREAM, 0);
    fcntl(srv->sockfd, F_SETFL, O_NONBLOCK);

    if (srv->sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    /* set socket options */
    const int enabled = 1;
    if (setsockopt(srv->sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int)) < 0)
    {
        perror("setsockopt");
        return -1;
    }

    /* It may be important to zero out the padding bytes using memset, instead of = {0};
         when writing to external storage, network or comparing with memcmp */
    memset(&srv->saddr, 0, sizeof(struct sockaddr_storage));

    // it is important that we set what protocol we are using in the storage object
    srv->saddr.ss_family = svopts->family;

    switch (svopts->family)
    {

    case AF_INET:

        struct sockaddr_in *addrinfo = (struct sockaddr_in *)get_in_addr((struct sockaddr *)&srv->saddr);

        addrinfo->sin_family = AF_INET;
        addrinfo->sin_addr.s_addr = htonl(INADDR_ANY); // htonl (INADDR_ANY) is required here.
        addrinfo->sin_port = htons(svopts->port);

        int bind_status = bind(srv->sockfd, (struct sockaddr *)addrinfo, sizeof(*addrinfo));

        if (bind_status < 0)
        {
            perror("bind");
            return -1;
        }

        printf("server listening on %s\n", inet_ntoa(addrinfo->sin_addr));

        struct ipstr ipaddr = get_ip_str(&srv->saddr);
        fprintf(stdout, "Server Listening on %s:%s\n", ipaddr.address, ipaddr.port);

        socklen_t len = 0;
        struct sockaddr_storage srvaddr = {0};
        char ipstrs[INET_ADDRSTRLEN] = {0};
        int port = 0;

        printf("receiving sockname\n");
        len = sizeof(srvaddr);
        int ret = getsockname(srv->sockfd, (struct sockaddr *)&srvaddr, &len);

        if (ret < 0)
        {
            perror("getpeername");
        }

        if (srvaddr.ss_family == AF_INET)
        {
            printf("Connection normal\n");
            struct sockaddr_in *s = (struct sockaddr_in *)&srvaddr;
            port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ipstrs, sizeof(ipstrs));
        }
        else
        {
            // IPv6
            printf("Handling IPv6 connection\n");
        }

        // Instead of making two whole functions for both connected client and server
        // why dont we just have two wrappers over a DRY interface
        printf("=====================================\n");
        printf("Server sock name: %s:%d\n", ipstrs, port);

        // Lets rememeber that 0.0.0.0 means its listening from connections on all interfaces (ips)

        // INADDR_LOOPBACK (127.0.0.1)
        //         always refers to the local host via the loopback device;

        // INADDR_ANY (0.0.0.0)
        //       means any address for socket binding;

        break;

    case AF_INET6:
        fprintf(stderr, "IPv6 is currently not supported.\n");
        exit(-1);

        break; // Never going to be supported loool!
    }

    if (listen(srv->sockfd, svopts->backlog) < 0)
    {
        perror("listen");
        return -1;
    }

    // getaddrinfo is the most retarded function ever created.
    add_epoll_watch(srv->epollfd, srv->sockfd, srv->sockfd, EPOLLIN);

    return 0;
}

void poll_server(struct server *srv, struct serveropts *svopts, int wait)
{

    struct epoll_event ee = {0};

    int ret = epoll_wait(srv->epollfd, &ee, 1, wait);

    if (ret == 0)
        return;

    // New connection
    if (ee.data.fd == srv->sockfd)
    {
        struct Node *client = insert_node(srv->clients);

        // echo_list(srv->clients);

        memset(&client->caddr, 0, sizeof(client->caddr));
        socklen_t addrlen = sizeof(client->caddr);
        // I didn't realize this was important read manual harder

        // TODO: accept may need error handling
        client->connfd = accept(srv->sockfd, (struct sockaddr *)&client->caddr, &addrlen);
        fcntl(client->connfd, F_SETFL, O_NONBLOCK);

        printf("new connection established: %d, %p\n", client->connfd, (void *)&client->caddr);

        // !TODO trying out getpeername for both server and client.
        // if this works ill just replace the whole thing

        socklen_t len = 0;
        struct sockaddr_storage peeraddr = {0};
        char ipstr[INET_ADDRSTRLEN] = {0};
        int port = 0;

        printf("receiving peername\n");
        len = sizeof(peeraddr);
        int ret = getpeername(client->connfd, (struct sockaddr *)&peeraddr, &len);

        if (ret < 0)
        {
            perror("getpeername");
        }

        if (peeraddr.ss_family == AF_INET)
        {
            printf("Connection normal\n");
            struct sockaddr_in *s = (struct sockaddr_in *)&peeraddr;
            port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
        }
        else
        {
            // IPv6
            printf("Handling IPv6 connection\n");
        }

        printf("Received new connection from %s:%d\n", ipstr, port);

        add_epoll_watch(srv->epollfd, client->connfd, client, (EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP));
        // TODO: send connected message

        // EPOLLIN  - ready to read
        // EPOLLOUT - ready to write

        return;
    }

    /* Data arriving on an already-connected socket */
    if (ee.events & EPOLLIN)
    {

        struct Node *client = ee.data.ptr;
        assert(client != NULL && "read requested client was NULL");

        char msg_buffer[MAX_MSG] = {0};
        memset(&msg_buffer, '\0', sizeof(msg_buffer));
        // i should probably memset this before sending it over the net

        /* an error occured */
        if (read_socket(client->connfd, msg_buffer, sizeof(msg_buffer), 0) < 0)
        {

            #define CLIENT_DISCON "client has left\n"

            // lets try to find out why this isnt sending
            broadcast(srv, NULL, CLIENT_DISCON);
            if (close_socket(client, srv) < 0)
                fprintf(stderr, "Failure to close client socket: %s\n", strerror(errno));
        }
        else
        {

            if (strlen(client->nickname) > 1)
            {

                printf("\nServer: %s\n", msg_buffer);
            }
            else
            {
                printf("\n%s: %s\n", client->nickname, msg_buffer);
            }

            if (svopts->verbose) // vvvv turn this into a macro
                fprintf((svopts->logfile == NULL) ? (stdout) : (svopts->logfile),
                        "<host %s:%u sent %ld byte(s): [%s]>\n",
                        "hostname",
                        1234,
                        sizeof(msg_buffer),
                        msg_buffer);

            broadcast(srv, client, msg_buffer);
        }

        // parse commands here use strtok to get the argument of the command
        if (strncmp("/nick", msg_buffer, 5) == 0)
        {
            printf("Recieved switch\n");

            memset(client->nickname, '\0', sizeof(client->nickname));
            char *username = "admin";
            strncpy(client->nickname, username, strlen(username));
            // client->nickname = "admin";
        }

        // read hangup?
    }
    else if (ee.events & (EPOLLRDHUP | EPOLLHUP))
    {
        struct Node *client = ee.data.ptr;
        printf("Client on socket %d closed the connection\n", client->connfd);

        // whenever SEND_SHUTDOWN or RCV_SHUTDOWN are marked, this is called.
        // which is equal to a call of shutdown(SHUT_WR | SHUT_RD)
        // https://stackoverflow.com/questions/52976152/tcp-when-is-epollhup-generated

        if (shutdown(client->connfd, SHUT_RDWR) < 0)
        {
            perror("shutdown");
        }

        // we already call this if theres an "error" waiting to integrate this until scom/client is done
        // close_socket(client, srv);

        return;
    }
}

void shutdown_server(struct server *srv)
{

#define SERVER_SHUTDOWN "Server shutting down...\n"
    if (srv->clients->head != NULL && srv->clients->capacity >= 1)
    {
        broadcast(srv, NULL, SERVER_SHUTDOWN);
    }

    delete_list(srv->clients); // ALWAYS delete clients

    // man 3p shutdown
    // shutdown the server from writing send FIN
    if (shutdown(srv->sockfd, SHUT_WR) < 0)
    {
        perror("shutdown");
    }

    // close the server and epoll socket
    if (close(srv->sockfd) < 0)
    {
        perror("close");
    }

    if (close(srv->epollfd) < 0)
    {
        perror("close");
    }
}
