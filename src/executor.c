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

/**
 * Execute commands with separator information (handles pipes and &&)
 */
int execute_command_list(command_t *commands, int count) {
    if (commands == NULL || count == 0) {
        return 0;
    }
    
    int result = 0;
    int i = 0;
    
    while (i < count) {
        // Find the pipeline segment (all commands connected by pipes until && or end)
        int pipeline_start = i;
        int pipeline_count = 1;
        
        // Count consecutive pipe-separated commands
        while (i < count && commands[i].sep == SEP_PIPE) {
            pipeline_count++;
            i++;
        }
        
        // Now i points to the last command in this pipeline segment
        // Execute the pipeline
        int fd_read = -1;
        int fd_write;
        int fd_pipe[2];
        
        for (int j = 0; j < pipeline_count; j++) {
            int cmd_idx = pipeline_start + j;
            
            // Create pipe if this is not the last command in pipeline
            if (j + 1 < pipeline_count) {
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
            
            result = execute_single_command(commands[cmd_idx].args, fd_read, fd_write);
            
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
            if (j + 1 < pipeline_count) {
                fd_read = fd_pipe[PIPE_READ];
            }
        }
        
        // Check if we should continue (for && operator)
        if (i < count && commands[i].sep == SEP_AND) {
            // If the pipeline failed, don't execute the next command
            if (result != 0) {
                // Skip remaining commands after failed &&
                break;
            }
        }
        
        // Move to next command/pipeline
        i++;
    }
    
    return result;
}

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
        
        result = execute_single_command(commands[i], fd_read, fd_write);
        
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

int execute_single_command(char **command, int fd_read, int fd_write) {
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
    return execute_external(command, fd_read, fd_write);
}

int execute_external(char **command, int fd_read, int fd_write) {
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
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        
        // Re-enable raw mode after child finishes
        if (was_raw_mode) {
            enable_raw_mode();
        }
        
        // Return the exit status of the child process
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return 1;  // Command was terminated by signal
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
