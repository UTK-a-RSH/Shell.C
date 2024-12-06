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

// Function to parse input with single and double quotes
int parse_arguments(char *input, char *args[]) {
    int i = 0;
    char *ptr = input;
    while (*ptr != '\0' && i < MAX_ARGS - 1) {
        // Skip leading spaces
        while (*ptr == ' ') ptr++;
        if (*ptr == '\0') break;

        if (*ptr == '\'') {
            ptr++; // Skip the opening single quote
            args[i++] = ptr;
            // Find the closing single quote
            while (*ptr != '\'' && *ptr != '\0') ptr++;
            if (*ptr == '\0') {
                fprintf(stderr, "Error: unmatched single quote\n");
                return -1;
            }
            *ptr = '\0'; // Terminate the argument
            ptr++; // Move past the closing single quote
        }
        else if (*ptr == '\"') {
            ptr++; // Skip the opening double quote
            args[i++] = ptr;
            char *arg_start = ptr;
            while (*ptr != '\"' && *ptr != '\0') {
                if (*ptr == '\\') {
                    // Handle escape characters
                    ptr++;
                    if (*ptr == '\"' || *ptr == '\\' || *ptr == '$' || *ptr == '\n') {
                        // Move the escaped character
                        memmove(ptr - 1, ptr, strlen(ptr) + 1);
                    }
                    else {
                        // Keep the backslash for other characters
                        ptr--;
                    }
                }
                ptr++;
            }
            if (*ptr == '\0') {
                fprintf(stderr, "Error: unmatched double quote\n");
                return -1;
            }
            *ptr = '\0'; // Terminate the argument
            ptr++; // Move past the closing double quote
        }
        else {
            args[i++] = ptr;
            // Find the next space or quote
            while (*ptr != ' ' && *ptr != '\'' && *ptr != '\"' && *ptr != '\0') ptr++;
            if (*ptr != '\0') {
                *ptr = '\0';
                ptr++;
            }
        }
    }
    args[i] = NULL; // Null-terminate the arguments array
    return 0;
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    
    while (1) { // Infinite loop for the REPL
        printf("$ "); // Display the prompt
        fflush(stdout); // Ensure the prompt is shown immediately

        if (fgets(input, sizeof(input), stdin) != NULL) { // Read user input
            input[strcspn(input, "\n")] = '\0'; // Remove trailing newline

            // Parse input into arguments handling single and double quotes
            if (parse_arguments(input, args) != 0) {
                continue; // Skip to next loop iteration on parse error
            }

            // Handle 'exit' command
            if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
                int exit_status = 0;
                if (args[1] != NULL) {
                    exit_status = atoi(args[1]);
                }
                exit(exit_status);
            }

            // Handle 'echo' command
            if (args[0] != NULL && strcmp(args[0], "echo") == 0) {
                for (int j = 1; args[j] != NULL; j++) {
                    printf("%s", args[j]);
                    if (args[j + 1] != NULL) {
                        printf(" ");
                    }
                }
                printf("\n");
                continue;
            }

            // Handle 'type' command
            if (args[0] != NULL && strcmp(args[0], "type") == 0) {
                if (args[1] != NULL) {
                    const char *builtins[] = {"pwd", "exit", "type", "echo", "cd"};
                    int is_builtin = 0;
                    int num_builtins = sizeof(builtins) / sizeof(builtins[0]);

                    for (int k = 0; k < num_builtins; k++) {
                        if (strcmp(args[1], builtins[k]) == 0) {
                            is_builtin = 1;
                            break;
                        }
                    }
                    if (is_builtin) {
                        printf("%s is a shell builtin\n", args[1]);
                    } else {
                        char *path_env = getenv("PATH");
                        if (path_env == NULL) {
                            printf("PATH not set\n");
                            continue;
                        }

                        char path_copy[PATH_MAX];
                        strncpy(path_copy, path_env, PATH_MAX);
                        path_copy[PATH_MAX - 1] = '\0';

                        char *dir = strtok(path_copy, ":");
                        int found = 0;
                        while (dir != NULL) {
                            char full_path[PATH_MAX];
                            snprintf(full_path, sizeof(full_path), "%s/%s", dir, args[1]);
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
                    printf("type: missing argument\n");
                }
                continue;
            }

            // Handle 'pwd' command
            if (args[0] != NULL && strcmp(args[0], "pwd") == 0) {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("%s\n", cwd);
                } else {
                    perror("pwd");
                }
                continue;
            }

            // Handle 'cd' command
            if (args[0] != NULL && strcmp(args[0], "cd") == 0) {
                if (args[1] != NULL) {
                    // Handle '~' character
                    if (strcmp(args[1], "~") == 0) {
                        char *home = getenv("HOME");
                        if (home != NULL) {
                            if (chdir(home) != 0) {
                                printf("cd: %s: No such file or directory\n", home);
                            }
                        } else {
                            printf("cd: HOME not set\n");
                        }
                    } else {
                        if (chdir(args[1]) != 0) {
                            printf("cd: %s: No such file or directory\n", args[1]);
                        }
                    }
                } else {
                    printf("cd: missing argument\n");
                }
                continue; // Prevent external execution
            }

            // External command execution
            pid_t pid = fork(); // Fork a child process
            if (pid == 0) {
                // Child process attempts to execute the command
                execvp(args[0], args);
                // If execvp returns, there was an error
                printf("%s: command not found\n", args[0]);
                _exit(1); // Exit child process
            } else if (pid > 0) {
                // Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            } else {
                // Handle fork failure
                perror("fork");
            }
        }
    }
    return 0;
}