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

            pid_t pid = fork(); // 7. Fork a child process
            if (pid == 0) {
                // 8. Child process attempts to execute the command
                execvp(args[0], args);
                // 9. If execvp returns, there was an error
                printf("%s: command not found\n", args[0]);
                _exit(1); // Exit child process
            } else if (pid > 0) {
                // 10. Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            } else {
                // 11. Handle fork failure
                perror("fork");
            }
        }
    }
    return 0;
}