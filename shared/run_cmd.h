#ifndef SCOM_SHARED_RUNCMD_H
#define SCOM_SHARED_RUNCMD_H

#define CMD_PREFIX '/' // worst bug i ever had
#define MAX_TOKENS 16

// typedef void (*CommandFunc)(int argc, char *argv[]);
typedef void (*CommandFunc)(int argc, char *argv[], void *userdata); // passing a userdata here because its a common pattern

typedef struct {
    const char *name;
    const char *description;
    CommandFunc func;
} Command;

int run_command(Command commands[], char *input, void *userdata);


#endif