#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "../include/builtins.h"
#include "../include/variables.h"

// Built-in command types
typedef enum {
    BUILTIN_CD = 0,
    BUILTIN_PWD,
    BUILTIN_ECHO,
    BUILTIN_EXIT,
    BUILTIN_SET,
    BUILTIN_EXPORT,
    BUILTIN_UNSET,
    BUILTIN_HELP,
    BUILTIN_UNKNOWN
} BuiltinType;

// Built-in command table
typedef struct {
    const char *name;
    BuiltinType type;
    int (*func)(char **args);
    int must_run_in_parent;  // 1 if must run in parent, 0 if can run in child
} Builtin;

static Builtin builtins[] = {
    {"cd", BUILTIN_CD, builtin_cd, 1},
    {"pwd", BUILTIN_PWD, builtin_pwd, 0},
    {"echo", BUILTIN_ECHO, builtin_echo, 0},
    {"exit", BUILTIN_EXIT, builtin_exit, 1},
    {"set", BUILTIN_SET, builtin_set, 1},
    {"export", BUILTIN_EXPORT, builtin_export, 1},
    {"unset", BUILTIN_UNSET, builtin_unset, 1},
    {"help", BUILTIN_HELP, builtin_help, 0},
    {NULL, BUILTIN_UNKNOWN, NULL, 0}  // Sentinel
};

int is_builtin(const char *command) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(command, builtins[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

int must_run_in_parent(const char *command) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(command, builtins[i].name) == 0) {
            return builtins[i].must_run_in_parent;
        }
    }
    return 0;
}

int execute_builtin(char **args) {
    if (args[0] == NULL) {
        return 0;  // Empty command
    }

    // Find the builtin type
    BuiltinType type = BUILTIN_UNKNOWN;
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

int builtin_set(char **args) {
    // If no arguments, print all variables
    if (args[1] == NULL) {
        print_variables();
        return 0;
    }
    
    // Check if format is VAR=value
    char *equals = strchr(args[1], '=');
    if (equals != NULL) {
        // Split into name and value
        *equals = '\0';
        const char *name = args[1];
        const char *value = equals + 1;
        
        if (set_variable(name, value) != 0) {
            fprintf(stderr, "set: failed to set variable '%s'\n\r", name);
            return 1;
        }
    } else if (args[2] != NULL) {
        // Format is: set VAR value
        const char *name = args[1];
        const char *value = args[2];
        
        if (set_variable(name, value) != 0) {
            fprintf(stderr, "set: failed to set variable '%s'\n\r", name);
            return 1;
        }
    } else {
        fprintf(stderr, "set: usage: set VAR=value or set VAR value (or just 'set' to print all)\n\r");
        return 1;
    }
    
    return 0;
}

int builtin_export(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "export: usage: export VAR=value or export VAR\n");
        return 1;
    }
    
    // Check if format is VAR=value
    char *equals = strchr(args[1], '=');
    if (equals != NULL) {
        // Split into name and value
        *equals = '\0';
        const char *name = args[1];
        const char *value = equals + 1;
        
        if (export_variable(name, value) != 0) {
            fprintf(stderr, "export: failed to export variable '%s'\n", name);
            return 1;
        }
    } else {
        // Format is: export VAR (export existing shell variable)
        const char *name = args[1];
        
        if (export_variable(name, NULL) != 0) {
            fprintf(stderr, "export: failed to export variable '%s'\n", name);
            return 1;
        }
    }
    
    return 0;
}

int builtin_unset(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "unset: usage: unset VAR\n\r");
        return 1;
    }
    
    const char *name = args[1];
    
    if (unset_variable(name) != 0) {
        // Not an error if variable doesn't exist
        // fprintf(stderr, "unset: variable '%s' not found\n\r", name);
    }
    
    return 0;
}

int builtin_help(char **args) {
    if (args[1] != NULL) {
        // Help for specific command
        const char *cmd = args[1];

        int cmd_index = -1;
        for (int i = 0; builtins[i].name != NULL; i++) {
            if (strcmp(cmd, builtins[i].name) == 0) {
                cmd_index = i;
                break;
            }
        }
        
        switch (cmd_index) {
            case 0: // cd
                printf("cd: cd [directory]\n\r");
                printf("  Change the current directory.\n\r");
                printf("  If no directory is specified, changes to HOME directory.\n\r");
                break;
            case 1: // pwd
                printf("pwd: pwd\n\r");
                printf("  Print the current working directory.\n\r");
                break;
            case 2: // echo
                printf("echo: echo [args...]\n\r");
                printf("  Print arguments to standard output.\n\r");
                printf("  Variables can be expanded using $VAR syntax.\n\r");
                break;
            case 3: // exit
                printf("exit: exit\n\r");
                printf("  Exit the shell.\n\r");
                break;
            case 4: // set
                printf("set: set [VAR=value | VAR value]\n\r");
                printf("  Set a shell variable (not exported to environment).\n\r");
                printf("  Without arguments, displays all shell variables.\n\r");
                printf("  Alternative: VAR=value (direct assignment)\n\r");
                break;
            case 5: // export
                printf("export: export VAR[=value]\n\r");
                printf("  Export a variable to the environment.\n\r");
                printf("  - export VAR=value: Create and export variable\n\r");
                printf("  - export VAR: Export existing shell variable\n\r");
                break;
            case 6: // unset
                printf("unset: unset VAR\n\r");
                printf("  Remove a variable from both shell and environment.\n\r");
                break;
            case 7: // help
                printf("help: help [command]\n\r");
                printf("  Display help information about builtin commands.\n\r");
                printf("  Without arguments, lists all available commands.\n\r");
                break;
            default:
                printf("help: no help topics match '%s'\n\r", cmd);
                return 1;
        }
    } else {
        // General help
        printf("\n\r");
        printf("kord-sh - A simple Unix shell\n\r");
        printf("============================\n\r\n\r");
        printf("Built-in commands:\n\r");
        printf("  cd [dir]          - Change directory\n\r");
        printf("  pwd               - Print working directory\n\r");
        printf("  echo [args...]    - Print arguments\n\r");
        printf("  exit              - Exit the shell\n\r");
        printf("  set [VAR=value]   - Set shell variable or display all\n\r");
        printf("  export VAR[=val]  - Export variable to environment\n\r");
        printf("  unset VAR         - Remove variable\n\r");
        printf("  help [command]    - Display this help\n\r");
        printf("\n\r");
        printf("Variable Assignment:\n\r");
        printf("  VAR=value         - Set shell variable directly\n\r");
        printf("  $VAR              - Expand variable value\n\r");
        printf("\n\r");
        printf("Features:\n\r");
        printf("  - Pipes: command1 | command2\n\r");
        printf("  - I/O Redirection: < input.txt > output.txt >> append.txt\n\r");
        printf("  - Variable expansion in all commands\n\r");
        printf("\n\r");
        printf("For more info on a specific command, type: help <command>\n\r");
        printf("\n\r");
    }
    
    return 0;
}
