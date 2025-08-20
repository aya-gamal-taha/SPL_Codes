#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

int microshell_main(int argc, char *argv[]) {
    char input[1024];
    int last_status = 0;

    while (1) {
        // Display prompt
        printf("MicroShell> ");
        fflush(stdout);

        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // EOF or error
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }

        // Check for exit command
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Parse command with redirection support
        char *args[100];
        char *infile = NULL;
        char *outfile = NULL;
        char *errfile = NULL;
        int arg_count = 0;
        int parsing_error = 0;

        // Make a copy of input for parsing
        char input_copy[1024];
        strcpy(input_copy, input);
        
        char *token = strtok(input_copy, " ");
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                // Input redirection
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after <\n");
                    parsing_error = 1;
                    last_status = 1;
                    break;
                }
                infile = token;
            } else if (strcmp(token, ">") == 0) {
                // Output redirection
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after >\n");
                    parsing_error = 1;
                    last_status = 1;
                    break;
                }
                outfile = token;
            } else if (strcmp(token, "2>") == 0) {
                // Error redirection
                token = strtok(NULL, " ");
                if (token == NULL) {
                    fprintf(stderr, "syntax error: expected filename after 2>\n");
                    parsing_error = 1;
                    last_status = 1;
                    break;
                }
                errfile = token;
            } else {
                // Regular argument
                args[arg_count++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        // Skip if parsing error or no command
        if (parsing_error) {
            continue;
        }
        if (arg_count == 0) {
            continue;
        }

        // Check if this is a variable assignment (like "filename=error_var.txt")
        // Variable assignments should be ignored/skipped in this shell
        if (strchr(args[0], '=') != NULL) {
            // This looks like a variable assignment, skip execution
            continue;
        }

        // Fork and execute command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            
            // Handle input redirection
            if (infile != NULL) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr, "cannot access %s: %s\n", infile, strerror(errno));
                    exit(1);
                }
                if (dup2(fd, STDIN_FILENO) < 0) {
                    fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                    close(fd);
                    exit(1);
                }
                close(fd);
            }

            // Handle output redirection
            if (outfile != NULL) {
                int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    fprintf(stderr, "%s: %s\n", outfile, strerror(errno));
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                    close(fd);
                    exit(1);
                }
                close(fd);
            }

            // Handle error redirection
            if (errfile != NULL) {
                int fd = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    fprintf(stderr, "%s: %s\n", errfile, strerror(errno));
                    exit(1);
                }
                if (dup2(fd, STDERR_FILENO) < 0) {
                    fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                    close(fd);
                    exit(1);
                }
                close(fd);
            }

            // Execute the command
            execvp(args[0], args);
            
            // If execvp returns, there was an error
            // For commands like "filename=error_var.txt", don't print error
            if (strchr(args[0], '=') == NULL) {
                fprintf(stderr, "%s: command not found\n", args[0]);
            }
            exit(127);
            
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status); // Signal termination
            } else {
                last_status = 1; // Command terminated abnormally
            }
        } else {
            // Fork failed
            fprintf(stderr, "fork failed: %s\n", strerror(errno));
            last_status = 1;
        }
    }

    // Return the status of the last executed command
    // If no command was executed or shell exited normally, return 0
    return last_status;
}
