#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "built_in.h"
#include "cmd_handling.h"

#define HIST_MAX 100

struct history_entry {
    unsigned long cmd_id;
    double run_time;
    struct command_line * cmds;
    int comm_sz;
    char * line;
    /* What else do we need here? */
};

void print_history(void);

void add_to_history(struct command_line * cmds, int comm_sz, char * line);

void free_history(void);

void history_exec(struct command_line * cmds, char * line);

void run_history_command(int hist_num, char * line);

#endif
