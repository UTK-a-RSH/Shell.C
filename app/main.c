#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h> // Added for exit()
#include <limits.h> // Added for PATH_MAX

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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
                        // 12. Retrieve PATH environment variable
                        char *path_env = getenv("PATH");
                        if (path_env == NULL) {
                            printf("PATH not set\n");
                            continue;
                        }

                        // 13. Duplicate PATH to avoid modifying the original
                        char path_copy[PATH_MAX];
                        strncpy(path_copy, path_env, PATH_MAX);
                        path_copy[PATH_MAX - 1] = '\0';

                        // 14. Split PATH into directories
                        char *dir = strtok(path_copy, ":");
                        int found = 0;
                        while (dir != NULL) {
                            char full_path[PATH_MAX];
                            snprintf(full_path, sizeof(full_path), "%s/%s", dir, args[1]);
                            // 15. Check if the file exists and is executable
                            if (access(full_path, X_OK) == 0) {
                                printf("%s is %s\n", args[1], full_path);
                                found = 1;
                                break;
                            }
                            dir = strtok(NULL, ":");
                        }
                        if (!found) {
                            printf("%s: not found\n", args[1]);
                        }
                    }
                } else {
                    printf("type: missing argument\n"); // Handle missing argument
                }
                continue; // Go back to the prompt
            }

            // 16. Check for 'pwd' command
            if (args[0] != NULL && strcmp(args[0], "pwd") == 0) {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("%s\n", cwd);
                } else {
                    perror("pwd");
                }
                continue; // Go back to the prompt
            }

            pid_t pid = fork(); // 17. Fork a child process
            if (pid == 0) {
                // 18. Child process attempts to execute the command
                execvp(args[0], args);
                // 19. If execvp returns, there was an error
                printf("%s: command not found\n", args[0]);
                _exit(1); // Exit child process
            } else if (pid > 0) {
                // 20. Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            } else {
                // 21. Handle fork failure
                perror("fork");
            }
        }
    }
    return 0;
}