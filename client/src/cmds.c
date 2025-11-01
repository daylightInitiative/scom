
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

void cmd_help(int argc, char *argv[]) {
    scom_usage(0);
}

void cmd_exit(int argc, char *argv[]) {
    printf("Exiting.\n");
    exit(0);
}

Command commands[] = {
    {"help", "Show available commands", cmd_help},
    {"exit", "Exit the program", cmd_exit},
    {"quit", "Exit the program", cmd_exit},
    {NULL, NULL, NULL} // sentinel
};

int run_command(char *input) {
    char *argv[MAX_TOKENS];
    int argc = 0;

    char *token = strtok(input, " \t\n");
    while (token && argc < MAX_TOKENS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (argc == 0)
        return 1;

    for (int i = 0; commands[i].name != NULL; i++) {
        // pointer arithmetic is interesting...
        // not having to do argv[0] + (sizeof(char) * offset) is quite nice
        if (argv[0][0] == CMD_PREFIX && (strcmp(argv[0] + 1, commands[i].name) == 0)) {
            commands[i].func(argc, argv);
            return 0;
        }
    }

    logfmt(stdout, ERROR, "Unknown command: %s\n", argv[0]);
    return 1;
}