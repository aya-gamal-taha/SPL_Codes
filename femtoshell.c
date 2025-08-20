#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int femtoshell_main(int argc, char *argv[]) {
    char input[10240];        // buffer to store user input
    int last_status = 0;      // variable to keep track of the last command's exit status

    while (1) {
        // Print the shell prompt
        printf("MyFemto> ");
        fflush(stdout);

        // Read user input (fgets ensures we don't overflow the buffer)
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // End of File (Ctrl+D) → exit loop
        }

        // Remove the newline character at the end of input
        input[strcspn(input, "\n")] = 0;

        // If the user just pressed Enter (empty line), show prompt again
        if (strlen(input) == 0) {
            continue;
        }

        // Handle "exit" command
        if (strcmp(input, "exit") == 0) {
            printf("Good Bye\n");
            return last_status; // Return the last command's status when exiting
        }

        // Handle "echo" command
        if (strncmp(input, "echo", 4) == 0) {
            if (strlen(input) > 5) {
                // Print everything after "echo " (with space)
                printf("%s\n", input + 5);
            } else {
                // If no arguments after echo, just print a blank line
                printf("\n");
            }
            last_status = 0; // echo always succeeds
            continue;
        }

        // If the command is not recognized
        printf("Invalid command\n");
        last_status = 1; // Invalid command = failure
        continue;
    }

    // If the shell exits because of EOF, return the last command status
    return last_status;
}
