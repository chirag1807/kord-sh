#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "../include/executor.h"
#include "../include/builtins.h"
#include "../include/raw_input.h"
#include "../include/variables.h"
#include "../include/parser.h"
#include "../include/jobs.h"

int execute_command(char ***commands) {
    if (commands == NULL || commands[0] == NULL) {
        return 0;  // Empty command
    }
    
    int result = 0;
    int command_count = 0;
    
    // Count number of commands
    while (commands[command_count] != NULL) {
        command_count++;
    }
    
    // Check if this is a background command (only applies to single commands, not pipes)
    int background = 0;
    if (command_count == 1) {
        background = is_background_command(commands[0]);
    }

    int fd_read = -1;  // -1 means first command reads from STDIN
    int fd_write;
    int fd_pipe[2];

    for (int i = 0; i < command_count; i++) {
        // Create pipe if this is not the last command
        if ((i + 1) < command_count) {
            if (pipe(fd_pipe) == -1) {
                perror("pipe");
                if (fd_read != -1) {
                    close(fd_read);
                }
                return 1;
            }
            fd_write = fd_pipe[PIPE_WRITE];
        } else {
            fd_write = -1;  // Last command writes to STDOUT
        }
        
        // Only the last command in a pipeline can be backgrounded
        int is_background = (background && i == command_count - 1);
        result = execute_single_command(commands[i], fd_read, fd_write, is_background);
        
        // If command returns -1, it means exit command
        if (result == -1) {
            if (fd_read != -1) close(fd_read);
            if (fd_write != -1) close(fd_write);
            return -1;
        }

        // Close file descriptors in parent process
        if (fd_read != -1) close(fd_read);
        if (fd_write != -1) close(fd_write);
        
        // Next command reads from this pipe's read end
        if ((i + 1) < command_count) {
            fd_read = fd_pipe[PIPE_READ];
        }
    }
    
    return result;
}

int execute_single_command(char **command, int fd_read, int fd_write, int background) {
    // here, command[0] would be the actual command while subsequent array values would be its args.
    if (command == NULL || command[0] == NULL) {
        return 0;  // Empty command
    }
    
    // Check if it's a variable assignment (VAR=value)
    if (is_variable_assignment(command)) {
        return execute_variable_assignment(command);
    }
    
    // Check if it's a built-in command and must run in parent process
    if (is_builtin(command[0]) && (must_run_in_parent(command[0]))) {
        return execute_builtin(command);
    }
    
    // Otherwise, execute as external command
    return execute_external(command, fd_read, fd_write, background);
}

int execute_external(char **command, int fd_read, int fd_write, int background) {
    // Temporarily disable raw mode so child processes get normal terminal settings (cooked mode)
    int was_raw_mode = is_raw_mode_enabled();
    if (was_raw_mode) {
        disable_raw_mode();
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        // Re-enable raw mode if it was enabled before
        if (was_raw_mode) {
            enable_raw_mode();
        }
        return 1;
    }
    else if (pid == 0) {
        if (fd_read != -1 && fd_read != STDIN_FILENO) {
            dup2(fd_read, STDIN_FILENO);
            close(fd_read);
        }
        if (fd_write != -1 && fd_write != STDOUT_FILENO) {
            dup2(fd_write, STDOUT_FILENO);
            close(fd_write);
        }

        apply_io_redirection(command);

        // Child process
        // Check if it's a builtin that can run in child (like pwd, echo in pipes)
        if (is_builtin(command[0]) && !must_run_in_parent(command[0])) {
            int result = execute_builtin(command);
            exit(result);
        }
        
        // External command
        if (execvp(command[0], command) == -1) {
            perror("kord-sh");
        }

        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        if (background) {
            // Build command string for display
            char cmd_str[1024] = {0};
            for (int i = 0; command[i] != NULL; i++) {
                if (i > 0) strncat(cmd_str, " ", sizeof(cmd_str) - strlen(cmd_str) - 1);
                strncat(cmd_str, command[i], sizeof(cmd_str) - strlen(cmd_str) - 1);
            }
            
            int job_id = add_job(pid, cmd_str);
            if (job_id != -1) {
                printf("[%d] %d\n", job_id, pid);
            }
            
            // Don't wait for background job
        } else {
            // Foreground job - wait for completion
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        
        // Re-enable raw mode after child finishes
        if (was_raw_mode) {
            enable_raw_mode();
        }
    }
    
    return 0;
}

void apply_io_redirection(char **command) {
    if (command == NULL) {
        return;
    }

    int read_idx = 0;   // Index for reading from original command
    int write_idx = 0;  // Index for writing valid args back to command

    while (command[read_idx] != NULL) {
        // Check for input redirection: <
        if (strcmp(command[read_idx], "<") == 0) {
            if (command[read_idx + 1] == NULL) {
                fprintf(stderr, "kord-sh: syntax error: expected filename after '<'\n");
                exit(EXIT_FAILURE);
            }
            
            int fd = open(command[read_idx + 1], O_RDONLY);
            if (fd == -1) {
                perror(command[read_idx + 1]);
                exit(EXIT_FAILURE);
            }
            
            dup2(fd, STDIN_FILENO);
            close(fd);
            
            read_idx += 2;  // Skip both '<' and filename
        }
        // Check for append redirection: >>
        else if (strcmp(command[read_idx], ">>") == 0) {
            if (command[read_idx + 1] == NULL) {
                fprintf(stderr, "kord-sh: syntax error: expected filename after '>>'\n");
                exit(EXIT_FAILURE);
            }
            
            int fd = open(command[read_idx + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror(command[read_idx + 1]);
                exit(EXIT_FAILURE);
            }
            
            dup2(fd, STDOUT_FILENO);
            close(fd);
            
            read_idx += 2;  // Skip both '>>' and filename
        }
        // Check for output redirection: >
        else if (strcmp(command[read_idx], ">") == 0) {
            if (command[read_idx + 1] == NULL) {
                fprintf(stderr, "kord-sh: syntax error: expected filename after '>'\n");
                exit(EXIT_FAILURE);
            }
            
            int fd = open(command[read_idx + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror(command[read_idx + 1]);
                exit(EXIT_FAILURE);
            }
            
            dup2(fd, STDOUT_FILENO);
            close(fd);
            
            read_idx += 2;  // Skip both '>' and filename
        }
        // Regular argument - keep it
        else {
            // Only move if indices differ (optimization)
            if (write_idx != read_idx) {
                command[write_idx] = command[read_idx];
            }
            write_idx++;
            read_idx++;
        }
    }

    // NULL-terminate at the new end position
    command[write_idx] = NULL;
}
