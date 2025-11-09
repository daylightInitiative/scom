#ifndef SCOM_SHARED_RUNCMD_H
#define SCOM_SHARED_RUNCMD_H

#define CMD_PREFIX '/' // worst bug i ever had
#define MAX_TOKENS 16

typedef void (*CommandFunc)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *description;
    CommandFunc func;
} Command;

int run_command(Command commands[], char *input);


#endif