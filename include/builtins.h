#ifndef BUILTINS_H
#define BUILTINS_H

/**
 * Check if a command is a built-in command
 * Returns 1 if built-in, 0 otherwise
 */
int is_builtin(const char *command);

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

#endif // BUILTINS_H
