#ifndef VARIABLES_H
#define VARIABLES_H

/**
 * Initialize the variable system
 * Must be called before using any variable functions
 */
void init_variables(void);

/**
 * Clean up the variable system
 * Should be called before program exit
 */
void cleanup_variables(void);

/**
 * Set a shell variable (not exported to environment)
 * Returns 0 on success, -1 on failure
 */
int set_variable(const char *name, const char *value);

/**
 * Get a variable value (checks shell variables first, then environment)
 * Returns the value if found, NULL otherwise
 */
const char *get_variable(const char *name);

/**
 * Export a variable to the environment
 * If the variable exists as shell variable, it gets promoted to environment
 * If it doesn't exist, it gets created in the environment
 * Returns 0 on success, -1 on failure
 */
int export_variable(const char *name, const char *value);

/**
 * Unset a variable (removes from both shell variables and environment)
 * Returns 0 on success, -1 on failure
 */
int unset_variable(const char *name);

/**
 * Print all shell variables
 */
void print_variables(void);

/**
 * Check if a command is a variable assignment (VAR=value)
 * Returns 1 if it's an assignment, 0 otherwise
 */
int is_variable_assignment(char **command);

/**
 * Execute a variable assignment (VAR=value)
 * Returns 0 on success, 1 on failure
 */
int execute_variable_assignment(char **command);

#endif // VARIABLES_H
