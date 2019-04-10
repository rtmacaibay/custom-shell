#include "cmd_handling.h"
#include "debug.h"
#include "history.h"

#include <stdio.h>

struct history_entry hist_list[HIST_MAX];
int counter = 0;
long int obj_total = 0;

void print_history(void) {
    /* This function should print history entries */
    int i;

    for (i = 0; i < counter; i++) {
    	//print stuff
    	printf("   %ld %s", hist_list[i].cmd_id, hist_list[i].line);
    }
}

void add_to_history(char * line) {
	//if we are at max cap, then move stuff down
	if (counter == 100) {
		//update counter back
		counter--;
		//free the first set of commands
		free(hist_list[0].line);
		//move everything down
		memmove(&hist_list[0], &hist_list[1], sizeof(struct history_entry) * counter);
	}

	//make a pointer so it makes our lives easier
	struct history_entry * item = &hist_list[counter];

	//keep track of cmd id
	item->cmd_id = obj_total++;
	//get a copy of the commands
	item->line = strdup(line);
	free(line);

	//update counter if we not at max
	if (counter < 100) {
		counter++;
	}
}

void free_history(void) {
	int i;

	for (i = 0; i < counter; i++) {
		//free original line!!
		LOG("Freeing history: %s\n", hist_list[i].line);
		free(hist_list[i].line);
		LOGP("Done freeing history\n");
	}
}

void history_exec(struct command_line * cmds) {
	//check what kind of execution we're doing
	if (strcmp(cmds[0].tokens[0], "!!") == 0) {
		run_history_command(obj_total - 1);
	} else if (cmds[0].tokens[0][0] == '!') {
		int hist_num = atoi(&cmds[0].tokens[0][1]);
		if (hist_num == 0 && strcmp(&cmds[0].tokens[0][1], "0") == 0) {
			run_history_command(hist_num);
		} else if (hist_num > 0) {
			run_history_command(hist_num);
		} else if (hist_num < 0) {
			printf("%s: invalid history number\n", cmds[0].tokens[0]);
		}
	}
}

void run_history_command(int hist_num) {
	//check if we're out of bounds
	if (hist_num >= obj_total) {
		return;
	}
	if (obj_total > counter) {
		if (hist_num < obj_total - counter) {
			return;
		}
	} else {
		if (hist_num < 0) {
			return;
		}
	}

	//get where that index is in our list
	int index = counter - (obj_total - hist_num);

	//run
	run(hist_list[index].line, 0);
}