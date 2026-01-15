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

int read_user_input(char *command);

#endif // PROMPT_H
