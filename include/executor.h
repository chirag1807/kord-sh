#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "config.h"

/**
 * Execute commands (handles multiple commands for pipes)
 * Returns 0 on success, non-zero on failure
 * Returns -1 if shell should exit
 */
int execute_command(char ***commands);

/**
 * Execute a single command (either built-in or external)
 * Returns 0 on success, non-zero on failure
 * Returns -1 if shell should exit
 */
int execute_single_command(char **command, int fd_read, int fd_write);

/**
 * Execute an external command using fork/exec
 * Returns 0 on success, non-zero on failure
 */
int execute_external(char **command, int fd_read, int fd_write);

/**
 * Apply I/O redirection based on command arguments
 * Scans for <, >, >> operators and redirects stdin/stdout accordingly
 * Modifies the command array in-place to remove redirection operators
 * 
 * Example: ["grep", "pattern", ">", "out.txt", NULL] 
 *       -> Opens out.txt, redirects stdout, modifies to ["grep", "pattern", NULL]
 */
void apply_io_redirection(char **command);

#endif // EXECUTOR_H
