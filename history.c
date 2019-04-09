#include "built_in.h"
#include "cmd_handling.h"
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

void add_to_history(struct command_line * cmds, int comm_sz, char * line) {
	int i;
	int j;

	//check if there even is anything bc we gotta check stuff
	if(comm_sz > 0) {
		//check if its a history execution
		if(cmds[0].tokens[0][0] == '!') {
			//skip that stuff
			free(line);
			return;
		}
	}

	//if we are at max cap, then move stuff down
	if (counter == 100) {
		//update counter back
		counter--;
		//free the first set of commands
		free_cmds(hist_list[0].cmds, hist_list[0].comm_sz);
		//move everything down
		memmove(&hist_list[0], &hist_list[1], sizeof(struct history_entry) * counter);
	}

	//make a pointer so it makes our lives easier
	struct history_entry * item = &hist_list[counter];

	//keep track of cmd id
	item->cmd_id = obj_total++;
	//get a copy of the commands
	item->cmds = malloc(comm_sz * sizeof(struct command_line));
	for (i = 0; i < comm_sz; i++) {
		//copy pipe status
		item->cmds[i].stdout_pipe = cmds[i].stdout_pipe;

		//copy output file stuff
		if (cmds[i].stdout_file != NULL) {
			item->cmds[i].stdout_file = (char *) malloc(BUF_SZ);
			strcpy(item->cmds[i].stdout_file, cmds[i].stdout_file);
		} else {
			item->cmds[i].stdout_file = NULL;
		}

		//copy token size
		item->cmds[i].tokens_size = cmds[i].tokens_size;

		//allocate space for tokens
		item->cmds[i].tokens = (char **) malloc(cmds[i].tokens_size* sizeof(char *));
		for (j = 0; j < cmds[i].tokens_size; j++) {
			//if that stuff is null, we at the end
			if (cmds[i].tokens[j] == NULL) {
				item->cmds[i].tokens[j] = (char *) NULL;
				break;
			}
			//copy tokens
			item->cmds[i].tokens[j] = (char *) malloc(BUF_SZ);
			strcpy(item->cmds[i].tokens[j], cmds[i].tokens[j]);
		}
	}

	//copy total # of commands
	item->comm_sz = comm_sz;
	//copy original line
	item->line = malloc(ARG_MAX);
	strcpy(item->line, line);

	//free the original original line???
	free(line);

	//update counter if we not at max
	if (counter < 100) {
		counter++;
	}
}

void free_history(void) {
	int i;

	for (i = 0; i < counter; i++) {
		//free history!
		free_cmds(hist_list[i].cmds, hist_list[i].comm_sz);
		//free original line!!
		free(hist_list[i].line);
	}
}

void history_exec(struct command_line * cmds, char * line) {
	//check what kind of execution we're doing
	if (strcmp(cmds[0].tokens[0], "!!") == 0) {
		run_history_command(obj_total - 1, line);
	} else if (cmds[0].tokens[0][0] == '!') {
		int hist_num = atoi(&cmds[0].tokens[0][1]);
		if (hist_num == 0 && strcmp(&cmds[0].tokens[0][1], "0") == 0) {
			run_history_command(hist_num, line);
		} else if (hist_num > 0) {
			run_history_command(hist_num, line);
		} else if (hist_num < 0) {
			printf("%s: invalid history number\n", cmds[0].tokens[0]);
		}
	}
}

void run_history_command(int hist_num, char * line) {
	int status;

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
	if (!check_built_in(hist_list[index].cmds, hist_list[index].comm_sz, line)) {
		pid_t pid = fork();

		if (pid == 0) {
			if (!isatty(STDIN_FILENO)) {
				close(fileno(stdin));
			}
			execute_pipeline(hist_list[index].cmds);
		} else if (pid < 0) {
			perror("fork");
			exit(1);	
		} else {
			wait(&status);
		}
	}
}