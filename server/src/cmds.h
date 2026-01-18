#ifndef SCOMD_CMDS_H
#define SCOMD_CMDS_H

// server command evaluator

#include "../shared/run_cmd.h"

int run_server_command(Command commands[], char *input, void *userdata);
void scomd_help(int argc, char *argv[], void *userdata);
void scomd_nick(int argc, char *argv[], void *userdata);
void scomd_kick(int argc, char *argv[], void *userdata);
void scomd_list(int argc, char *argv[], void *userdata);

#endif