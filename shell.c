#include "built_in.h"
#include "cmd_handling.h"
#include "history.h"
#include "str_func.h"
#include "timer.h"

int main(void) {
	char * line = NULL;
	size_t line_sz = 0;
	int status;

	//initalize built-in stuff
	init_built_in();

	//start timing
	double start = get_time();

	//run infinitely until user specifies exit
	while (1) {
		//initialize struct that holds commands
		struct command_line * cmds = malloc(ARG_MAX * sizeof(struct command_line));
		//total num of commands
		int comm_sz;

		//check if we're scripting
		if (isatty(STDIN_FILENO)) {
			print_prompt();  
		}

		//grab the line
		ssize_t sz = getline(&line, &line_sz, stdin);

		//if there's nothing then skip it/break it
		if (sz == -1 || sz == EOF) {
			close(fileno(stdin));
			break;
		} else if (sz - 1 == 0) {
			decrease_comm_num();
			continue;
		}
		
		//save a copy of the original line
		char * orig_line = malloc(sz + 1);
		strcpy(orig_line, line);

		//grab the commands
		comm_sz = get_commands(line, cmds);

		if (comm_sz == -1) {
			break;
		}

		//if there's nothing continue
		if (comm_sz == 0) {
			add_to_history(cmds, comm_sz, orig_line);
			continue;
		}

		//check if we have any built ins
		if (strcmp(cmds[0].tokens[0], "exit") == 0) {
			free_cmds(cmds, comm_sz);
			break;
		} else if (check_built_in(cmds, comm_sz, orig_line)) {
			free_cmds(cmds, comm_sz);
			continue;
		}

		//fork!
		pid_t pid = fork();

		//execute our commands
		if (pid == 0) {
			if (!isatty(STDIN_FILENO)) {
				close(fileno(stdin));
			}
			execute_pipeline(cmds);
		} else if (pid < 0) {
			perror("fork");
			exit(1);	
		} else {
			wait(&status);
		}

		free_cmds(cmds, comm_sz);
	}
	sleep(1);

	//free history stuff
	free_history();

	//free env variables
	free_env();

	double end = get_time();

	if (isatty(STDIN_FILENO)) {
		printf("Time elapsed: %fs\n", end - start);
	}
	return 0;
}
