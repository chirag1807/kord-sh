#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/builtins.h"

// Built-in command table
typedef struct {
    const char *name;
    int (*func)(char **args);
} Builtin;

static Builtin builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"echo", builtin_echo},
    {"exit", builtin_exit},
    {NULL, NULL}  // Sentinel
};

int is_builtin(const char *command) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(command, builtins[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

int execute_builtin(char **args) {
    if (args[0] == NULL) {
        return 0;  // Empty command
    }

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            return builtins[i].func(args);
        }

    }
    
    return 1;  // Not found (shouldn't happen if is_builtin was checked)
}

int builtin_cd(char **args) {
    const char *path;
    
    if (args[1] == NULL) {
        // No argument, go to home directory
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else {
        path = args[1];
    }
    
    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }
    
    return 0;
}

int builtin_pwd(char **args) {
    (void)args;  // Unused parameter
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    }

    perror("pwd");
    return 1;
}

int builtin_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        if (i > 1) {
            printf(" ");
        }
        printf("%s", args[i]);
    }
    printf("\n");
    return 0;
}

int builtin_exit(char **args) {
    (void)args;  // Unused parameter
    return -1;  // Special return value to signal exit
}
