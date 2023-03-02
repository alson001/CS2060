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
void arg_parse(char **cmd, char ***arguments);
int ampersand_command(char **tokens);
void proc_exit();

struct PCBTable processes[MAX_PROCESSES];
int nProcesses;


/*******************************************************************************
 * Signal handler : ex4
 ******************************************************************************/

// static void signal_handler(int signo) {

//         // Use the signo to identy ctrl-Z or ctrl-C and print “[PID] stopped or print “[PID] interrupted accordingly.
//         // Update the status of the process in the PCB table 

// }



static void proc_update_status(pid_t pid, int status, int exit_code) {

    /******* FILL IN THE CODE *******/
    // Call everytime you need to update status and exit code of a process in PCBTable
    // May use WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG, WIFSTOPPED
    for (int i = 0; i < nProcesses; i++) {
        if (processes[i].pid == pid) {
            processes[i].status = status;
            processes[i].exitCode = exit_code;
            break;
        }
    }
}


/*******************************************************************************
 * Built-in Commands
 ******************************************************************************/

static void command_info(char **commands) {
    if (commands[1] == NULL) {
        fprintf(stderr, "Wrong command\n");
        return;
    }

    // If option is 0
        //Print details of all processes in the order in which they were run. You will need to print their process IDs, their current status (Exited, Running, Terminating, Stopped)
        // For Exited processes, print their exit codes.
    // If option is 1
        //Print the number of exited process.
    // If option is 2
        //Print the number of running process.
    // If option is 3
        //Print the number of terminating process.
    // If option is 4
        //Print the number of stopped process.

    //For all other cases print “Wrong command” to stderr.
    // Parse the option.
    int option = atoi(commands[1]);
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
                        int exit_code = WEXITSTATUS(status);
                        proc_update_status(processes[i].pid, status, exit_code);
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
            fprintf(stderr, "Error: Invalid option.\n");
            break;
    }
}

static void command_wait(char **args) {

  if (args[1] == NULL) {
        return;
    }

    pid_t pid = atoi(args[1]);
    for (int i = 0; i < nProcesses; i++) {
        if (processes[i].pid == pid) {
            if (processes[i].status == 2) {
    // If the process indicated by the process id is RUNNING, wait for it (can use waitpid()).
                int status;
                waitpid(pid, &status, WUNTRACED);
    // After the process terminate, update status and exit code (call proc_update_status())
                processes[i].status = WIFEXITED(status) ? 1 : (WIFSIGNALED(status) ? 3 : 2);
                processes[i].exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            }
            break;
        }
    }
}


static void command_terminate(char **cmd) {
        /******* FILL IN THE CODE *******/
    char **arg = NULL;
    arg_parse(cmd, &arg);

    // Handle edge case
    if (arg[1] == NULL || arg[2] != NULL) {
        free(arg);
        return;
    }

    int pid = atoi(arg[1]);
    // Find the pid in the PCBTable
    for (int i = 0; i < nProcesses; i++) {
    // If {PID} is RUNNING:
        //Terminate it by using kill() to send SIGTERM
        // The state of {PID} should be “Terminating” until {PID} exits
        if (processes[i].pid == pid && processes[i].status == 2) {
            int exit_code = kill(pid, SIGTERM);
            proc_update_status(pid, 3, exit_code);
            break;
        }
    }
    free(arg);
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

static void command_exec(int len, char **cmd) {
    char **arg = NULL;
    char *program = cmd[0];
    arg_parse(cmd, &arg);

    // check if program exists and is executable : use access()
    if (access(program, R_OK) == 0 && access(program, X_OK) == 0) {
        // fork a subprocess and execute the program
        pid_t pid;
        if ((pid = fork()) == 0) {
            // CHILD PROCESS
            add_process(pid);
            execv(program, arg);
            free(arg);
            // Exit the child
            exit(0);
        } else {
            // PARENT PROCESS
            // register the process in process table
            add_process(pid);
            int status;
            int isBackground = 0;
            if (len > 1 && strcmp(cmd[len - 1], "&") == 0) {
                isBackground = 1;
            }
            if (isBackground) {
                //print Child [PID] in background
                printf("Child [%d] in background\n", pid);
                waitpid(pid, &status, WNOHANG);
            } else {
                // else wait for the child process to exit
                // Use waitpid() with WNOHANG when not blocking during wait and  waitpid() with WUNTRACED when parent needs to block due to wait
                waitpid(pid, &status, WUNTRACED);
            }
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                proc_update_status(pid, 1, exit_code);
            }
        }
    } else {
        fprintf(stderr, "%s not found\n", program);
    }
    free(arg);
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
    signal(SIGCHLD, proc_exit);
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



void arg_parse(char **cmd, char ***arguments) {
    char **arg_arr = (char**) calloc(1, sizeof(char *));
    int i = 0;
    while (cmd[i] != (char *) NULL) {
        if (strcmp(cmd[i], (char *) "&") == 0 || strcmp(cmd[i], (char *) "<") == 0 || strcmp(cmd[i], (char *) ">") == 0 || strcmp(cmd[i], (char *) "2>") == 0) {
            break;
        } else {
            arg_arr[i] = cmd[i];
            i++;
            // realloc to dynamically change memory allocation of argument array
            arg_arr = realloc(arg_arr, (i + 1) * sizeof(char *));
        }
    }
    // Add NULL for termination for each command
    arg_arr[i] = NULL;
    *arguments = arg_arr;
}


int ampersand_command(char **tokens) {
    int index = 0;
    while (tokens[index] != (char *) NULL) {
        if (strcmp(tokens[index], (char *) "&") == 0) {
            return 1;
        }
        index++;
    }

    return 0;
}


void proc_exit() {
    pid_t pid;
    int exit_code;
    pid = wait(&exit_code);
    if (pid > 0) {
        proc_update_status(pid, 1, exit_code);
    }
}
