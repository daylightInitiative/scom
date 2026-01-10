
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "log.h"
#include "run_cmd.h"
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

void scomd_nick(int argc, char *argv[], void *userdata) {

    if (userdata == NULL) {
        logfmt(stderr, ERROR, "%s was called without connection context", __func__);
        return;
    }
    struct Node *client = (struct Node *)userdata;
    
    // create a 0 initialized buffer
    if (argv[1] != NULL) {
        logfmt(stdout, INFO, "%s is the supplied nickname", argv[1]);

        snprintf(client->nickname, MAX_NAME, "%s", argv[1]);
    }
    

    return;
}

void scomd_help(int argc, char *argv[], void *userdata) {


}