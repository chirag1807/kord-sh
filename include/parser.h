#ifndef PARSER_H
#define PARSER_H

#define MAX_COMMANDS 64
#define MAX_ARGS 64

/**
 * Parse command string into array of commands (for pipes)
 * Returns array of command arrays (NULL terminated)
 * Example: "ls -la | grep txt" -> [["ls", "-la", NULL], ["grep", "txt", NULL], NULL]
 * Caller must free the returned array using free_commands()
 */
char ***parse_command(const char *command);

/**
 * Free the command array returned by parse_command
 */
void free_commands(char ***commands);

/**
 * Check if command should run in background (ends with &)
 * Returns 1 if command ends with &, 0 otherwise
 * Removes the & from the command arguments if found
 */
int is_background_command(char **args);

#endif // PARSER_H
