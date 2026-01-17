#ifndef RAW_INPUT_H
#define RAW_INPUT_H

/**
 * Enable raw mode on the terminal
 * Returns 0 on success, -1 on failure
 */
int enable_raw_mode(void);

/**
 * Disable raw mode and restore terminal to cooked mode
 */
void disable_raw_mode(void);

/**
 * Read user input in raw mode with immediate character echo
 * Supports:
 * - Arrow keys (up/down for history, left/right for cursor movement)
 * - Backspace
 * - Basic line editing
 */
int read_input_raw(char *buffer, size_t buffer_size);

/**
 * Check if raw mode is currently enabled
 * Returns 1 if enabled, 0 otherwise
 */
int is_raw_mode_enabled(void);

#endif // RAW_INPUT_H
