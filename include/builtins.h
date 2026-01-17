#ifndef BUILTINS_H
#define BUILTINS_H

/**
 * Check if a command is a built-in command
 * Returns 1 if built-in, 0 otherwise
 */
int is_builtin(const char *command);

/**
 * Check if a built-in command must run in parent process
 * Returns 1 if must run in parent, 0 if can run in child
 */
int must_run_in_parent(const char *command);

/**
 * Execute a built-in command
 * Returns 0 on success, non-zero on failure
 * Returns -1 for exit command (special case)
 */
int execute_builtin(char **args);

/**
 * Built-in command: cd - change directory
 */
int builtin_cd(char **args);

/**
 * Built-in command: pwd - print working directory
 */
int builtin_pwd(char **args);

/**
 * Built-in command: echo - print arguments
 */
int builtin_echo(char **args);

/**
 * Built-in command: exit - exit shell
 */
int builtin_exit(char **args);

/**
 * Built-in command: set - set shell variable
 * Usage: set VAR=value or set VAR value
 */
int builtin_set(char **args);

/**
 * Built-in command: export - export variable to environment
 * Usage: export VAR=value or export VAR (to export existing shell var)
 */
int builtin_export(char **args);

/**
 * Built-in command: unset - unset variable
 * Usage: unset VAR
 */
int builtin_unset(char **args);

/**
 * Built-in command: help - display help information
 * Usage: help [command]
 */
int builtin_help(char **args);

#endif // BUILTINS_H
