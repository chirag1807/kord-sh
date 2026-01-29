#include "../include/common.h"
#include "../include/raw_input.h"
#include "../include/history.h"
#include "../include/prompt.h"

/* Word boundary characters for navigation */
#define IS_WORD_BOUNDARY(c) ((c) == ' ' || (c) == '\t' || (c) == '/' || (c) == '.' || (c) == '-' || (c) == '_' || (c) == '=' || (c) == ':' || (c) == ';')

/* Terminal state */
static struct termios original_termios;
static int raw_mode_active = 0;

void disable_raw_mode(void) {
    if (raw_mode_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
        raw_mode_active = 0;
    }
}

int enable_raw_mode(void) {
    if (raw_mode_active) {
        return 0;  // Already in raw mode
    }
    
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        return -1;
    }
    
    // Only register cleanup function on first call to current function (enable_raw_mode)
    static int atexit_registered = 0;
    if (!atexit_registered) {
        atexit(disable_raw_mode);
        atexit_registered = 1;
    }
    
    struct termios raw = original_termios;
    
    /* Disable:
     * - IXON: Software flow control (Ctrl+S, Ctrl+Q)
     * - ICRNL: Carriage return to newline translation
     * - BRKINT: Break condition causes SIGINT
     * - INPCK: Input parity checking
     * - ISTRIP: Strip 8th bit of characters
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    
    /* Disable:
     * - OPOST: Output processing (enables raw output)
     */
    raw.c_oflag &= ~(OPOST);
    
    /* Set:
     * - CS8: 8 bits per byte
     */
    raw.c_cflag |= (CS8);
    
    /* Disable:
     * - ECHO: Don't echo typed characters
     * - ICANON: Canonical mode (line buffering)
     * - IEXTEN: Extended input processing (Ctrl+V)
     * - ISIG: Signal generation (Ctrl+C, Ctrl+Z)
     */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    /* Control characters:
     * - VMIN = 1: read() returns as soon as data is available
     * - VTIME = 0: no timeout
     */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    raw_mode_active = 1;
    return 0;
}

int is_raw_mode_enabled(void) {
    return raw_mode_active;
}

/**
 * Read a single byte from stdin
 */
static int read_byte(void) {
    char c;
    int nread = read(STDIN_FILENO, &c, 1);
    return (nread == 1) ? c : -1;
}

/**
 * Write data to stdout
 */
static void write_stdout(const char *data, size_t len) {
    write(STDOUT_FILENO, data, len);
}

/**
 * Move cursor left by n positions
 */
static void move_cursor_left(int n) {
    if (n <= 0) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "\033[%dD", n);
    write_stdout(buf, strlen(buf));
}

/**
 * Move cursor right by n positions
 */
static void move_cursor_right(int n) {
    if (n <= 0) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "\033[%dC", n);
    write_stdout(buf, strlen(buf));
}

/**
 * Clear from cursor to end of line
 */
static void clear_to_end(void) {
    write_stdout("\033[K", 3);
}

/**
 * Handle backspace: remove character before cursor
 */
static void handle_backspace(char *buffer, int *cursor, int *length) {
    if (*cursor <= 0) return;
    
    // Shift characters left
    memmove(&buffer[*cursor - 1], &buffer[*cursor], *length - *cursor);
    (*cursor)--;
    (*length)--;
    buffer[*length] = '\0';
    
    // move back one position
    write_stdout("\b", 1);
    write_stdout(&buffer[*cursor], *length - *cursor);
    write_stdout(" ", 1);  // Clear the last character
    
    // Move cursor back to correct position
    move_cursor_left(*length - *cursor + 1);
}

/**
 * Handle delete: remove character at cursor position
 */
static void handle_delete(char *buffer, int *cursor, int *length) {
    if (*cursor >= *length) return;
    
    // Shift characters left
    memmove(&buffer[*cursor], &buffer[*cursor + 1], *length - *cursor - 1);
    (*length)--;
    buffer[*length] = '\0';
    
    // Visual feedback: clear to end, reprint remaining text
    write_stdout(&buffer[*cursor], *length - *cursor);
    write_stdout(" ", 1);  // Clear the last character
    
    // Move cursor back to correct position
    move_cursor_left(*length - *cursor + 1);
}

/**
 * Move cursor to next word (Ctrl+Right)
 */
static void move_cursor_next_word(const char *buffer, int *cursor, int length) {
    if (*cursor >= length) return;
    
    // Skip any word boundaries
    while (*cursor < length && IS_WORD_BOUNDARY(buffer[*cursor])) {
        write_stdout(&buffer[*cursor], 1);
        (*cursor)++;
    }
    
    // Move to end of current word
    while (*cursor < length && !IS_WORD_BOUNDARY(buffer[*cursor])) {
        write_stdout(&buffer[*cursor], 1);
        (*cursor)++;
    }
}

/**
 * Move cursor to previous word (Ctrl+Left)
 */
static void move_cursor_prev_word(const char *buffer, int *cursor) {
    if (*cursor <= 0) return;
    
    int original_cursor = *cursor;
    
    // Skip any word boundaries before cursor
    while (*cursor > 0 && IS_WORD_BOUNDARY(buffer[*cursor - 1])) {
        (*cursor)--;
    }
    
    // Move to start of current word
    while (*cursor > 0 && !IS_WORD_BOUNDARY(buffer[*cursor - 1])) {
        (*cursor)--;
    }
    
    // Visual feedback
    move_cursor_left(original_cursor - *cursor);
}

/**
 * Delete word forward (Ctrl+Delete)
 */
static void delete_word_forward(char *buffer, int *cursor, int *length) {
    if (*cursor >= *length) return;
    
    int start = *cursor;
    int end = *cursor;
    
    // Skip word boundaries
    while (end < *length && IS_WORD_BOUNDARY(buffer[end])) {
        end++;
    }
    
    // Delete word characters
    while (end < *length && !IS_WORD_BOUNDARY(buffer[end])) {
        end++;
    }
    
    if (end > start) {
        // Shift remaining characters left
        memmove(&buffer[start], &buffer[end], *length - end);
        *length -= (end - start);
        buffer[*length] = '\0';
        
        // Visual feedback
        write_stdout(&buffer[*cursor], *length - *cursor);
        // Clear extra characters
        for (int i = 0; i < (end - start); i++) {
            write_stdout(" ", 1);
        }
        move_cursor_left(*length - *cursor + (end - start));
    }
}

/**
 * Delete word backward (Ctrl+Backspace)
 */
static void delete_word_backward(char *buffer, int *cursor, int *length) {
    if (*cursor <= 0) return;
    
    int end = *cursor;
    int start = *cursor;
    
    // Skip word boundaries before cursor
    while (start > 0 && IS_WORD_BOUNDARY(buffer[start - 1])) {
        start--;
    }
    
    // Delete word characters
    while (start > 0 && !IS_WORD_BOUNDARY(buffer[start - 1])) {
        start--;
    }
    
    if (start < end) {
        // Shift remaining characters left
        memmove(&buffer[start], &buffer[end], *length - end);
        *length -= (end - start);
        buffer[*length] = '\0';
        *cursor = start;
        
        // Visual feedback
        move_cursor_left(end - start);
        write_stdout(&buffer[*cursor], *length - *cursor);
        // Clear extra characters
        for (int i = 0; i < (end - start); i++) {
            write_stdout(" ", 1);
        }
        move_cursor_left(*length - *cursor + (end - start));
    }
}

/**
 * Insert character at cursor position
 */
static void insert_char(char *buffer, int *cursor, int *length, char c, size_t buffer_size) {
    if (*length >= (int)buffer_size - 1) return;
    
    // Shift characters right to make room
    memmove(&buffer[*cursor + 1], &buffer[*cursor], *length - *cursor);
    buffer[*cursor] = c;
    (*length)++;
    buffer[*length] = '\0';
    
    // Visual feedback: write character and remaining text
    write_stdout(&buffer[*cursor], *length - *cursor);
    (*cursor)++;
    
    // Move cursor back to correct position
    move_cursor_left(*length - *cursor);
}

/**
 * Find all files/directories matching prefix
 */
static char **find_completions(const char *prefix, int *count) {
    *count = 0;
    
    char *last_slash = strrchr(prefix, '/');
    const char *dir_path = ".";
    const char *file_prefix = prefix;
    char dir_buffer[1024];
    
    if (last_slash != NULL) {
        // Extract directory path and file prefix
        size_t dir_len = last_slash - prefix;
        if (dir_len == 0) {
            dir_path = "/";
        } else {
            strncpy(dir_buffer, prefix, dir_len);
            dir_buffer[dir_len] = '\0';
            dir_path = dir_buffer;
        }
        file_prefix = last_slash + 1;
    }
    
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        return NULL;
    }
    
    // First pass: count matches
    struct dirent *entry;
    size_t prefix_len = strlen(file_prefix);
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if name starts with prefix (case insensitive)
        // If prefix is empty, match all files
        if (prefix_len == 0 || strncasecmp(entry->d_name, file_prefix, prefix_len) == 0) {
            (*count)++;
        }
    }
    
    if (*count == 0) {
        closedir(dir);
        return NULL;
    }
    
    // Allocate array for matches
    char **matches = malloc((*count + 1) * sizeof(char *));
    if (matches == NULL) {
        closedir(dir);
        return NULL;
    }
    
    // Second pass: collect matches
    rewinddir(dir);
    int idx = 0;
    while ((entry = readdir(dir)) != NULL && idx < *count) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if name starts with prefix (case insensitive)
        // If prefix is empty, match all files
        if (prefix_len == 0 || strncasecmp(entry->d_name, file_prefix, prefix_len) == 0) {
            // Build full path for stat
            char full_path[2048];
            if (last_slash != NULL) {
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            }
            
            // Check if it's a directory
            struct stat st;
            int is_dir = 0;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = 1;
            }
            
            // Allocate and store match (add / for directories)
            size_t name_len = strlen(entry->d_name);
            matches[idx] = malloc(name_len + 2);  // +1 for /, +1 for null
            if (matches[idx] == NULL) {
                // Cleanup on error
                for (int i = 0; i < idx; i++) {
                    free(matches[i]);
                }
                free(matches);
                closedir(dir);
                return NULL;
            }
            
            strcpy(matches[idx], entry->d_name);
            if (is_dir) {
                strcat(matches[idx], "/");
            }
            idx++;
        }
    }
    
    closedir(dir);
    matches[idx] = NULL;
    *count = idx;
    
    return matches;
}

/**
 * Handle tab completion
 */
static void handle_tab_completion(char *buffer, int *cursor, int *length, size_t buffer_size) {
    // Get word at cursor - extract the current word from buffer
    char word[256] = {0};
    int word_start = *cursor;
    
    // Find start of current word (go back to whitespace or beginning)
    while (word_start > 0 && !isspace((unsigned char)buffer[word_start - 1])) {
        word_start--;
    }
    
    // Extract word from word_start to cursor
    int word_len = *cursor - word_start;
    if (word_len > 0 && word_len < 256) {
        strncpy(word, &buffer[word_start], word_len);
        word[word_len] = '\0';
    }
    
    // tab completions requires at least one character
    if (word_len == 0) {
        return;
    }
    
    // Find completions
    int match_count = 0;
    char **matches = find_completions(word, &match_count);
    
    if (match_count == 0) {
        // No matches - beep or do nothing
        return;
    }
    
    if (match_count == 1) {
        // Single match - complete it
        const char *completion = matches[0];
        
        // Extract just the filename part from word for comparison
        const char *word_filename = word;
        char *last_slash_in_word = strrchr(word, '/');
        size_t dir_prefix_len = 0;
        if (last_slash_in_word != NULL) {
            word_filename = last_slash_in_word + 1;
            dir_prefix_len = (last_slash_in_word - word) + 1;  // Include the slash
        }
        
        size_t word_filename_len = strlen(word_filename);
        size_t completion_len = strlen(completion);
        
        // Calculate what to add: the part of completion that wasn't typed
        const char *to_add = completion + word_filename_len;
        size_t add_len = strlen(to_add);
        
        // Check if we have space
        if (*length + add_len >= buffer_size - 1) {
            free(matches[0]);
            free(matches);
            return;
        }
        
        // Remove old word from cursor to end of word (if any characters extend beyond cursor)
        int word_end = *cursor;
        while (word_end < *length && !isspace((unsigned char)buffer[word_end])) {
            word_end++;
        }
        
        if (word_end > *cursor) {
            memmove(&buffer[*cursor], &buffer[word_end], *length - word_end);
            *length -= (word_end - *cursor);
            buffer[*length] = '\0';
            
            // Clear and redraw
            clear_to_end();
            write_stdout(&buffer[*cursor], *length - *cursor);
            move_cursor_left(*length - *cursor);
        }
        
        // We need to replace only the filename part (after the last slash), keeping directory prefix
        // Calculate position where filename starts in the buffer
        int filename_start = word_start + dir_prefix_len;
        
        // Move cursor to the start of the filename (not the whole word)
        if (*cursor > filename_start) {
            int chars_back = *cursor - filename_start;
            move_cursor_left(chars_back);
            *cursor = filename_start;
        }
        
        // Remove the typed filename part
        int filename_chars = word_filename_len;
        if (filename_chars > 0) {
            memmove(&buffer[*cursor], &buffer[*cursor + filename_chars], *length - *cursor - filename_chars);
            *length -= filename_chars;
            buffer[*length] = '\0';
            
            // Redraw from cursor position
            clear_to_end();
            write_stdout(&buffer[*cursor], *length - *cursor);
            move_cursor_left(*length - *cursor);
        }
        
        // Insert the complete filename with correct case
        for (size_t i = 0; i < completion_len && *length < (int)buffer_size - 1; i++) {
            insert_char(buffer, cursor, length, completion[i], buffer_size);
        }
        
        free(matches[0]);
        free(matches);
    } else {
        // Multiple matches - display them
        write_stdout("\n\r", 2);
        
        // Sort matches for better display
        for (int i = 0; i < match_count - 1; i++) {
            for (int j = i + 1; j < match_count; j++) {
                if (strcmp(matches[i], matches[j]) > 0) {
                    char *temp = matches[i];
                    matches[i] = matches[j];
                    matches[j] = temp;
                }
            }
        }
        
        // Display matches in columns
        int max_len = 0;
        for (int i = 0; i < match_count; i++) {
            int len = strlen(matches[i]);
            if (len > max_len) max_len = len;
        }
        
        int cols = 80 / (max_len + 2);
        if (cols < 1) cols = 1;
        
        for (int i = 0; i < match_count; i++) {
            write_stdout(matches[i], strlen(matches[i]));
            
            if ((i + 1) % cols == 0 || i == match_count - 1) {
                write_stdout("\n\r", 2);
            } else {
                int padding = max_len + 2 - strlen(matches[i]);
                for (int p = 0; p < padding; p++) {
                    write_stdout(" ", 1);
                }
            }
        }
        
        // Redraw prompt and current input
        char *prompt_str = build_prompt();
        if (prompt_str) {
            write_stdout(prompt_str, strlen(prompt_str));
            free(prompt_str);
        }
        write_stdout(buffer, *length);
        move_cursor_left(*length - *cursor);
        
        // Cleanup
        for (int i = 0; i < match_count; i++) {
            free(matches[i]);
        }
        free(matches);
    }
}


int read_input_raw(char *buffer, size_t buffer_size) {
    if (!raw_mode_active) {
        fprintf(stderr, "Error: Raw mode not enabled\n");
        return -1;
    }
    
    int cursor = 0;    // Current cursor position in buffer
    int length = 0;    // Current length of input
    memset(buffer, 0, buffer_size);
    
    // History navigation state
    static int history_index = -1;  // -1 means not navigating history
    static int from_history = 0;    // 1 if current buffer is from history

    while (1) {
        int c = read_byte();
        
        if (c == -1) {
            continue;  // Timeout, no input
        }
        
        // Handle escape sequences (arrow keys, etc.)
        if (c == 27) {  // ESC
            int c2 = read_byte();
            if (c2 == '[') {
                int c3 = read_byte();
                
                if (c3 == 'A') {
                    // UP arrow - navigate history backwards (newer to older)
                    int hist_count = get_history_count();
                    if (hist_count > 0) {
                        // Initialize history navigation
                        if (history_index == -1) {
                            history_index = hist_count - 1;
                        } else if (history_index > 0) {
                            history_index--;
                        } else {
                            // Already at oldest entry
                            continue;
                        }
                        
                        const char *hist_cmd = get_history(history_index);
                        if (hist_cmd != NULL) {
                            // Clear current line
                            move_cursor_left(cursor);
                            clear_to_end();
                            
                            // Copy history command to buffer
                            strncpy(buffer, hist_cmd, buffer_size - 1);
                            buffer[buffer_size - 1] = '\0';
                            length = strlen(buffer);
                            cursor = length;
                            from_history = 1;
                            
                            // Display the command
                            write_stdout(buffer, length);
                        }
                    }
                    continue;
                } else if (c3 == 'B') {
                    // DOWN arrow - navigate history forwards (older to newer)
                    int hist_count = get_history_count();
                    if (hist_count > 0 && history_index != -1) {
                        if (history_index < hist_count - 1) {
                            history_index++;
                            
                            const char *hist_cmd = get_history(history_index);
                            if (hist_cmd != NULL) {
                                // Clear current line
                                move_cursor_left(cursor);
                                clear_to_end();
                                
                                // Copy history command to buffer
                                strncpy(buffer, hist_cmd, buffer_size - 1);
                                buffer[buffer_size - 1] = '\0';
                                length = strlen(buffer);
                                cursor = length;
                                from_history = 1;
                                
                                // Display the command
                                write_stdout(buffer, length);
                            }
                        } else {
                            // Go past newest entry - clear line
                            history_index = -1;
                            from_history = 0;
                            
                            // Clear current line
                            move_cursor_left(cursor);
                            clear_to_end();
                            
                            buffer[0] = '\0';
                            length = 0;
                            cursor = 0;
                        }
                    }
                    continue;
                } else if (c3 == 'C') {
                    // RIGHT arrow - move cursor right
                    if (cursor < length) {
                        write_stdout(&buffer[cursor], 1);
                        cursor++;
                    }
                    continue;
                } else if (c3 == 'D') {
                    // LEFT arrow - move cursor left
                    if (cursor > 0) {
                        write_stdout("\b", 1);
                        cursor--;
                    }
                    continue;
                } else if (c3 == 'H') {
                    // HOME - move to beginning
                    move_cursor_left(cursor);
                    cursor = 0;
                    continue;
                } else if (c3 == 'F') {
                    // END - move to end
                    move_cursor_right(length - cursor);
                    cursor = length;
                    continue;
                } else if (c3 == '3') {
                    // DELETE key sequence is ESC [ 3 ~
                    // Or Ctrl+Delete: ESC [ 3 ; 5 ~
                    int c4 = read_byte();
                    if (c4 == '~') {
                        handle_delete(buffer, &cursor, &length);
                    } else if (c4 == ';') {
                        int c5 = read_byte();
                        int c6 = read_byte();
                        if (c5 == '5' && c6 == '~') {
                            // Ctrl+Delete - delete word forward
                            delete_word_forward(buffer, &cursor, &length);
                        }
                    }
                    continue;
                } else if (c3 == '1') {
                    // Ctrl+Arrow keys: ESC [ 1 ; 5 C/D
                    int c4 = read_byte();
                    if (c4 == ';') {
                        int c5 = read_byte();
                        int c6 = read_byte();
                        if (c5 == '5') {
                            if (c6 == 'C') {
                                // Ctrl+Right - move to next word
                                move_cursor_next_word(buffer, &cursor, length);
                            } else if (c6 == 'D') {
                                // Ctrl+Left - move to previous word
                                move_cursor_prev_word(buffer, &cursor);
                            }
                        }
                    }
                    continue;
                } else if (c3 == ';') {
                    // Alternative Ctrl+Arrow format: ESC [ ; 5 C/D
                    int c4 = read_byte();
                    int c5 = read_byte();
                    if (c4 == '5') {
                        if (c5 == 'C') {
                            // Ctrl+Right
                            move_cursor_next_word(buffer, &cursor, length);
                        } else if (c5 == 'D') {
                            // Ctrl+Left
                            move_cursor_prev_word(buffer, &cursor);
                        }
                    }
                    continue;
                }
            } else if (c2 == 127 || c2 == 8) {
                // Some terminals send ESC+Backspace for Ctrl+Backspace
                delete_word_backward(buffer, &cursor, &length);
                continue;
            }
            continue;
        }
        
        // Handle special characters
        if (c == 1) {  // Ctrl+A - move to beginning
            move_cursor_left(cursor);
            cursor = 0;
            continue;
        } else if (c == 4) {  // Ctrl+D (EOF)
            if (length == 0) {
                return -1;  // EOF on empty line
            }
            continue;
        } else if (c == 3) {  // Ctrl+C
            write_stdout("^C\n\r", 4);
            buffer[0] = '\0';
            return 0;
        } else if (c == 23) {  // Ctrl+W - delete word backward
            delete_word_backward(buffer, &cursor, &length);
            continue;
        } else if (c == '\r' || c == '\n') {  // Enter
            write_stdout("\n\r", 2);
            buffer[length] = '\0';
            
            // If command is from history and not modified, move it to latest position
            if (from_history && history_index != -1) {
                const char *hist_cmd = get_history(history_index);
                if (hist_cmd != NULL && strcmp(buffer, hist_cmd) == 0) {
                    move_history_to_latest(history_index);
                }
            }
            
            // Reset history navigation state
            history_index = -1;
            from_history = 0;
            
            return length;
        } else if (c == 127 || c == 8) {  // Backspace or DEL
            // If user modifies history command, it's no longer from history
            if (from_history) {
                from_history = 0;
            }
            handle_backspace(buffer, &cursor, &length);
        } else if (c == '\t') {  // Tab
            // Tab completion for files and directories
            handle_tab_completion(buffer, &cursor, &length, buffer_size);
            continue;
        } else if (c >= 32 && c < 127) {  // Printable character
            // If user types anything, reset history navigation
            if (from_history) {
                from_history = 0;
            }
            if (history_index != -1) {
                history_index = -1;
            }
            insert_char(buffer, &cursor, &length, c, buffer_size);
        }
        // Ignore other control characters
    }
    
    return length;
}
