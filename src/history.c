#include "../include/common.h"
#include "../include/history.h"

typedef struct {
    char *command;
} HistoryEntry;

static HistoryEntry history[MAX_HISTORY];
static int history_count = 0;

/**
 * Get home directory path
 */
static const char *get_home_dir(void) {
    const char *home = getenv("HOME");
    if (home == NULL) {
        home = ".";  // Fallback to current directory
    }
    return home;
}

/**
 * Get full path to .kord_history file
 */
static void get_history_path(char *buffer, size_t size) {
    const char *home = get_home_dir();
    snprintf(buffer, size, "%s/.kord_history", home);
}

void init_history(void) {
    // Initialize history array
    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i].command = NULL;
    }
    history_count = 0;
    
    // Load history from .kord_history file
    char history_path[1024];
    get_history_path(history_path, sizeof(history_path));
    
    FILE *file = fopen(history_path, "r");
    if (file == NULL) {
        // File doesn't exist, that's okay
        return;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file) && history_count < MAX_HISTORY) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines
        if (line[0] == '\0') {
            continue;
        }
        
        // Add to history
        history[history_count].command = strdup(line);
        if (history[history_count].command != NULL) {
            history_count++;
        }
    }
    
    fclose(file);
}

void cleanup_history(void) {
    // Save history to file
    char history_path[1024];
    get_history_path(history_path, sizeof(history_path));
    
    FILE *file = fopen(history_path, "w");
    if (file != NULL) {
        for (int i = 0; i < history_count; i++) {
            if (history[i].command != NULL) {
                fprintf(file, "%s\n", history[i].command);
            }
        }
        fclose(file);
    }
    
    // Free memory
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (history[i].command != NULL) {
            free(history[i].command);
            history[i].command = NULL;
        }
    }
    history_count = 0;
}

void add_history(const char *command) {
    if (command == NULL || command[0] == '\0') {
        return;
    }
    
    // Don't add duplicate consecutive entries
    if (history_count > 0 && history[history_count - 1].command != NULL) {
        if (strcmp(history[history_count - 1].command, command) == 0) {
            return;
        }
    }
    
    // If history is full, remove oldest entry
    if (history_count >= MAX_HISTORY) {
        if (history[0].command != NULL) {
            free(history[0].command);
        }
        
        // Shift all entries down
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i].command = history[i + 1].command;
        }
        history[MAX_HISTORY - 1].command = NULL;
        history_count--;
    }
    
    // Add new entry
    history[history_count].command = strdup(command);
    if (history[history_count].command != NULL) {
        history_count++;
    }
}

const char *get_history(int index) {
    if (index < 0 || index >= history_count) {
        return NULL;
    }
    
    return history[index].command;
}

int get_history_count(void) {
    return history_count;
}

void move_history_to_latest(int index) {
    if (index < 0 || index >= history_count) {
        return;
    }
    
    // Save the command at the index
    char *command = history[index].command;
    
    // Shift all entries after index down by one
    for (int i = index; i < history_count - 1; i++) {
        history[i].command = history[i + 1].command;
    }
    
    // Place the saved command at the end
    history[history_count - 1].command = command;
}
