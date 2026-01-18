#ifndef ALIASES_H
#define ALIASES_H

/**
 * Initialize alias system
 * Loads aliases from ~/.kordrc file
 */
void init_aliases(void);

/**
 * Cleanup alias system
 * Frees all allocated memory
 */
void cleanup_aliases(void);

/**
 * Set an alias
 * Returns 0 on success, -1 on failure
 */
int set_alias(const char *name, const char *value);

/**
 * Get an alias value
 * Returns NULL if alias not found
 */
const char *get_alias(const char *name);

/**
 * Remove an alias
 * Returns 0 on success, -1 if not found
 */
int unset_alias(const char *name);

/**
 * Print all aliases
 */
void print_aliases(void);

/**
 * Expand aliases in command if needed
 * Returns newly allocated string with expanded command (must be freed by caller)
 * Returns NULL if no expansion needed (use original command)
 */
char *expand_alias(const char *command);

#endif // ALIASES_H
