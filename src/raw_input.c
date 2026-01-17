#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include "../include/raw_input.h"

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
    
    // Visual feedback: move back, clear to end, reprint remaining text
    write_stdout("\b", 1);
    write_stdout(&buffer[*cursor], *length - *cursor);
    write_stdout(" ", 1);  // Clear the last character
    
    // Move cursor back to correct position
    move_cursor_left(*length - *cursor + 1);
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

int read_input_raw(char *buffer, size_t buffer_size) {
    if (!raw_mode_active) {
        fprintf(stderr, "Error: Raw mode not enabled\n");
        return -1;
    }
    
    int cursor = 0;    // Current cursor position in buffer
    int length = 0;    // Current length of input
    memset(buffer, 0, buffer_size);
    
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
                    // UP arrow - TODO: implement history navigation
                    continue;
                } else if (c3 == 'B') {
                    // DOWN arrow - TODO: implement history navigation
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
                }
            }
            continue;
        }
        
        // Handle special characters
        if (c == 4) {  // Ctrl+D (EOF)
            if (length == 0) {
                return -1;  // EOF on empty line
            }
            continue;
        } else if (c == 3) {  // Ctrl+C
            write_stdout("^C\n\r", 4);
            buffer[0] = '\0';
            return 0;
        } else if (c == '\r' || c == '\n') {  // Enter
            write_stdout("\n\r", 2);
            buffer[length] = '\0';
            return length;
        } else if (c == 127 || c == 8) {  // Backspace or DEL
            handle_backspace(buffer, &cursor, &length);
        } else if (c == '\t') {  // Tab
            // TODO: implement tab completion
            continue;
        } else if (c >= 32 && c < 127) {  // Printable character
            insert_char(buffer, &cursor, &length, c, buffer_size);
        }
        // Ignore other control characters
    }
    
    return length;
}
