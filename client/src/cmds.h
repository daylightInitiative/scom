#ifndef SCOMD_CMDS_H
#define SCOMD_CMDS_H

#define CMD_PREFIX '/' // worst bug i ever had
#define MAX_TOKENS 16

typedef void (*CommandFunc)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *description;
    CommandFunc func;
} Command;

// func definitions
void cmd_help(int argc, char *argv[]);
void cmd_exit(int argc, char *argv[]);
int run_command(char *input);


#endif