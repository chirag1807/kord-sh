#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/parser.h"

char **parse_command(const char *command) {
    char **args = malloc(MAX_ARGS * sizeof(char *));
    if (!args) {
        perror("malloc");
        return NULL;
    }
    
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        free(args);
        perror("strdup");
        return NULL;
    }
    
    int arg_count = 0;
    char *saveptr;
    char *token = strtok_r(cmd_copy, " \t\n\r", &saveptr);
    
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count] = strdup(token);
        if (!args[arg_count]) {
            // Cleanup on error
            for (int i = 0; i < arg_count; i++) {
                free(args[i]);
            }
            free(args);
            free(cmd_copy);
            perror("strdup");
            return NULL;
        }
        arg_count++;
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }
    
    args[arg_count] = NULL;  // NULL terminate the array
    free(cmd_copy);
    
    return args;
}

void free_args(char **args) {
    if (args == NULL) {
        return;
    }
    
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

