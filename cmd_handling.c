#include "cmd_handling.h"

void execute_cmd(char ** tokens) {
	if (strcmp(tokens[0], "exit") == 0) {
		exit(0);
	} else if (strcmp(tokens[0], "cd") == 0) {
		exit(0);
	}
	if (execvp(tokens[0], tokens) < 0) {
		//printf("%s: command not found\n", tokens[0]);
		//kill(0, SIGUSR1);
		exit(1);
	}
}

void execute_pipeline(struct command_line *cmds) {
	if (cmds[0].tokens_size == -1) {
		return;
	}
    //create pipes
	int fd[2];

    //check if our pipes got rekt
	if (pipe(fd) == -1) {
		perror("pipe");
		return;
	}

    //base case: check if we're at last set of commands
	if (!cmds[0].stdout_pipe) {

		//check there is an output file
		if (cmds[0].stdout_file != NULL) {
	    	//open the file for output
			fd[0] = open(cmds[0].stdout_file, O_CREAT |  O_WRONLY | O_TRUNC, 0666);
	    	//finesse output file
			dup2(fd[0], STDOUT_FILENO);
		}
		//run the command
		//execvp(cmds[0].tokens[0], cmds[0].tokens);
		execute_cmd(cmds[0].tokens);
		return;   
	}

    //fork a new process yeet
	pid_t pid = fork();

    //check if child or parent
	if (pid == 0) {
		/* definitely a child */
		//get output stream
		dup2(fd[1], STDOUT_FILENO);
		//close input stream
		close(fd[0]);
		//execute command
		//execvp(cmds[0].tokens[0], cmds[0].tokens);
		execute_cmd(cmds[0].tokens);
		//go to sleep fools
		//sleep(1);
	} else if (pid < 0) {
		perror("fork");
		exit(1);	
	} else {
		/* definitely a parent */
		//get input stream
		dup2(fd[0], STDIN_FILENO);
		//close output stream
		close(fd[1]);
		//recursion lmao -> go to next command
		execute_pipeline(&cmds[1]);
	}
}

void free_cmds(struct command_line * cmds, int comm_sz) {
	int i;
	int j;

	for (i = 0; i < comm_sz; i++) {
		for (j = 0; j < cmds[i].tokens_size - 1; j++) {
			if (cmds[i].tokens[j] != NULL)
				free(cmds[i].tokens[j]);
		}
		if (cmds[i].stdout_file != NULL) {
			free(cmds[i].stdout_file);
		}
		free(cmds[i].tokens);
	}
	free(cmds);
	cmds = NULL;
}

void remove_quotes(char ** tok) {
	char copy[BUF_SZ];
	char new_tok[BUF_SZ];
	strcpy(copy, *tok);
	char * quote_type = strpbrk(copy, "\'\"");

	//if we got a quote, then we gottem
	if (quote_type) {
		//lets copy this stuff
		memset(new_tok, 0, BUF_SZ);
		quote_type[1] = '\0';
		int start = strcspn(*tok, quote_type);
		int offset = start + 1;
		int end = strcspn(*tok + offset, quote_type) + offset;
		strncpy(new_tok, *tok, start);
		strncat(new_tok, *tok + offset, end - offset);
		strcat(new_tok, *tok + (end + 1));
		memmove(*tok, new_tok, BUF_SZ);
		if (quote_type[0] == '\'') {
			return;
		}
	}

	//this is to expand our env vars
	if (**tok == '$') {
		int offset = 1;
		if (*(*tok + 1) == '{') {
			*(*tok + (strrchr(*tok, '}') - *tok)) = '\0';
			offset = 2;
		}
		char ret[BUF_SZ];
		memset(ret, 0, BUF_SZ);
		if (getenv(*tok + offset) != NULL) {
			strcpy(ret, getenv(*tok + offset));
			memmove(*tok, ret, BUF_SZ);
		}
	}
}

int get_commands(char * line, struct command_line * cmds) {
	//grab first token
	char * tok = next_token(&line, " \t\n");
	//this is to check if we're on a new command
	bool new_command = true;
	//keeps track of the total commands we have
	int comm_sz = -1;

	//points to the current cmd struct
	struct command_line * curr;
	//points to the current set of tokens
	char ** curr_tokens;

	//tokenize
	while (tok != NULL) {
		//are we piping?
		if (strcmp(tok, "|") == 0 && !new_command) {
			//note we're in a new command now
			new_command = true;
			//tokenize
			tok = next_token(&line, " \t\n");
			//note that our current cmd is now piping
			curr->stdout_pipe = true;
			//cont...
			continue;
		}

		//if we run into a comment, we dont care about the rest
		if (tok[0] == '#') {
			break;
		}

		//oh look, redirection
		if (strcmp(tok, ">") == 0 && !new_command) {
			if (comm_sz != -1) {
				tok = next_token(&line, " \t\n");
				if (tok != NULL) {
					curr->stdout_file = malloc(BUF_SZ);
					strcpy(curr->stdout_file, tok);
				}
				break;
			}
		}

		//check if we have a new command
		if (new_command) {
			//if we not at beginning, set last token to be null
			if (comm_sz != -1) {
				curr_tokens[curr->tokens_size++] = (char *) NULL;
			}

			//increase total commands
			comm_sz += 1;
			new_command = false;

			//keep track of that spot
			curr = &cmds[comm_sz];

			//set things to defaults
			curr->stdout_pipe = false;
			curr->stdout_file = NULL;

			//keep track of tokens array
			curr->tokens = (char **) malloc(ARG_MAX * sizeof(char *));
			curr_tokens = curr->tokens;
			curr->tokens_size = 0;
		}

		//initialize a token
		curr_tokens[curr->tokens_size] = (char *) malloc(BUF_SZ);
		//remove quotes
		remove_quotes(&tok);
		//copy the token
		strcpy(curr_tokens[curr->tokens_size++], tok);
		//tokenize
		tok = next_token(&line, " \t\n");
		//we at max cap
		if (curr->tokens_size >= ARG_MAX) {
			//printf("%s: Too many arguments.\n", curr_tokens[0]);
			return -1;
		}
	}

	//make sure to set last token to null
	if (comm_sz != -1) {
		curr_tokens[curr->tokens_size++] = (char *) NULL;
	}
	return comm_sz + 1;
}