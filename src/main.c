#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/colors.h"
#include "../include/prompt.h"
#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/raw_input.h"
#include "../include/variables.h"

int main()
{
    char command[1024];
    
    // Initialize variable system
    init_variables();
    
    // Print welcome banner
    print_welcome();
    
    // Enable raw mode for better input handling (arrow keys, immediate echo)
    // Comment out the next line if you prefer cooked mode
    if (enable_raw_mode() == -1) {
        fprintf(stderr, "%sWarning: Failed to enable raw mode, using cooked mode%s\n", COLOR_BOLD_YELLOW, COLOR_RESET);
    }

    while (1)
    {
        // Print shell prompt
        print_prompt();

        // Read user input
        int len = read_user_input(command);
        
        // Handle EOF (Ctrl+D)
        if (len == -1) {
            print_goodbye();
            break;
        }
        
        // Skip empty commands
        if (len == 0) {
            continue;
        }

        // Parse command into array of commands (for pipes)
        char ***commands = parse_command(command);
        if (commands == NULL) {
            continue;
        }

        // Execute commands
        int result = execute_command(commands);
        
        // Free commands
        free_commands(commands);
        
        // Check if shell should exit (exit command returns -1)
        if (result == -1) {
            print_goodbye();
            break;
        }
    }
    
    // Cleanup variable system
    cleanup_variables();
    
    // Raw mode is automatically disabled on exit via atexit()
    return EXIT_SUCCESS;
}
