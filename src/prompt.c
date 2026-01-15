#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <string.h>
#include "../include/colors.h"
#include "../include/prompt.h"

void print_prompt(void)
{
    char *prompt = build_prompt();
    if (!prompt) {
        fprintf(stderr, "kord-sh$ ");
        fflush(stderr);
        return;
    }

    fprintf(stdout, "%s", prompt);
    fflush(stdout);
    free(prompt);
}

char *build_prompt() {
    char hostname[HOST_NAME_MAX + 1];
    char cwd[PATH_MAX];
    char *username;
    char *prompt;
    size_t PROMPT_SIZE;

    // Get current directory
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        return NULL;
    }

    // Get the hostname
    if (gethostname(hostname, sizeof(hostname)) == -1)
    {
        perror("gethostname");
        return NULL;
    }

    // Get the username
    username = getenv("USER");
    if (!username) {
        struct passwd *pw = getpwuid(getuid()); // Get the user info from the user ID
        username = pw ? pw->pw_name : "unknown";
    }

    // Calculate required size: color codes + username + hostname + cwd + formatting + null terminator
    // Color codes: ~100 bytes (to be safe), formatting chars: "@:$ " = 4 bytes
    PROMPT_SIZE = strlen(username) + strlen(hostname) + strlen(cwd) + 150;
    
    // Allocate prompt with calculated size
    prompt = malloc(PROMPT_SIZE);
    if (!prompt) {
        perror("malloc");
        return NULL;
    }

    snprintf(
        prompt,
        PROMPT_SIZE,
        "%s%s@%s%s%s:%s%s%s$ ",
        COLOR_BOLD_GREEN, username,
        COLOR_BOLD_CYAN, hostname,
        COLOR_WHITE,
        COLOR_BOLD_BLUE, cwd,
        COLOR_RESET
    );

    return prompt;
}
int read_user_input(char *command) {
    int ch;
    int i = 0;
    
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
        if (i < 1023) { // Prevent buffer overflow (assuming 1024 buffer size)
            command[i++] = (char)ch;
        }
    }
    command[i] = '\0';
    
    // Return -1 on EOF (Ctrl+D)
    return (ch == EOF) ? -1 : i;
}

