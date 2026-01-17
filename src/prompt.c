#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>
#include "../include/colors.h"
#include "../include/prompt.h"
#include "../include/raw_input.h"

#define SHELL_VERSION "1.0.0"

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
    // Use raw mode if enabled, otherwise use cooked mode
    if (is_raw_mode_enabled()) {
        return read_input_raw(command, 1024);
    }
    
    // Cooked mode (original implementation)
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

void print_welcome(void) {
    struct utsname sys_info;
    struct passwd *pw = getpwuid(getuid());
    char *username = pw ? pw->pw_name : getenv("USER");
    if (!username) username = "user";
    
    // Get system info
    if (uname(&sys_info) == -1) {
        perror("uname");
    }
    
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%B %d, %Y at %I:%M %p", t);
    
    // Print banner
    printf("\n");
    printf("%s╔═════════════════════════════════════════════════════════════════╗%s\n", COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s                                                                 %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s██╗  ██╗ ██████╗ ██████╗ ██████╗       ███████╗██╗  ██╗%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s██║ ██╔╝██╔═══██╗██╔══██╗██╔══██╗      ██╔════╝██║  ██║%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s█████╔╝ ██║   ██║██████╔╝██║  ██║█████╗███████╗███████║%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s██╔═██╗ ██║   ██║██╔══██╗██║  ██║╚════╝╚════██║██╔══██║%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s██║  ██╗╚██████╔╝██║  ██║██████╔╝      ███████║██║  ██║%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s     %s╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═════╝       ╚══════╝╚═╝  ╚═╝%s     %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s                                                                 %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s╚═════════════════════════════════════════════════════════════════╝%s\n", COLOR_BOLD_CYAN, COLOR_RESET);
    printf("\n");
    
    printf("  %s⚡ Version:%s %s%s%s\n", COLOR_BOLD_YELLOW, COLOR_RESET, COLOR_BOLD_WHITE, SHELL_VERSION, COLOR_RESET);
    printf("  %s👤 User:%s    %s%s%s\n", COLOR_BOLD_YELLOW, COLOR_RESET, COLOR_BOLD_WHITE, username, COLOR_RESET);
    
    if (uname(&sys_info) != -1) {
        printf("  %s💻 System:%s  %s%s %s%s\n", COLOR_BOLD_YELLOW, COLOR_RESET, COLOR_BOLD_WHITE, sys_info.sysname, sys_info.machine, COLOR_RESET);
    }
    
    printf("  %s📅 Date:%s    %s%s%s\n", COLOR_BOLD_YELLOW, COLOR_RESET, COLOR_BOLD_WHITE, time_str, COLOR_RESET);
    printf("\n");
    printf("  %sType 'help' for available commands, or press Ctrl+D to exit%s\n", COLOR_DIM, COLOR_RESET);
    printf("\n");
}

void print_goodbye(void) {
    if (is_raw_mode_enabled()) {
        disable_raw_mode();
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%I:%M %p", t);
    
    printf("\n");
    printf("%s╔═══════════════════════════════════════════════════════════╗%s\n", COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s                                                           %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s              %s🌟 Thank you for using KORD-SH! 🌟%s           %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_GREEN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s║%s                                                           %s║%s\n", COLOR_BOLD_CYAN, COLOR_RESET, COLOR_BOLD_CYAN, COLOR_RESET);
    printf("%s╚═══════════════════════════════════════════════════════════╝%s\n", COLOR_BOLD_CYAN, COLOR_RESET);
    printf("\n");
    printf("  %sSession ended at %s%s%s%s\n", COLOR_DIM, COLOR_RESET, COLOR_BOLD_WHITE, time_str, COLOR_RESET);
    printf("  %sGoodbye! 👋%s\n\n", COLOR_BOLD_YELLOW, COLOR_RESET);
}
