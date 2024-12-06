#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h> // Added for exit()

#define MAX_INPUT 100
#define MAX_ARGS 10

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    
    while (1) { // 1. Create an infinite loop for the REPL
        printf("$ "); // 2. Display the prompt
        fflush(stdout); // Ensure the prompt is shown immediately

        if (fgets(input, sizeof(input), stdin) != NULL) { // 3. Read user input
            // 4. Remove trailing newline
            input[strcspn(input, "\n")] = '\0';

            // 5. Tokenize input into arguments
            int i = 0;
            char *token = strtok(input, " ");
            while (token != NULL && i < MAX_ARGS - 1) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL; // Null-terminate the arguments array

            // 6. Check for 'exit' command
            if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
                int exit_status = 0; // Default exit status
                if (args[1] != NULL) {
                    exit_status = atoi(args[1]); // Convert argument to integer
                }
                exit(exit_status); // Terminate the shell with the specified status
            }

            // 7. Check for 'echo' command
            if (args[0] != NULL && strcmp(args[0], "echo") == 0) {
                // 8. Handle 'echo' by printing the arguments
                for (int j = 1; args[j] != NULL; j++) {
                    printf("%s", args[j]);
                    if (args[j + 1] != NULL) {
                        printf(" ");
                    }
                }
                printf("\n"); // Newline after echo
                continue; // Go back to the prompt
            }

            // 9. Check for 'type' command
            if (args[0] != NULL && strcmp(args[0], "type") == 0) {
                if (args[1] != NULL) { // Ensure a command name is provided
                    // 10. List of shell builtins
                    const char *builtins[] = {"echo", "exit", "type"};
                    int is_builtin = 0;
                    // 11. Iterate through builtins to check
                    for (int k = 0; k < 3; k++) {
                        if (strcmp(args[1], builtins[k]) == 0) {
                            is_builtin = 1;
                            break;
                        }
                    }
                    if (is_builtin) {
                        printf("%s is a shell builtin\n", args[1]);
                    } else {
                        printf("%s: not found\n", args[1]);
                    }
                } else {
                    printf("type: missing argument\n"); // Handle missing argument
                }
                continue; // Go back to the prompt
            }

            pid_t pid = fork(); // 12. Fork a child process
            if (pid == 0) {
                // 13. Child process attempts to execute the command
                execvp(args[0], args);
                // 14. If execvp returns, there was an error
                printf("%s: command not found\n", args[0]);
                _exit(1); // Exit child process
            } else if (pid > 0) {
                // 15. Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            } else {
                // 16. Handle fork failure
                perror("fork");
            }
        }
    }
    return 0;
}