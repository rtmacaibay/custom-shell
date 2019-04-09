#include "built_in.h"

int comm_num = 0;
bool changed = false;

char user[BUF_SZ];
char host[BUF_SZ];

char curr_dir[BUF_SZ];
char home_dir[BUF_SZ];
int home_dir_size;
int curr_dir_size;

//environ keeps track of our environment vars
char * environ[BUF_SZ];
int env_size = 0;

void sigint_handler(int signo) {
	//do nothing haha!
}

void init_built_in(void) {
	//literally initialize home dir
	strcpy(user, getlogin());
	gethostname(host, BUF_SZ);
	set_home();
	environ[0] = NULL;
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
	strcpy(home_dir, "/home/");
	strcat(home_dir, user);
	home_dir_size = strlen(home_dir);
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
		//printf("not enough tokens... \n");
		return;
	}

	//set the environment
	if (setenv(tokens[1], tokens[2], 0)) {
		printf("setenv: %s\n", strerror(errno));
	} else {
		//keep track of those variables
		environ[env_size] = malloc(BUF_SZ);
		strcpy(environ[env_size], tokens[1]);
		strcat(environ[env_size], "=");
		strcat(environ[env_size++], tokens[2]);
		environ[env_size] = NULL;
	}
}

int check_built_in(struct command_line * cmds, int comm_sz, char * line) {
	//add our stuff to the history
	add_to_history(cmds, comm_sz, line);

	if (strcmp(cmds[0].tokens[0], "setenv") == 0) {
		change_env(cmds[0].tokens, cmds[0].tokens_size);
		return 1;
	} else if (strcmp(cmds[0].tokens[0], "set") == 0) {
		char * curr;
		int index = 0;

		while ((curr = environ[index++]) != NULL) {
			printf("%s\n", curr);
		}

		return 1;
	} else if (strcmp(cmds[0].tokens[0], "cd") == 0) {
		change_dir(cmds[0].tokens, cmds[0].tokens_size);
		return 1;
	} else if (strcmp(cmds[0].tokens[0], "history") == 0) {
		print_history();

		return 1;
	} else if (cmds[0].tokens[0][0] == '!') {
		history_exec(cmds, line);
		return 1;
	}
	return 0;
}

void free_env(void) {
	if (env_size > 0) {
		int i;
		for (i = 0; i < env_size; i++) {
			free(environ[i]);
		} 
	}
}

void decrease_comm_num(void) {
	comm_num--;
}