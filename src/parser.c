#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/parser.h"
#include "../include/variables.h"

/**
 * Helper function to trim leading and trailing whitespace
 */
static char *trim_whitespace(char *str) {
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    return str;
}

/**
 * Expand variables in a string (e.g., "$foo" -> "value")
 * Returns a newly allocated string that must be freed by caller
 */
static char *expand_variables(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    
    // Allocate buffer for expanded string (worst case: same size if no variables)
    size_t max_len = strlen(str) * 4 + 1024;  // Allow room for expansion
    char *result = malloc(max_len);
    if (!result) {
        perror("malloc");
        return NULL;
    }
    
    const char *src = str;
    char *dst = result;
    size_t remaining = max_len - 1;
    
    while (*src && remaining > 0) {
        if (*src == '$') {
            src++;  // Skip the '$'
            
            // Extract variable name
            const char *var_start = src;
            while (*src && (isalnum((unsigned char)*src) || *src == '_')) {
                src++;
            }
            
            if (src > var_start) {
                // We have a variable name
                size_t var_len = src - var_start;
                char var_name[256];
                
                if (var_len < sizeof(var_name)) {
                    strncpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    // Get variable value
                    const char *value = get_variable(var_name);
                    if (value) {
                        size_t value_len = strlen(value);
                        if (value_len < remaining) {
                            strcpy(dst, value);
                            dst += value_len;
                            remaining -= value_len;
                        }
                    }
                    // If variable not found, replace with empty string
                }
            } else {
                // Just a '$' without a variable name, keep it
                if (remaining > 0) {
                    *dst++ = '$';
                    remaining--;
                }
            }
        } else {
            // Regular character
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    return result;
}

/**
 * Parse a single command string into arguments
 */
static char **parse_single_command(const char *cmd_str) {
    char **args = malloc(MAX_ARGS * sizeof(char *));
    if (!args) {
        perror("malloc");
        return NULL;
    }
    
    char *cmd_copy = strdup(cmd_str);
    if (!cmd_copy) {
        free(args);
        perror("strdup");
        return NULL;
    }
    
    int arg_count = 0;
    char *ptr = cmd_copy;
    
    while (*ptr && arg_count < MAX_ARGS - 1) {
        // Skip leading whitespace
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        
        if (!*ptr) break;
        
        char *start = ptr;
        char quote = 0;
        
        // Check if this token starts with a quote
        if (*ptr == '"' || *ptr == '\'') {
            quote = *ptr;
            ptr++;
            start = ptr;
            
            // Find matching closing quote
            while (*ptr && *ptr != quote) ptr++;
            
            if (*ptr == quote) {
                *ptr = '\0';  // Null-terminate at closing quote
                
                // Expand variables in the argument
                char *expanded = expand_variables(start);
                if (expanded) {
                    args[arg_count] = expanded;
                } else {
                    args[arg_count] = strdup(start);
                }
                
                ptr++;  // Move past the closing quote
            } else {
                // No closing quote found, treat as regular token
                char *expanded = expand_variables(start);
                if (expanded) {
                    args[arg_count] = expanded;
                } else {
                    args[arg_count] = strdup(start);
                }
            }
        } else {
            // Regular token (no quotes)
            while (*ptr && !isspace((unsigned char)*ptr)) ptr++;
            
            char temp = *ptr;
            *ptr = '\0';
            
            // Expand variables in the argument
            char *expanded = expand_variables(start);
            if (expanded) {
                args[arg_count] = expanded;
            } else {
                args[arg_count] = strdup(start);
            }
            
            if (temp) *ptr = temp;
        }
        
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
        if (*ptr) ptr++;
    }
    
    args[arg_count] = NULL;  // NULL terminate the array
    free(cmd_copy);
    
    return args;
}

/**
 * Parse command string with separator information
 * Handles both | (pipes) and && (conditional execution)
 */
command_t *parse_command_with_separators(const char *command, int *count) {
    *count = 0;
    
    command_t *commands = malloc(MAX_COMMANDS * sizeof(command_t));
    if (!commands) {
        perror("malloc");
        return NULL;
    }
    
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        free(commands);
        perror("strdup");
        return NULL;
    }
    
    char *ptr = cmd_copy;
    int cmd_count = 0;
    
    while (*ptr && cmd_count < MAX_COMMANDS) {
        // Skip leading whitespace
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        if (!*ptr) break;
        
        // Find the next separator (| or &&)
        char *cmd_start = ptr;
        char *cmd_end = ptr;
        separator_type_t sep = SEP_NONE;
        
        // Scan for separator
        while (*ptr) {
            if (*ptr == '|') {
                cmd_end = ptr;
                sep = SEP_PIPE;
                *ptr = '\0';  // Null-terminate command
                ptr++;
                break;
            } else if (*ptr == '&' && *(ptr + 1) == '&') {
                cmd_end = ptr;
                sep = SEP_AND;
                *ptr = '\0';  // Null-terminate command
                ptr += 2;     // Skip both &
                break;
            }
            ptr++;
        }
        
        // If no separator found, command ends at string end
        if (sep == SEP_NONE) {
            cmd_end = ptr;
        }
        
        // Trim whitespace from command segment
        cmd_start = trim_whitespace(cmd_start);
        
        // Skip empty segments
        if (strlen(cmd_start) == 0) {
            continue;
        }
        
        // Parse this command segment into arguments
        char **args = parse_single_command(cmd_start);
        if (!args) {
            // Cleanup on error
            for (int i = 0; i < cmd_count; i++) {
                for (int j = 0; commands[i].args[j] != NULL; j++) {
                    free(commands[i].args[j]);
                }
                free(commands[i].args);
            }
            free(commands);
            free(cmd_copy);
            return NULL;
        }
        
        commands[cmd_count].args = args;
        commands[cmd_count].sep = sep;
        cmd_count++;
    }
    
    free(cmd_copy);
    *count = cmd_count;
    
    return commands;
}

/**
 * Free command list returned by parse_command_with_separators
 */
void free_command_list(command_t *commands, int count) {
    if (commands == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (commands[i].args != NULL) {
            for (int j = 0; commands[i].args[j] != NULL; j++) {
                free(commands[i].args[j]);
            }
            free(commands[i].args);
        }
    }
    free(commands);
}

char ***parse_command(const char *command) {
    // Allocate array to hold multiple commands
    char ***commands = malloc(MAX_COMMANDS * sizeof(char **));
    if (!commands) {
        perror("malloc");
        return NULL;
    }
    
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        free(commands);
        perror("strdup");
        return NULL;
    }
    
    int cmd_count = 0;
    char *saveptr;
    char *pipe_token = strtok_r(cmd_copy, "|", &saveptr);
    
    while (pipe_token != NULL && cmd_count < MAX_COMMANDS - 1) {
        // Trim whitespace from the command segment
        pipe_token = trim_whitespace(pipe_token);
        
        // Skip empty segments
        if (strlen(pipe_token) == 0) {
            pipe_token = strtok_r(NULL, "|", &saveptr);
            continue;
        }
        
        // Parse this command segment into arguments
        char **args = parse_single_command(pipe_token);
        if (!args) {
            // Cleanup on error
            for (int i = 0; i < cmd_count; i++) {
                for (int j = 0; commands[i][j] != NULL; j++) {
                    free(commands[i][j]);
                }
                free(commands[i]);
            }
            free(commands);
            free(cmd_copy);
            return NULL;
        }
        
        commands[cmd_count] = args;
        cmd_count++;
        pipe_token = strtok_r(NULL, "|", &saveptr);
    }
    
    commands[cmd_count] = NULL;  // NULL terminate the array
    free(cmd_copy);
    
    return commands;
}

void free_commands(char ***commands) {
    if (commands == NULL) {
        return;
    }
    
    for (int i = 0; commands[i] != NULL; i++) {
        for (int j = 0; commands[i][j] != NULL; j++) {
            free(commands[i][j]);
        }
        free(commands[i]);
    }
    free(commands);
}
