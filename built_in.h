#ifndef _BUILT_IN_H_
#define _BUILT_IN_H_

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmd_handling.h"
#include "history.h"

void init_built_in(void);

void set_home(void);

void print_prompt(void);

void change_dir(char ** tokens, int tokens_size);

void change_env(char ** tokens, int tokens_size);

int check_built_in(struct command_line * cmds, int comm_sz, char * line, int add);

void free_env(void);

void decrease_comm_num(void);

#endif