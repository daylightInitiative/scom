
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
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
    //remove_node(srv->clients, client); // remove the node from the list
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
    struct Node *node = (struct Node *)userdata;
    struct server *srv = node->srv;

    int nusers = srv->clients->capacity;
    struct Node *client = srv->clients->head;
    
    // very fun to implement, taught me about memory vs string boundaries
    /*
        if your variable is the number of chars written
        str[len] = '\0'

        if your variable is the total allocation size
        str[len - 1] = '\0'
    */
    // preallocating memory since we know the quanity of clients is more efficient
    char *prefix = "Current Users: ";
    char *seperator = ", ";
    size_t sep_len = strlen(seperator);
    size_t prefix_len = strlen(prefix);
    size_t bufsize = prefix_len + 1 + 1; // \n \0
    for (struct Node *c = srv->clients->head; c; c = c->next)
        bufsize += strlen(c->nickname) + sep_len;

    // lets minus one seperator length because we dont need one at the end
    bufsize -= sep_len;

    char *users_string = malloc(bufsize);
    if (!users_string) return;
    memset(users_string, '\0', bufsize);
    
    memcpy(users_string, prefix, prefix_len);
    size_t offset = prefix_len;

    int client_n = 1;
    for (struct Node *client = srv->clients->head; client; client = client->next) {
        size_t nick_len = strlen(client->nickname);
        memcpy(users_string + offset, client->nickname, nick_len);
        offset += nick_len;
        if (client_n != nusers) {
            memcpy(users_string + offset, seperator, sep_len);
            offset += sep_len;
        }
        client_n++;
    }

    users_string[bufsize - 2] = '\n';
    users_string[bufsize - 1] = '\0';

    send_socket(node->connfd, users_string, 0);

    free(users_string);
}

void scomd_kick(int argc, char *argv[], void *userdata) {
    if (userdata == NULL) {
        logfmt(stderr, ERROR, "%s was called without connection context", __func__);
        return;
    }
    struct Node *client = (struct Node *)userdata;
    struct server *srv = client->srv;

    if (argc > 1 && argv[1] != NULL) {

        struct Node *found_client = find_client_by_nickname(srv, argv[1]);
        if (found_client) {
            send_socket(found_client->connfd, "You have been kicked by an operator.\n", 0);
            logfmt(NULL, INFO, "%s was kicked from the server.", client->nickname);
            disconnect_client(client, srv);
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
    if (argc > 1 && argv[1] != NULL) {

        char *new_name = argv[1];
        for (int i = 0; i < strlen(new_name); i++) {
            if (ispunct(new_name[i]) || isblank(new_name[i])) {
                // disallow special characters
                return;
            }
        }

        logfmt(stdout, INFO, "%s is the supplied nickname", new_name);

        snprintf(client->nickname, MAX_NAME, "%s", new_name);

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