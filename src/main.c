#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/prompt.h"
#include "../include/parser.h"
#include "../include/executor.h"

int main()
{
    char command[1024];

    while (1)
    {
        // Print shell prompt
        print_prompt();

        // Read user input
        int len = read_user_input(command);
        
        // Handle EOF (Ctrl+D)
        if (len == -1) {
            printf("\nlogout\n");
            break;
        }
        
        // Skip empty commands
        if (len == 0) {
            continue;
        }

        // Parse command into arguments
        char **args = parse_command(command);
        if (args == NULL) {
            continue;
        }

        // Execute command
        int result = execute_command(args);
        
        // Free arguments
        free_args(args);
        
        // Check if shell should exit (exit command returns -1)
        if (result == -1) {
            break;
        }
    }

    return EXIT_SUCCESS;
}
