#ifndef PARSER_H
#define PARSER_H

#define MAX_COMMANDS 64
#define MAX_ARGS 64

// Command separator types
typedef enum {
    SEP_NONE = 0,    // No separator (last command)
    SEP_PIPE = 1,    // | (pipe output to next command)
    SEP_AND = 2      // && (execute next only if current succeeds)
} separator_type_t;

// Represents a single command with its separator
typedef struct {
    char **args;              // Command arguments (NULL terminated)
    separator_type_t sep;     // Separator after this command
} command_t;

/**
 * Parse command string into array of commands (for pipes and &&)
 * Returns array of command structures (NULL terminated args for last command)
 * Example: "ls -la | grep txt" -> [["ls", "-la", NULL], ["grep", "txt", NULL], NULL]
 * Caller must free the returned array using free_commands()
 */
char ***parse_command(const char *command);

/**
 * Parse command string with separator information
 * Returns array of command_t structures with separator types
 * Caller must free using free_command_list()
 */
command_t *parse_command_with_separators(const char *command, int *count);

/**
 * Free the command array returned by parse_command
 */
void free_commands(char ***commands);

/**
 * Free the command list returned by parse_command_with_separators
 */
void free_command_list(command_t *commands, int count);

#endif // PARSER_H
