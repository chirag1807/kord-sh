#ifndef EXECUTOR_H
#define EXECUTOR_H

/**
 * Execute a command (either built-in or external)
 * Returns 0 on success, non-zero on failure
 * Returns -1 if shell should exit
 */
int execute_command(char **args);

/**
 * Execute an external command using fork/exec
 * Returns 0 on success, non-zero on failure
 */
int execute_external(char **args);

#endif // EXECUTOR_H
