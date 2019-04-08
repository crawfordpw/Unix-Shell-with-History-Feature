/*===========================================================================
Authors: Patrick Crawford
For: Operating Systems 5129/6029
Date last modified: November 29, 2016
Project: Unix shell with history feature
Description: This is a Unix based shell that can handle a command up to 80 
             characters in length. It also supports commands with any number of
             of arguments, up to 41 characters in length. It allows for basic
             redirection, piping, and two commands. Also has a history feature
             where the last 10 commands entered are stored and can be executed
             again by a separate command. 
=============================================================================*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_LINE 80 /* the maximum length of command */
#define HISTORY_SIZE 11 /* history */
#define READ_END 0  /* used for piping */
#define WRITE_END 1

char *history[HISTORY_SIZE][MAX_LINE] = { '\0' }; /* history file */
int total = 0; /* total number of commands entered */

int parseInput(char input_buffer[], char *args[], int *concurrent, int *recent);
int parseArguments(char *args[], char *cmd1[], char *cmd2[], int argument);
void putHistory(char *input_buffer);
void printHistory();
void isPipe(char *cmd1[], char *cmd2[]);
void isRedirection(char *cmd1[], char **file);
void isTwo(char *cmd1[], char *cmd2[], int *concurrent);
void isSingle(char *args[], int *concurrent);
void runN(char *args[], char input_buffer[], int *recent);
void runHelp();

int main(void)
{
	char input_buffer[MAX_LINE];
	char *args[MAX_LINE/2 +1]; /* command line arguments */
	char *cmd1[MAX_LINE/2 +1]; /* split args into seperate command */
	char *cmd2[MAX_LINE/2 +1]; /* split args into seperate command */
	int concurrent; /* flag to determine if child process runs concurrent with parent */
	int recent = 0; /* flag to determine to run most recent command */
	int should_run = 1; /* flag to determine when to exit program */
	int argument; /* number of arguments */
	int result; /* either one command, pipe, redirection, or two commands */
	int rflag = 1; /* flag for !!. 1 means no other commands entered yet */
	int i;
	
	while(should_run){
	
		if(recent == 0){
			printf("osh>");
			fflush(stdout);
		}
		
		concurrent = 0;
		
		/* get user input and parse as necessary */
		argument = parseInput(input_buffer, args, &concurrent, &recent);
		if(argument == 0) continue;
		result = parseArguments(args,cmd1,cmd2,argument);
		recent = 0;
		
		if(strcmp(args[0], "exit") == 0) /* if command is "exit" */
			should_run = 0;
		else if(strcmp(args[0], "history") == 0){ /* if command is "history" */
			printHistory();
			rflag = 0;
		}
		else if(strcmp(args[0], "!!") == 0){ /* in command is "!!" */
			if(rflag) fprintf(stderr, "No recent command found\n");
			else recent = 1;
		}
		else if(args[0][0] == '!' && isdigit(args[0][1])){ /* if command is "!n" */
			runN(args, input_buffer, &recent);
			rflag = 0;
		}
		else if(strcmp(args[0], "cd") == 0){ /* if command is "cd" */
			if (args[1] == NULL)
    			fprintf(stderr, "expected argument to \"cd\"\n");
  			else {
    			if (chdir(args[1]) != 0)
      				perror("osh");
  			}
			rflag = 0;
		}
		else if(strcmp(args[0], "help") == 0){ /* if command is "help" */
			runHelp();
		    rflag = 0;	
		}	
		else{
			if(result == 1) /* Pipe */
				isPipe(cmd1, cmd2);
			else if(result == 2) /* Redirection */
				isRedirection(cmd1, cmd2);	
			else if(result == 3) /* Two commands */
				isTwo(cmd1, cmd2, &concurrent);	
			else /* Single command */
				isSingle(args, &concurrent);
			rflag = 0;
		} /* end else */		
	} /* end while */
	return 0;
}

/* parses input from command line into useful sections that can be more easy to work with */
int parseInput(char input_buffer[], char *args[], int *concurrent, int *recent)
{
	int length; /* length of user input */
	int i; /* loop index */
	int argument = 0; /* number of arguments */
	char *token; /* splits user input into tokens */
	
	if(*recent == 0){
		length = read(STDIN_FILENO,input_buffer,MAX_LINE); /* get user input */
		input_buffer[length-1] = '\0'; /* ensures input is null terminated */
	}
	else if(*recent == 1){
		memcpy(input_buffer,history[1],MAX_LINE);
	}
	
	putHistory(input_buffer); /* place command in history */
	total++;
	
	/* parses input in args and determines if user entered & */
	token = strtok(input_buffer, " \t\n");
	while (token != NULL) {
		if(strcmp(token, "&") == 0)
			*concurrent = 1;
    	else{
    		args[argument] = token;
    		argument++;
    	}
    	token = strtok(NULL, " \t\n");
    }
    args[argument] = '\0'; /* ensure args is null at the end. needed for execvp */
    
    return argument;
}

/* parse the arguments made from parseInput for execvp to work properly */
int parseArguments(char *args[], char *cmd1[], char *cmd2[], int argument)
{
	int res = 0; /* flag for execute */
	int split;  /* where to split commands */
	int i;  /* loop index */
	int count = 0;
	
	
	/* determine flag and where to split if necessary*/
	for (i = 0; i < argument; i++) {
		if (strcmp(args[i], "|") == 0) {  /* Pipe */
			res = 1;
			split = i;
		}
    	else if (strcmp(args[i], ">>") == 0) {   /* Redirection */
      		res = 2;
      		split = i;
    	}
    	else if (strcmp(args[i], "&&") == 0) {   /* Two commands */
      		res = 3;
      		split = i;
    	}
  	} /* end for loop */
  	
  	
  	/* assigning cmd1 and cmd2 if pipe, redirection, or two commands found */
  	if(res > 0){
  		for (i = 0; i < split; i++){
      		cmd1[i] = args[i];
      	}	
      	for (i = split+1; i < argument; i++) {
			cmd2[count] = args[i];
			count++;
    	}
    	cmd1[split] = '\0'; /* ensure cmd ends with a NULL */
    	cmd2[count] = '\0';
  	}
  	return res;
}

/* places new command into history. keeps most recent 10 */
void putHistory(char *input_buffer)
{
	int i;
	
	for(i = HISTORY_SIZE - 1; i > 0; i--){
		memcpy(history[i],history[i-1],MAX_LINE);
	}
	memcpy(history[0],input_buffer,MAX_LINE);
}

/* prints the 10 most recent commands */
void printHistory()
{	
	int i;
	int hCount = total;
	
	if(hCount >= HISTORY_SIZE - 1) hCount = 10;
	
	for(i = hCount; i > 0; i--){ 
		printf("%d %s\n", i, (char*)history[i-1]);
		hCount--;
	}
}

/* creates two child processes to pipe result of cmd1 to cmd2 */
void isPipe(char *cmd1[], char *cmd2[])
{
	int fd[2];
	pid_t pid1, pid2;

	/* creates pipe */
	if(pipe(fd) == -1){
		fprintf(stderr, "Pipe failed");
	}

	pid1 = fork();
	if(pid1 < 0){ /* child process 1 failed */
		fprintf(stderr, "Fork Failed");
	}
	if(pid1 == 0){	  /* child process 1 */
		dup2(fd[READ_END], 0); /* reassign stdin to read end */
    	close(fd[WRITE_END]);	/* close write end */

    	if(execvp(cmd2[0], cmd2) == -1)
				fprintf(stderr, "Error executing command\n");
		exit(0);
  	} 
	else { /* parent process */
		pid2 = fork();
		if(pid1 < 0){ /* child process 2 failed */
			fprintf(stderr, "Fork Failed");
		}
		if(pid2 == 0){ /* child process 2 */
    		dup2(fd[WRITE_END], 1); /* reassign stdout to write end */
    		close(fd[READ_END]); /* close read end */

    		if(execvp(cmd1[0], cmd1) == -1)
				fprintf(stderr, "Error executing command\n");
			exit(0);
		} 
		else{ /* parent process */
			wait(NULL);
			close(fd[READ_END]);
    		close(fd[WRITE_END]);
    	}
    	wait(NULL);
	}
}

/* creates two child processes to redirect result of cmd1 to cmd2 file */
void isRedirection(char *cmd[], char **file)
{
	int fd[2];
	int fil;
	int count;
	char c;
	pid_t pid1, pid2;

	/* creates pipe */
	if(pipe(fd) == -1){
		fprintf(stderr, "Pipe failed");
	}

	pid1 = fork();
	if(pid1 < 0){ /* child process 1 failed */
		fprintf(stderr, "Fork Failed");
	}
	if(pid1 == 0){	  /* child process 1 */
		fil = open(file[0], O_RDWR | O_CREAT, 0666);
		
		if (fil < 0) {
      		fprintf(stderr, "Failed to open file");
    	}
    	
		dup2(fd[READ_END], 0); /* reassign stdin to read end */
    	close(fd[WRITE_END]);	/* close write end */

    	while ((count = read(0, &c, 1)) > 0)
      		write(fil, &c, 1);
      	exit(0);
  	} 
	else { /* parent process */
		pid2 = fork();
		if(pid1 < 0){ /* child process 2 failed */
			fprintf(stderr, "Fork Failed");
		}
		if(pid2 == 0){ /* child process 2 */
    		dup2(fd[WRITE_END], 1); /* reassign stdout to write end */
    		close(fd[READ_END]); /* close read end */

    		if(execvp(cmd[0], cmd) == -1)
				fprintf(stderr, "Error executing command\n");
			exit(0);
		} 
		else{ /* parent process */
    		wait(NULL);
			close(fd[READ_END]);
    		close(fd[WRITE_END]);
    	}
        wait(NULL);
	}
}

/* creates two child processes to run seperate commands */
void isTwo(char *cmd1[], char *cmd2[], int *concurrent)
{
	pid_t pid1, pid2;
	pid1 = fork();
	if(pid1 < 0){ /* child process 1 failed */
		fprintf(stderr, "Fork Failed");
	}
	if(pid1 == 0){	  /* child process 1 */
    	if(execvp(cmd1[0], cmd1) == -1)
			fprintf(stderr, "Error executing command\n");
		exit(0);
  	} 
	else { /* parent process */
		pid2 = fork();
		if(pid2 < 0){ /* child process 2 failed */
			fprintf(stderr, "Fork Failed");
		}
		if(pid2 == 0){ /* child process 2 */
    		if(execvp(cmd2[0], cmd2) == -1)
				fprintf(stderr, "Error executing command\n");
			exit(0);
		} 
		else{ /* parent process */
			wait(NULL);
    	}
    	wait(NULL);
	}
}

/* runs a single command */
void isSingle(char *args[], int *concurrent)
{
	pid_t pid;
	pid = fork(); /* fork a process */
		
	if(pid < 0){ /* error occured */
		fprintf(stderr, "Fork Failed\n");;
	}
	if(pid == 0){ /* child process to run command */
		if(execvp(args[0], args) == -1)
			fprintf(stderr, "Error executing command\n");
		exit(0);
	}
	else { /* parent process */
		if(*concurrent == 0) wait(NULL);
	}
}

/* runs the Nth command */
void runN(char *args[], char input_buffer[], int *recent)
{
	int i;
	int index = 0;
	
	/* converts argument to an int and basic error handeling */
	index = atoi(&args[0][1]);
	if(index > HISTORY_SIZE - 1 || index == 0){
		printf("error - enter command betweeen 1-10\n");
	}
	else if(total - 1 < index || total == 0){
		printf("error - command not found at that index\n");
	}
	else {
		memcpy(input_buffer,history[index],MAX_LINE);
		*recent = 2; /* ensures memcpy doesn't run twice in parseInput */
	}
}

/* help dialog when "help" command is entered */
void runHelp()
{
	printf("    osh help: This is a simple shell with history feature.\n\n"
			"\tCurrently does not support tab complete or arrow keys.\n"
			"\tIncorrectly using \"cat\" command, especially with \"|\",\n"
			"\t    may cause shell to behave unpredicatably.\n" 
			"\tCharacter limit of 80.\n"
			"\tCan use arguments with commands.\n\n"  
			"\t1 Ctrl-C: terminates osh\n" 
			"\t2 exit: ends current session of osh\n"
			"\t3 history: displays 10 most recent commands\n"
			"\t4 !!: execute most recent command\n"
			"\t5 !n: execute nth command up to 10\n"
			"\t6 cd ...: change directory to location ...\n"
			"\t7 command1 && command2: execute command1 and command2\n"
			"\t8 command1 | command2: output of command1 is input to command2\n"
			"\t9 command >> filename: overwrites output of command to a file\n"
	);
}
