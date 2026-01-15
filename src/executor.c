#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/executor.h"
#include "../include/builtins.h"

int execute_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        return 0;  // Empty command
    }
    
    // Check if it's a built-in command
    if (is_builtin(args[0])) {
        return execute_builtin(args);
    }
    
    // Otherwise, execute as external command
    return execute_external(args);
}

int execute_external(char **args) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        // Child process
        printf("child, ls");
        if (execvp(args[0], args) == -1) {
            perror("kord-sh");
        }
        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    
    return 0;
}

