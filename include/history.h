#ifndef HISTORY_H
#define HISTORY_H

#define MAX_HISTORY 50

/**
 * Initialize history system
 * Loads history from ~/.kord_history file
 */
void init_history(void);

/**
 * Cleanup history system
 * Saves history to file and frees all allocated memory
 */
void cleanup_history(void);

/**
 * Add command to history
 * If command count exceeds MAX_HISTORY, oldest entry is removed
 */
void add_history(const char *command);

/**
 * Get history entry at index (0 = oldest, count-1 = newest)
 * Returns NULL if index out of bounds
 */
const char *get_history(int index);

/**
 * Get number of history entries
 */
int get_history_count(void);

/**
 * Move history entry to the latest position
 * Used when user selects a history command via arrow keys
 */
void move_history_to_latest(int index);

#endif // HISTORY_H
