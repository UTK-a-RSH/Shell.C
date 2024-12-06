#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 100
#define MAX_ARGS 10

int main() {
    printf("$ ");
    fflush(stdout);

    char input[MAX_INPUT];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';

        // Tokenize input
        char *args[MAX_ARGS];
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execvp(args[0], args);
            // If execvp returns, there was an error
            printf("%s: not found\n", args[0]);
            _exit(1);
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
        } else {
            // Fork failed
            perror("fork");
        }
    }
    return 0;
}