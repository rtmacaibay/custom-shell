#ifndef _CMD_HANDLING_H_
#define _CMD_HANDLING_H_

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "str_func.h"

#define BUF_SZ 128

#ifndef ARG_MAX
#define ARG_MAX 4096
#endif

struct command_line {
	char ** tokens;
	int tokens_size;
	bool stdout_pipe;
	char * stdout_file;
};

void execute_cmd(char ** tokens);

void execute_pipeline(struct command_line * cmds);

void free_cmds(struct command_line * cmds, int comm_sz);

void remove_quotes(char ** tok);

int get_commands(char * line, struct command_line * cmds);

#endif
