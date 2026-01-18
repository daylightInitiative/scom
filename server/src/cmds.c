
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "log.h"
#include "run_cmd.h"
#include "server.h"
#include "list.h"

// inside here we are going to define functions but also a wrapper around run_cmd for server connfd context
int run_server_command(Command commands[], char *input, void *userdata) {
    // so this way we can do permission checks, etc without repeating the same code

    struct Node *client = (struct Node *)userdata;
    logfmt(stderr, ERROR,
       "DEBUG connfd=%d (userdata=%p)",
       client->connfd, (void *)client);

    // permission checks TODO

    logfmt(stdout, DEBUG, "Passed permission checks while running command.");

    if (run_command(commands, input, userdata) > 0) {
        logfmt(stderr, ERROR, "Error while parsing command from socket %d", client->connfd);
        return 1;
    }

    return 0;
}

// probably going to use this for banning so putting it in a function for now
void disconnect_client(struct Node *client, struct server *srv) {

    int sockfd = client->connfd;
    shutdown(sockfd, SHUT_RDWR); // stop reading and writing to this socket

    close_socket(client, srv); // safely disconnect
}

/* finds the client by the given nickname and returns it, -1 upon failure to locate */
struct Node *find_client_by_nickname(struct server *srv, char *nickname) {

    struct Node *client = srv->clients->head;
    for (client = client; client != NULL; client = client->next)
    {
        if (strcmp(client->nickname, nickname) == 0)
            return client;
    }
    
    return NULL;
}

void scomd_list(int argc, char *argv[], void *userdata) {
    struct Node *client = (struct Node *)userdata;
    struct server *srv = client->srv;

    int nusers = 0;
    struct Node *client = srv->clients->head;


}

void scomd_kick(int argc, char *argv[], void *userdata) {
    if (userdata == NULL) {
        logfmt(stderr, ERROR, "%s was called without connection context", __func__);
        return;
    }
    struct Node *client = (struct Node *)userdata;
    struct server *srv = client->srv;

    if (argv[1] != NULL) {

        struct Node *found_client = find_client_by_nickname(srv, argv[1]);
        if (found_client) {
            disconnect_client(client, srv);
            logfmt(NULL, INFO, "%s was kicked from the server.", client->nickname);
        } else {
            logfmt(stdout, WARN, "User with nickname %s was not found.", argv[1]);
        }
    }
}

void scomd_nick(int argc, char *argv[], void *userdata) {

    if (userdata == NULL) {
        logfmt(stderr, ERROR, "%s was called without connection context", __func__);
        return;
    }
    struct Node *client = (struct Node *)userdata;
    struct server *srv = client->srv;

    char original_name[MAX_NAME];
    snprintf(original_name, MAX_NAME, "%s", client->nickname);
    
    // create a 0 initialized buffer
    if (argv[1] != NULL) {
        logfmt(stdout, INFO, "%s is the supplied nickname", argv[1]);

        snprintf(client->nickname, MAX_NAME, "%s", argv[1]);

        size_t buflen = (MAX_NAME * 2) + 25;
        char nick_changed[buflen];
        snprintf(nick_changed, buflen, "%s is now named %s\n", original_name, client->nickname);

        char *nickname = strdup(nick_changed);
        broadcast(srv, NULL, nick_changed);
        free(nickname);
    }
    

    return;
}

void scomd_help(int argc, char *argv[], void *userdata) {


}