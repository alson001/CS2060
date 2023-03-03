/**
 * CS2106 AY22/23 Semester 2 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "myshell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void add_process(pid_t pid);
void update_terminate(pid_t pid, int status);
void update_wait(pid_t pid, int index);
void update_redirect(int len, char **args);
void parse(char **cmd, char ***arguments);
void update_exit();

struct PCBTable processes[MAX_PROCESSES];
int nProcesses;


/*******************************************************************************
 * Signal handler : ex4
 ******************************************************************************/

// static void signal_handler(int signo) {

//         // Use the signo to identy ctrl-Z or ctrl-C and print “[PID] stopped or print “[PID] interrupted accordingly.
//         // Update the status of the process in the PCB table 

// }



static void proc_update_status(pid_t pid, int status, int exitCode) {

    // Call everytime you need to update status and exit code of a process in PCBTable
    // May use WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG, WIFSTOPPED
    
    // Find the pid in the processes array
    // Set its status and exitcode as stated
    for (int i = 0; i < nProcesses; i++) {
        if (processes[i].pid == pid) {
            processes[i].status = status;
            processes[i].exitCode = exitCode;
            break;
        }
    }
}


/*******************************************************************************
 * Built-in Commands
 ******************************************************************************/

static void command_info(char **args) {

    // Edge case: if person types "info" command with no arguments after
    if (args[1] == NULL) {
        fprintf(stderr, "Wrong command\n");
        return;
    }

    // Parse the option.
    int option = atoi(args[1]);
    int numExited, numRunning, numTerminating, numStopped;   
    // Perform the requested action.
    switch (option) {
        case 0:
            // Print details of all processes in the order in which they were run.
            // You will need to print their process IDs, their current status (Exited,
            // Running, Terminating, Stopped), and for Exited processes, their exit codes.
            for (int i = 0; i < nProcesses; i++) {
                if (processes[i].pid != -1) {
                    int status;
                    int result = waitpid(processes[i].pid, &status, WNOHANG);
                    if (result == processes[i].pid) {
                        int exitCode = WEXITSTATUS(status);
                        proc_update_status(processes[i].pid, status, exitCode);
                    }
                    printf("[%d] ", processes[i].pid);
                    switch (processes[i].status) {
                        case 1:
                            printf("Exited %d\n", processes[i].exitCode);
                            break;
                        case 2:
                            printf("Running\n");
                            break;
                        case 3:
                            printf("Terminating\n");
                            break;
                        case 4:
                            printf("Stopped\n");
                            break;
                    }
                }
            }
            break;
        case 1:
            // Print the number of exited processes.
            numExited = 0;
            for (int i = 0; i < nProcesses; i++) {
                if (processes[i].status == 1) {
                    numExited++;
                }
            }
            printf("Exited processes: %d\n", numExited);
            break;
        case 2:
            // Print the number of running processes.
            numRunning = 0;
            for (int i = 0; i < nProcesses; i++) {
                if (processes[i].status == 2) {
                    numRunning++;
                }
            }
            printf("Running processes: %d\n", numRunning);
            break;
        case 3:
            // Print the number of terminating processes.
            numTerminating = 0;
            for (int i = 0; i < nProcesses; i++) {
                if (processes[i].status == 3) {
                    numTerminating++;
                }
            }
            printf("Terminating processes: %d\n", numTerminating);
            break;
        case 4:
            // Print the number of stopped processes.
            numStopped = 0;
            for (int i = 0; i < nProcesses; i++) {
                if (processes[i].status == 4) {
                    numStopped++;
                }
            }
            printf("Stopped processes: %d\n", numStopped);
            break;
        default:
            // Invalid option.
            fprintf(stderr, "Wrong Command\n");
            break;
    }
}

static void command_wait(char **args) {

    // Edge case: No specified PID and invalid wait call (too many arguments)
    if (args[1] == NULL || args[2] != NULL) {
        free(args);
        return;
    }

    pid_t pid = atoi(args[1]);
    for (int i = 0; i < nProcesses; i++) {
    // If process is RUNNING:
        // Wait it by using update_wait()  
        if (processes[i].pid == pid) {
            if (processes[i].status == 2) {
                update_wait(processes[i].pid, i);
            }
            break;
        }
    }
}


static void command_terminate(char **args) {

    // Terminate
    char **arguments = NULL;
    parse(args, &arguments);

    // Edge cases: No specified PID and invalid terminate call (too many arguments)
    if (arguments[1] == NULL || arguments[2] != NULL) {
        free(arguments);
        return;
    }

    int pid = atoi(arguments[1]);
    // Find the pid in the processes array
    for (int i = 0; i < nProcesses; i++) {
    // If process is RUNNING:
        // Terminate it by using update_terminate()
        if (processes[i].pid == pid)  {
            if (processes[i].status == 2) {
                update_terminate(processes[i].pid, 3);
            }
            break;
        }
    }
    free(arguments);
}

// static void command_fg(/* pass necessary parameters*/) {

//         /******* FILL IN THE CODE *******/
        

//     // if the {PID} status is stopped
//         //Print “[PID] resumed”
//         // Use kill() to send SIGCONT to {PID} to get it continue and wait for it
//         // After the process terminate, update status and exit code (call proc_update_status())
// }


/*******************************************************************************
 * Program Execution
 ******************************************************************************/

static void command_exec(int len, char **args) {
    char **arguments = NULL;
    char *program = args[0];
    parse(args, &arguments);

    // check if program exists and is executable : use access()
    if (access(program, R_OK) == 0 && access(program, X_OK) == 0) {
        
        // fork a subprocess and execute the program
        pid_t pid;

        if ((pid = fork()) == 0) {
            // CHILD PROCESS
            // register the process in process table
            add_process(pid);

            for (int i = 0; i < len; i++) {
                if (strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0 ||  
                    strcmp(args[i], "2>") == 0) {
                    update_redirect(len, args);
                    break;
                }
            }

            execv(program, arguments);
            free(arguments);
            // Exit the child
            exit(0);
        } else {
            // PARENT PROCESS
            // register the process in process table
            add_process(pid);
            int status;
            int isBackground = 0;
            if (len > 1 && strcmp(args[len - 1], "&") == 0) {
                isBackground = 1;
            }
            if (isBackground) {
                // print Child [PID] in background
                // Use waitpid() with WNOHANG when not blocking during wait
                printf("Child [%d] in background\n", pid);
                waitpid(pid, &status, WNOHANG);
            } else {  
                // Use waitpid() with WUNTRACED when parent needs to block due to wait
                waitpid(pid, &status, WUNTRACED);
            }
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                proc_update_status(pid, 1, exitCode);
            }
        }
    } else {
        fprintf(stderr, "%s not found\n", program);
    }
    free(arguments);
}

/*******************************************************************************
 * Command Processor
 ******************************************************************************/

static void command(int len,char **commands) {


        /******* FILL IN THE CODE *******/

    if (commands == NULL) {
        return;
    }

    char *process = commands[0];
        /******* FILL IN THE CODE *******/
    
    if (strcmp(process, "info") == 0) {
        // if command is "info" call command_info()             : ex1
        command_info(commands);
    } else if (strcmp(process, "wait") == 0) {
        // if command is "wait" call command_wait()             : ex2 
        command_wait(commands);
    } else if (strcmp(process, "terminate") == 0) {
        // if command is "terminate" call command_terminate()   : ex2
        command_terminate(commands);
    } else if (strcmp(process, "quit") == 0) {
        // if command is "quit" call my_quit()                  : ex1 
        my_quit();
    } else {
        // call command_exec() for all other commands           : ex1, ex2, ex3
        command_exec(len, commands);
    }

    // if command is "fg" call command_fg()                 : ex4
}

/*******************************************************************************
 * High-level Procedure
 ******************************************************************************/

void my_init(void) {

        /******* FILL IN THE CODE *******/

    nProcesses = 0;
    // use signal() with SIGTSTP to setup a signalhandler for ctrl+z : ex4
    // use signal() with SIGINT to setup a signalhandler for ctrl+c  : ex4

    // anything else you require

    // SIGCHLD signal handler for handling child processes
    signal(SIGCHLD, update_exit);
}

void my_process_command(size_t num_tokens, char **tokens) {

    // Determine the number of commands to execute
    int nCommands = 0;
    for (size_t i = 0; i < num_tokens - 1; i++) {
        if (strcmp(tokens[i], (char *) ";") != 0) {
            continue;
        }
        nCommands++;
    }
    nCommands++;

    // Execute each command in the input
    int argIndex = 0;
    for (int i = 0; i < nCommands; i++) {
        char **commandArgs = malloc(sizeof(char *) * num_tokens);
        int numArgs = 0;

        //if not NULL || ";", increase length of the argument
        while (tokens[argIndex] != NULL && strcmp(tokens[argIndex], (char *) ";") != 0) {
            commandArgs[numArgs] = tokens[argIndex];
            argIndex++;
            numArgs++;
        }

        commandArgs[numArgs] = NULL;
        command(numArgs, commandArgs);

        argIndex++;
        free(commandArgs);
    }

}

void my_quit(void) {


    /******* FILL IN THE CODE *******/
    // Kill every process in the PCB that ics either stopped or running
    for (int i = 0; i < nProcesses; i++) {
        if (processes[i].status == 2  || processes[i].status == 4) {
            printf("Killing [%d]\n", processes[i].pid);
            kill(processes[i].pid, SIGTERM);
        }
    }
    printf("\nGoodbye\n");
}
/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void add_process(pid_t pid) {
    processes[nProcesses].pid = pid;
    processes[nProcesses].status = 2;
    processes[nProcesses].exitCode = -1;
    nProcesses++;
}

void update_terminate(pid_t pid, int status) {
    int exitCode = kill(pid, SIGTERM);
    for (int i = 0; i < nProcesses; i++) {
        if (processes[i].pid == pid) {
            processes[i].status = status;
            processes[i].exitCode = exitCode;
            break;
        }
    }
}

void update_wait(pid_t pid, int index) {
    // If the process indicated by the process id is RUNNING, wait for it (can use waitpid()).
    int status;
    waitpid(pid, &status, WUNTRACED);
    // After the process terminate, update status and exit code (call proc_update_status())
    if (WIFEXITED(status)) {
        processes[index].status = 1;
    } else if (WIFSIGNALED(status)) {
        processes[index].status = 3;
    } else {
        processes[index].status = 2;
    }
    if (WIFEXITED(status)) {
        int exitCode = WEXITSTATUS(status);
        processes[index].exitCode = exitCode;
    } else {
        processes[index].exitCode = -1;
    }
}

void update_redirect(int len, char **args) {

    // Initialize an array of 3 to track if "<", ">" or "2>" is used 
    char **files = malloc(sizeof(char *) * 3);

    // Set each element in array to null
    for (int i = 0; i < 3; i++) {
        files[i] = NULL;
    }

    for (int i = 0; i < len; i++) {
        if (strcmp(args[i], (char *) "<") == 0) {
            files[0] = args[i + 1];
        } else if (strcmp(args[i], (char *) ">") == 0) {
            files[1] = args[i + 1];
        } else if (strcmp(args[i], (char *) "2>") == 0) {
            files[2] = args[i + 1];
        }
    }

    // Handle STDIN
    if (files[0] != NULL) {
        int fds = open(files[0], O_RDONLY);
        if (fds == -1) {
            fprintf(stderr, "%s does not exist\n", files[0]);
            free(files);
            // Exit with error for child process
            exit(1);
        }

        dup2(fds, STDIN_FILENO);
        close(fds);
    }

    // Handle STDOUT
    if (files[1] != NULL) {
        int fds = open(files[1], O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
        dup2(fds, STDOUT_FILENO);
        close(fds);
    }

    // Handle STDERR
    if (files[2] != NULL) {
        int fds = open(files[2], O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
        dup2(fds, STDERR_FILENO);
        close(fds);
    }

    // call execv() to execute the command in the child process
    free(files);
}


void parse(char **args, char ***arguments) {

    // calloc allocates the requested memory and returns a pointer to it
    char **arr = (char**) calloc(1, sizeof(char *));
    
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], (char *) "&") == 0) {
            break;
        } else if (strcmp(args[i], (char *) "<") == 0 || strcmp(args[i], (char *) ">") == 0 
                  || strcmp(args[i], (char *) "2>") == 0) {
            break;
        } else {
            arr[i] = args[i];
            i++;

            // realloc to dynamically change memory allocation of array
            arr = realloc(arr, (i + 1) * sizeof(char *));
        }
    }
    // Add NULL for termination for each command
    arr[i] = NULL;
    *arguments = arr;
}

void update_exit() {
    pid_t pid;
    int exitCode;
    pid = wait(&exitCode);
    if (pid > 0) {
        for (int i = 0; i < nProcesses; i++) {
            if (processes[i].pid == pid) {
                processes[i].status = 1;
                processes[i].exitCode = exitCode;
                break;
            }
        }
    }
}
