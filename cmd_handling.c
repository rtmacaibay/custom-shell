#include "cmd_handling.h"
#include "debug.h"
#include "history.h"

int init = 1;

int comm_num = 0;
bool changed = false;

char user[BUF_SZ];
char host[BUF_SZ];

char curr_dir[PATH_MAX];
char home_dir[PATH_MAX];
int home_dir_size;
int curr_dir_size;

//environ keeps track of our environment vars
char * e[BUF_SZ];
int env_size = 0;

void execute_cmd(char ** tokens) {
	LOG("Executing %s\n", tokens[0]);
	if (strcmp(tokens[0], "exit") == 0) {
		exit(0);
	} else if (strcmp(tokens[0], "cd") == 0) {
		exit(0);
	}
	if (execvp(tokens[0], tokens) < 0) {
		//printf("%s: command not found\n", tokens[0]);
		//kill(0, SIGUSR1);
		LOG("%s died\n", tokens[0]);
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

	if (comm_sz > 0)
		LOG("Freeing %s with comm_sz %d\n", cmds[0].tokens[0], comm_sz);

	for (i = 0; i < comm_sz; i++) {
		LOG("tokens size: %d\n", cmds[i].tokens_size);
		for (j = 0; j < cmds[i].tokens_size - 1; j++) {
			if (cmds[i].tokens[j] != NULL)
				free(cmds[i].tokens[j]);
		}
		LOGP("I still work here?\n");
		if (cmds[i].stdout_file != NULL) {
			free(cmds[i].stdout_file);
		}
		LOGP("I still work here?\n");
		free(cmds[i].tokens);
	}

	free(cmds);

	if (comm_sz > 0)
		LOGP("Done freeing\n");
	cmds = NULL;
}

int get_commands(char * line, struct command_line * cmds) {
	//grab first token
	char * tok = next_token(&line, " \'\"\t\n");
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
			tok = next_token(&line, " \'\"\t\n");
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
				tok = next_token(&line, " \'\"\t\n");
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
		//copy the token
		char * expand = expand_var(tok);
		if (expand != NULL) {
			while (strstr((const char *)expand, "$") != NULL) {
				expand = expand_var(expand);
			}

			strcpy(curr_tokens[curr->tokens_size++], expand);
		} else {
			strcpy(curr_tokens[curr->tokens_size++], tok);
		}

		//tokenize
		tok = next_token(&line, " \'\"\t\n");
		//we at max cap
		if (curr->tokens_size > ARG_MAX) {
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

int run(char * line, int add) {
	//initialize struct that holds commands
	struct command_line * cmds = malloc(ARG_MAX * sizeof(struct command_line));
	//total num of commands
	int comm_sz = 0;
	int status;

	char * orig_line = strdup(line);

	//grab the commands
	comm_sz = get_commands(line, cmds);

	//this means we went over the max args
	if (comm_sz == -1) {
		if (add)
			add_to_history(orig_line);
		free_cmds(cmds, comm_sz);
		return 1;
	}

	//if there's nothing, add to history but continue
	if (comm_sz == 0) {
		if (add)
			add_to_history(orig_line);
		free_cmds(cmds, comm_sz);
		return 1;
	}

	//check if we have any built ins
	if (strcmp(cmds[0].tokens[0], "exit") == 0) {
		free_cmds(cmds, comm_sz);
		free(orig_line);
		return 0;
	} else if (check_built_in(cmds, comm_sz, orig_line, add)) {
		free_cmds(cmds, comm_sz);
		return 1;
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
		LOGP("Waiting for child\n");
		waitpid(pid, &status, 0);
		LOGP("Done waiting\n");
	}

	free_cmds(cmds, comm_sz);
	return 1;
}

void sigint_handler(int signo) {
	//do nothing haha!
}

void init_built_in(void) {
	//literally initialize home dir
	strcpy(user, getlogin());
	gethostname(host, BUF_SZ);
	set_home();
	getcwd(curr_dir, PATH_MAX);
	curr_dir_size = strlen(curr_dir);
	e[0] = NULL;
	signal(SIGINT, sigint_handler);
}

void print_prompt(void) {
	char * dir;

	if (home_dir_size == curr_dir_size) {
		dir = "~";
	} else {
		dir = strrchr(curr_dir, '/');
		dir = &dir[1];
	}

	printf("--[%d|%s@%s: %s]--$ ", comm_num++, user, host, dir);
	fflush(stdout);
}

void set_home(void) { 
	struct passwd * pwu = getpwuid(getuid());
	strcpy(home_dir, pwu->pw_dir);
	home_dir_size = strlen(home_dir);
	if (init) {
		init = 0;
		return;
	} 
	curr_dir_size = home_dir_size;
	strcpy(curr_dir, home_dir);
	chdir(curr_dir);
}

void change_dir(char ** tokens, int tokens_size) {
    //char dir[BUF_SZ];

	if (tokens_size == 2) {
		set_home();
	} else {
		if (strcmp(tokens[1], "~") == 0) {
			set_home();
		} else if (chdir(tokens[1])) {
	    	//printf("%s: %s\n", tokens[1], strerror(errno));
		} else {
			getcwd(curr_dir, BUF_SZ);
			curr_dir_size = strlen(curr_dir);
		}
	}
}

void change_env(char ** tokens, int tokens_size) {
	if (tokens_size < 4) {
		return;
	}

	//set the environment
	if (setenv(tokens[1], tokens[2], true)) {
		printf("setenv: %s\n", strerror(errno));
	} else {
		//keep track of those variables
		e[env_size] = malloc(BUF_SZ);
		strcpy(e[env_size], tokens[1]);
		strcat(e[env_size], "=");
		strcat(e[env_size++], tokens[2]);
		e[env_size] = NULL;
	}
}

int check_built_in(struct command_line * cmds, int comm_sz, char * line, int add) {
	int is_built_in = 0;

	if (strcmp(cmds[0].tokens[0], "setenv") == 0) {
		if (add)
			add_to_history(line);
		change_env(cmds[0].tokens, cmds[0].tokens_size);
		is_built_in = 1;
	} else if (strcmp(cmds[0].tokens[0], "getset") == 0) {
		if (add)
			add_to_history(line);
		char * curr;
		int index = 0;

		while ((curr = e[index++]) != NULL) {
			printf("%s\n", curr);
		}

		is_built_in = 1;
	} else if (strcmp(cmds[0].tokens[0], "cd") == 0) {
		if (add)
			add_to_history(line);
		change_dir(cmds[0].tokens, cmds[0].tokens_size);
		is_built_in = 1;
	} else if (strcmp(cmds[0].tokens[0], "history") == 0) {
		if (add)
			add_to_history(line);
		print_history();
		is_built_in = 1;
	} else if (cmds[0].tokens[0][0] == '!') {
		history_exec(cmds);
		is_built_in = 1;
	}
	if (add && !is_built_in)
		add_to_history(line);
	return is_built_in;
}

void free_env(void) {
	if (env_size > 0) {
		int i;
		for (i = 0; i < env_size; i++) {
			free(e[i]);
		} 
	}
}

void decrease_comm_num(void) {
	comm_num--;
}