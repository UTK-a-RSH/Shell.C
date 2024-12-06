#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h> // For exit()
#include <limits.h> // For PATH_MAX

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define ARG_BUFFER_SIZE 1024

typedef enum {
    STATE_DEFAULT,
    STATE_IN_SINGLE_QUOTE,
    STATE_IN_DOUBLE_QUOTE,
    STATE_ESCAPE_DEFAULT,
    STATE_ESCAPE_DOUBLE_QUOTE
} parser_state_t;

// Function to parse input with single and double quotes and handle backslashes
int parse_arguments(char *input, char *args[]) {
    int i = 0;                     // Argument index
    int len = strlen(input);
    parser_state_t state = STATE_DEFAULT;
    char arg_buffer[ARG_BUFFER_SIZE];
    int arg_len = 0;

    for (int pos = 0; pos < len; pos++) {
        char c = input[pos];

        switch (state) {
            case STATE_DEFAULT:
                if (c == ' ') {
                    if (arg_len > 0) {
                        arg_buffer[arg_len] = '\0';
                        args[i++] = strdup(arg_buffer);
                        if (args[i-1] == NULL) {
                            perror("strdup");
                            return -1;
                        }
                        arg_len = 0;
                        if (i >= MAX_ARGS - 1) {
                            fprintf(stderr, "Error: too many arguments\n");
                            return -1;
                        }
                    }
                    // Else, skip multiple spaces
                }
                else if (c == '\'') {
                    state = STATE_IN_SINGLE_QUOTE;
                }
                else if (c == '\"') {
                    state = STATE_IN_DOUBLE_QUOTE;
                }
                else if (c == '\\') {
                    state = STATE_ESCAPE_DEFAULT;
                }
                else {
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                }
                break;

            case STATE_IN_SINGLE_QUOTE:
                if (c == '\'') {
                    state = STATE_DEFAULT;
                }
                else {
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                }
                break;

            case STATE_IN_DOUBLE_QUOTE:
                if (c == '\"') {
                    state = STATE_DEFAULT;
                }
                else if (c == '\\') {
                    state = STATE_ESCAPE_DOUBLE_QUOTE;
                }
                else {
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                }
                break;

            case STATE_ESCAPE_DEFAULT:
                if (c == '\0') {
                    fprintf(stderr, "Error: trailing backslash\n");
                    return -1;
                }
                else {
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                    state = STATE_DEFAULT;
                }
                break;

            case STATE_ESCAPE_DOUBLE_QUOTE:
                if (c == '\"' || c == '\\' || c == '$' || c == '`') {
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                }
                else {
                    // Backslash remains
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = '\\';
                    if (arg_len < ARG_BUFFER_SIZE - 1)
                        arg_buffer[arg_len++] = c;
                    else {
                        fprintf(stderr, "Error: argument too long\n");
                        return -1;
                    }
                }
                state = STATE_IN_DOUBLE_QUOTE;
                break;
        }
    }

    // End of input processing
    if (state == STATE_IN_SINGLE_QUOTE) {
        fprintf(stderr, "Error: unmatched single quote\n");
        return -1;
    }
    if (state == STATE_IN_DOUBLE_QUOTE) {
        fprintf(stderr, "Error: unmatched double quote\n");
        return -1;
    }
    if (state == STATE_ESCAPE_DEFAULT || state == STATE_ESCAPE_DOUBLE_QUOTE) {
        fprintf(stderr, "Error: trailing backslash\n");
        return -1;
    }

    if (arg_len > 0) {
        arg_buffer[arg_len] = '\0';
        args[i++] = strdup(arg_buffer);
        if (args[i-1] == NULL) {
            perror("strdup");
            return -1;
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

            // Parse input into arguments handling single and double quotes and backslashes
            if (parse_arguments(input, args) != 0) {
                continue; // Skip to next loop iteration on parse error
            }

            // Handle 'exit' command
            if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
                int exit_status = 0;
                if (args[1] != NULL) {
                    exit_status = atoi(args[1]);
                }
                // Free allocated memory before exiting
                for (int j = 0; args[j] != NULL; j++) {
                    free(args[j]);
                }
                exit(exit_status);
            }

            // Handle 'cd' command
            if (args[0] != NULL && strcmp(args[0], "cd") == 0) {
                if (args[1] != NULL) {
                    // Handle '~' character
                    if (strcmp(args[1], "~") == 0) {
                        char *home = getenv("HOME");
                        if (home != NULL) {
                            if (chdir(home) != 0) {
                                perror("cd");
                            }
                        }
                        else {
                            fprintf(stderr, "cd: HOME not set\n");
                        }
                    }
                    else {
                        if (chdir(args[1]) != 0) {
                            perror("cd");
                        }
                    }
                }
                else {
                    // If no argument is provided, change to HOME directory
                    char *home = getenv("HOME");
                    if (home != NULL) {
                        if (chdir(home) != 0) {
                            perror("cd");
                        }
                    }
                    else {
                        fprintf(stderr, "cd: HOME not set\n");
                    }
                }
                // Free allocated memory after built-in command
                for (int j = 0; args[j] != NULL; j++) {
                    free(args[j]);
                }
                continue; // Skip external command execution after handling built-in
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
                // Free allocated memory after built-in command
                for (int j = 0; args[j] != NULL; j++) {
                    free(args[j]);
                }
                continue; // Skip external command execution after handling built-in
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
                    }
                    else {
                        char *path_env = getenv("PATH");
                        if (path_env == NULL) {
                            printf("PATH not set\n");
                            // Free allocated memory
                            for (int j = 0; args[j] != NULL; j++) {
                                free(args[j]);
                            }
                            continue;
                        }

                        char path_copy[PATH_MAX];
                        strncpy(path_copy, path_env, PATH_MAX - 1);
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
                }
                else {
                    printf("type: missing argument\n"); // Handle missing argument
                }
                // Free allocated memory after built-in command
                for (int j = 0; args[j] != NULL; j++) {
                    free(args[j]);
                }
                continue; // Skip external command execution after handling built-in
            }

            // Handle 'pwd' command
            if (args[0] != NULL && strcmp(args[0], "pwd") == 0) {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("%s\n", cwd);
                }
                else {
                    perror("pwd");
                }
                // Free allocated memory after built-in command
                for (int j = 0; args[j] != NULL; j++) {
                    free(args[j]);
                }
                continue; // Skip external command execution after handling built-in
            }

            // External command execution
            pid_t pid = fork(); // Fork a child process
            if (pid == 0) {
                // Child process attempts to execute the command
                execvp(args[0], args);
                // If execvp returns, there was an error
                printf("%s: command not found\n", args[0]);
                _exit(1); // Exit child process
            }
            else if (pid > 0) {
                // Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            }
            else {
                // Handle fork failure
                perror("fork");
            }

            // Free allocated argument strings
            for (int j = 0; args[j] != NULL; j++) {
                free(args[j]);
            }
        }
    }

    return 0; // Ensures the shell runs indefinitely until 'exit' is called
}