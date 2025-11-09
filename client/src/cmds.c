
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "cmds.h"
#include "log.h"

void scom_usage(int status) {

    // this should be used for scom shell commands not its arguments
    fprintf(stdout, "displaying internal commands....\n");

    exit(status);
}

void scom_help(int argc, char *argv[]) {
    scom_usage(0);
}

void scom_exit(int argc, char *argv[]) {
    printf("Exiting.\n");
    exit(0);
}