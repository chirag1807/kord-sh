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

#endif // PARSER_H
