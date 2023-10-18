
#ifndef SCOM_LIST_H
#define SCOM_LIST_H
#include <netinet/in.h>
#include <sys/socket.h>

#include "server.h"

#define MAX_NAME 32

// TODO: hashmap or queue?? implementation
struct Node {

    char nickname[MAX_NAME];
    int connfd;
    
    struct sockaddr_storage caddr;
    socklen_t sock_len;

    struct server *srv;

    struct Node *next;
    struct Node *prev;
};

struct List {
 
    struct Node *head;
    struct Node *tail;

    unsigned int capacity;
};


struct List *create_list(void);
void delete_list(struct List *lp);
void echo_list(struct List *lp);

struct Node *insert_node(struct List *lp);
void remove_node(struct List *lp, struct Node *node);

#endif
