#ifndef PROMPT_H
#define PROMPT_H

/**
 * Display the shell prompt with username, hostname, and current directory
 */
void print_prompt(void);

/**
 * Build and return a dynamically allocated prompt string
 * Caller must free the returned string
 */
char *build_prompt(void);

/**
 * Read user input (raw mode or cooked mode)
 */
int read_user_input(char *command);

/**
 * Print welcome banner when shell starts
 */
void print_welcome(void);

/**
 * Print goodbye message when shell exits
 */
void print_goodbye(void);

#endif // PROMPT_H
