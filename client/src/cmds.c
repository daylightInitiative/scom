
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#include "client.h"
#include "cmds.h"
#include "log.h"

void scom_usage(int status) {

    // this should be used for scom shell commands not its arguments
    fprintf(stdout, "displaying internal commands....\n");

    return;
}

void scom_ping(int argc, char *argv[], void *userdata) {
    struct Client *c = (struct Client *)userdata;
    int sockfd = c->serverfd;
    logfmt(stdout, INFO, "Conducting ping test");

    clock_gettime(CLOCK_MONOTONIC, &c->last_ping);
    send_socket(sockfd, "PING\n", 0);
}

void scom_help(int argc, char *argv[], void *userdata) {
    scom_usage(0);
}

void scom_exit(int argc, char *argv[], void *userdata) {
    printf("Exiting.\n");

    return;
}