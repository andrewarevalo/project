#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
//test change
//define the max size of the command line, usually when you run any kind of c program it runs its own terminal 
// returns with some value and exits 
#define CMDLINE_MAX 512
#define MAX_ARGS 16
#define MAX_TOKEN_LENGTH 32


// we can already use special commands and execute external programs. 
//get familiar with: standard c library for pipe, execlp, fork, waitpid, 

int main(void)
{
        char cmd[CMDLINE_MAX];
        char* args[MAX_ARGS];
        // need to dynamically allocate 2d array to avoid warning about type in execv
        // it exepcts a const char* [] which making args[X][Y] does not do
        for(int i = 0; i < MAX_ARGS; i++) {
                args[i] = malloc(sizeof(char)*MAX_TOKEN_LENGTH);
        }
        // we don't just want one command
        while (1) {
                char *nl;
                int retval;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout); // type in characters, waits for the buffer to be filled or \n. so We want to print the prompt and we want the buffer for stdout to be empty
                                // usually matters when taking input, so when taking input, we want the buffer to be empty

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n'); // finds the newline character of the command 
                if (nl)
                        *nl = '\0'; // finds the position of the newline character and sets it to null. Should be the last character of the command

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } // if read in exit then break out of shell 
                // TODO handle built in commands pwd, cd, sls

                // split the command up into an array using whitespace to separate
                char* pch;
                // count how many tokens were parsed
                int argcount = 0;
                // gets the first token (splits on a space)
                pch = strtok(cmd, " ");
                // as long as there are tokens remaining
                while(pch != NULL) {
                        //printf("%s\n", pch);
                        // copy this token into the args array
                        strcpy(args[argcount], pch);
                        argcount++;
                        // get the next token
                        pch = strtok(NULL, " ");
                }
                // TODO handle |, > and >>

                // after tokens have been split, add NULL to end
                args[argcount] = NULL;
                // fork() runs two processes in parallel, the one with pid == 0 is the child
                // the one with pid > 0 is the parent
                int pid = fork();
                if (pid == 0) {
                        /* Child */
                        execvp(args[0], args);
                        perror("execv");
                        exit(1);
                } else if (pid > 0) {
                        /* Parent */
                        int status;
                        waitpid(pid, &status, 0);
                        printf("* completed '");
                        for(int i = 0; i < argcount; i++) {
                                printf("%s ", args[i]);
                        }
                        printf("' [%d]\n", WEXITSTATUS(status));
                } else {
                        perror("fork");
                        exit(1);
                }

                /* Regular command - this is where we start working on the program, implementing the system */
                //retval = system(cmd);
                
        }
        // free dynamically allocated memory
        for(int i = 0; i < MAX_ARGS; i++) {
                free(args[i]);
        }

        return EXIT_SUCCESS;
}
