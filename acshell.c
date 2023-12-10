// This is a class project for CS421 Operating System class (Spring 2023)
// The code template is provided by Dr. Kevin Brown at California State University, East Bay

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h> // for waitpid()

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 50
#define _XOPEN_SOURCE 700 /* for sigaction() */

static char buffer[BUFFER_SIZE]; // Buffer for the signal handler function

pid_t foreground_pid; // The pid of the foreground process
pid_t shell_pid;	  // The pid of the shell process

/* the signal handler function for Ctrl-\ */
void handle_SIGQUIT()
{

	// Check if there exists a foreground process and make sure the thecurrent process is not a child process foreground process is not the shell process
	if (foreground_pid != 0 && foreground_pid != shell_pid)
	{
		// Send the SIGQUIT signal to the foreground process
		kill(foreground_pid, SIGQUIT);
	}

	write(STDOUT_FILENO, buffer, strlen(buffer));

	// exit(0);
}

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

void setup(char inputBuffer[], char *args[], int *background)
{
	int length, /* # of characters in the command line */
		i,		/* loop index for accessing inputBuffer array */
		start,	/* index where beginning of next command parameter is */
		ct;		/* index of where to place the next parameter into args[] */

	ct = 0;

	/* read what the user enters on the command line */
	length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

	start = -1;
	if (length == 0)
		exit(0); /* ^d was entered, end of user command stream */
	if (length < 0)
	{
		perror("error reading the command");
		exit(-1); /* terminate with error code of -1 */
	}

	/* examine every character in the inputBuffer */
	for (i = 0; i < length; i++)
	{
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t': /* argument separators */
			if (start != -1)
			{
				args[ct] = &inputBuffer[start]; /* set up pointer */
				ct++;
			}
			inputBuffer[i] = '\0'; /* add a null char; make a C string */
			start = -1;
			break;
		case '\n': /* should be the final char examined */
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; /* no more arguments to this command */
			break;
		default: /* some other character */
			if (start == -1)
				start = i;
			if (inputBuffer[i] == '&')
			{
				*background = 1;
				start = -1;
				inputBuffer[i] = '\0';
			}
		}
	}
	args[ct] = NULL; /* just in case the input line was > 80 */
}

int main(void)
{
	char inputBuffer[MAX_LINE];		/* buffer to hold the command entered */
	int background;					/* equals 1 if a command is followed by '&' */
	char *args[(MAX_LINE / 2) + 1]; /* command line (of 80) has max of 40 arguments */
	int promptCounter = 1;			/* count the number of prompt shown so far */
	pid_t pid;						// Process ID of the child process created by fork()
	pid_t receiver_pid;				// The pid specified by the user to receive the signal
	struct sigaction handler;		/* set up the signal handler */

	shell_pid = getpid();
	// Print greeting and its pid when the shell starts up
	printf("Welcome to acshell. My pid is %d.\n", shell_pid);

	while (1)
	{ /* Program terminates normally inside setup */
		background = 0;
		printf("acshell[%d]:\n", promptCounter);

		// Catch the Ctrl-\ signal and pass it on to the foreground process (if it exists)
		handler.sa_handler = handle_SIGQUIT;
		handler.sa_flags = SA_RESTART;
		sigaction(SIGQUIT, &handler, NULL);
		strcpy(buffer, "Caught <ctrl><\\>\n");

		setup(inputBuffer, args, &background); /* get next command */

		// Check if the command is a built-in command
		// If the command is stop, stop a running background process by sending it the SIGSTOP signal
		if (strcmp(args[0], "stop") == 0)
		{
			// Use atoi(args[1]) to convert the arg[1] to a pid_t type
			receiver_pid = atoi(args[1]);

			// Use kill(pid, 0) to check if the pid is valid
			if (kill(receiver_pid, 0) == 0)
			{
				// If the pid is valid, send the SIGSTOP signal to the process
				kill(receiver_pid, SIGSTOP);
			}
			else
			{
				// If the pid is not valid, print an error message
				printf("Error: Invalid pid\n");
			}
		}
		// If the command is bg, resume a stopped background process by sending it the SIGCONT signal
		else if (strcmp(args[0], "bg") == 0)
		{
			// Use atoi(args[1]) to convert the arg[1] to a pid_t type
			receiver_pid = atoi(args[1]);

			// Use kill(pid, 0) to check if the pid is valid
			if (kill(receiver_pid, 0) == 0)
			{
				// If the pid is valid, send the SIGCONT signal to the process
				kill(receiver_pid, SIGCONT);
			}
			else
			{
				// If the pid is not valid, print an error message
				printf("Error: Invalid pid\n");
			}
		}
		// If the command is fg, resume a stopped background process by sending it the SIGCONT signal and wait for it to complete
		else if (strcmp(args[0], "fg") == 0)
		{
			// Use atoi(args[1]) to convert the arg[1] to a pid_t type
			receiver_pid = atoi(args[1]);

			// Use kill(pid, 0) to check if the pid is valid
			if (kill(receiver_pid, 0) == 0)
			{
				// If the pid is valid, send the SIGCONT signal to the process
				kill(receiver_pid, SIGCONT);
				// Set the foreground_pid to the pid brought to the foreground
				foreground_pid = receiver_pid;
			}
			else
			{
				// If the pid is not valid, print an error message
				printf("Error: Invalid pid\n");
			}
			// Parent waits for the child with the specified pid to finish
			waitpid(receiver_pid, NULL, 0);
			// Print a message when the child process completes
			printf("Child process %d complete\n", receiver_pid);
		}
		// If the command is kill, kill a running background process by sending it the SIGKILL signal
		else if (strcmp(args[0], "kill") == 0)
		{
			// Use atoi(args[1]) to convert the arg[1] to a pid_t type
			receiver_pid = atoi(args[1]);

			// Use kill(pid, 0) to check if the pid is valid
			if (kill(receiver_pid, 0) == 0)
			{
				// If the pid is valid, send the SIGKILL signal to the process
				kill(receiver_pid, SIGKILL);
			}
			else
			{
				// If the pid is not valid, print an error message
				printf("Error: Invalid pid\n");
			}
		}

		// If the command is exit, prints a message "acshell exiting" and exits the shell
		else if (strcmp(args[0], "exit") == 0)
		{
			printf("acshell exiting\n");
			// Wait for all child processes to complete before exiting
			if (background == 0)
			{
				// Parent waits for the child to complete
				waitpid(pid, NULL, 0);
			}
			exit(0);
		}
		// If the command is not a built-in command, fork a child process and execute the command
		else
		{
			// Fork a child process for the command
			pid = fork();

			// Parent prints a message indicating the pid of the child and whether the command is running in the foreground or background
			if (pid > 0)
			{
				printf("[Child pid = %d, background = %s]\n", pid, (background == 1) ? "TRUE" : "FALSE");
			}

			// If fork() fails, print an error message and exit
			if (pid < 0)
			{
				fprintf(stderr, "Fork Failed");
				return 1;
			}
			// If the child process is created successfully, execute the command
			if (pid == 0)
			{
				// Execute the command
				execvp(args[0], args);
			}
			// Parent process
			else
			{
				// If the command is not a background command, the parent process should wait for the child process to complete
				if (background == 0)
				{
					// Parent waits for the child with the given pid to complete
					waitpid(pid, NULL, 0);
					// Print a message when the child process completes
					printf("Child process %d complete\n", pid);
				}
			}
		}

		// Increment the prompt counter
		promptCounter++;

		/* the steps are:
		   (0) if built-in command, handle internally
		   (1) if not, fork a child process using fork()
		   (2) the child process will invoke execvp()
		   (3) if background == 0, the parent will wait,
		   otherwise returns to the setup() function. */
	}
}
