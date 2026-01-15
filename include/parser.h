#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 64

/**
 * Parse command string into tokens (arguments)
 * Returns array of strings (NULL terminated)
 * Caller must free the returned array
 */
char **parse_command(const char *command);

/**
 * Free the argument array returned by parse_command
 */
void free_args(char **args);

#endif // PARSER_H
