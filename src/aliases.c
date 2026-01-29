#include "../include/common.h"
#include "../include/aliases.h"

typedef struct {
    char name[MAX_ALIAS_NAME];
    char value[MAX_ALIAS_VALUE];
    int active;  // 1 if slot is used, 0 if free
} Alias;

static Alias aliases[MAX_ALIASES];
static int alias_count = 0;

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
 * Get full path to .kordrc file
 */
static void get_kordrc_path(char *buffer, size_t size) {
    const char *home = get_home_dir();
    snprintf(buffer, size, "%s/.kordrc", home);
}

void init_aliases(void) {
    // Initialize alias array
    for (int i = 0; i < MAX_ALIASES; i++) {
        aliases[i].active = 0;
    }
    alias_count = 0;
    
    // Load aliases from .kordrc file
    char kordrc_path[1024];
    get_kordrc_path(kordrc_path, sizeof(kordrc_path));
    
    FILE *file = fopen(kordrc_path, "r");
    if (file == NULL) {
        // File doesn't exist, that's okay
        return;
    }
    
    char line[MAX_ALIAS_VALUE + MAX_ALIAS_NAME + 10];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Parse alias command: alias name='value' or alias name="value"
        char *alias_start = strstr(line, "alias ");
        if (alias_start == line) {
            char *equals = strchr(line, '=');
            if (equals == NULL) {
                continue;
            }
            
            // Extract name (between "alias " and '=')
            char name[MAX_ALIAS_NAME];
            int name_len = equals - (line + 6);  // 6 = strlen("alias ")
            if (name_len <= 0 || name_len >= MAX_ALIAS_NAME) {
                continue;
            }
            
            strncpy(name, line + 6, name_len);
            name[name_len] = '\0';
            
            // Trim whitespace from name
            char *name_ptr = name;
            while (*name_ptr == ' ' || *name_ptr == '\t') name_ptr++;
            char *name_end = name_ptr + strlen(name_ptr) - 1;
            while (name_end > name_ptr && (*name_end == ' ' || *name_end == '\t')) {
                *name_end = '\0';
                name_end--;
            }
            
            // Extract value (after '=')
            char *value = equals + 1;
            while (*value == ' ' || *value == '\t') value++;
            
            // Remove quotes if present
            if ((*value == '\'' || *value == '"') && strlen(value) >= 2) {
                char quote = *value;
                value++;
                char *end = strrchr(value, quote);
                if (end) {
                    *end = '\0';
                }
            }
            
            // Add alias to memory
            set_alias(name_ptr, value);
        }
    }
    
    fclose(file);
}

void cleanup_aliases(void) {
    // No need to save to file, aliases are only stored in memory
    // Just cleanup the memory
    for (int i = 0; i < MAX_ALIASES; i++) {
        aliases[i].active = 0;
    }
    alias_count = 0;
}

int set_alias(const char *name, const char *value) {
    if (name == NULL || value == NULL) {
        return -1;
    }
    
    if (strlen(name) >= MAX_ALIAS_NAME || strlen(value) >= MAX_ALIAS_VALUE) {
        return -1;
    }
    
    // Check if alias already exists
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].active && strcmp(aliases[i].name, name) == 0) {
            // Update existing alias
            strncpy(aliases[i].value, value, MAX_ALIAS_VALUE - 1);
            aliases[i].value[MAX_ALIAS_VALUE - 1] = '\0';
            return 0;
        }
    }
    
    // Find free slot
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (!aliases[i].active) {
            aliases[i].active = 1;
            strncpy(aliases[i].name, name, MAX_ALIAS_NAME - 1);
            aliases[i].name[MAX_ALIAS_NAME - 1] = '\0';
            strncpy(aliases[i].value, value, MAX_ALIAS_VALUE - 1);
            aliases[i].value[MAX_ALIAS_VALUE - 1] = '\0';
            alias_count++;
            return 0;
        }
    }
    
    // No free slots
    return -1;
}

const char *get_alias(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].active && strcmp(aliases[i].name, name) == 0) {
            return aliases[i].value;
        }
    }
    
    return NULL;
}

int unset_alias(const char *name) {
    if (name == NULL) {
        return -1;
    }
    
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].active && strcmp(aliases[i].name, name) == 0) {
            aliases[i].active = 0;
            alias_count--;
            return 0;
        }
    }
    
    return -1;
}

void print_aliases(void) {
    if (alias_count == 0) {
        printf("No aliases defined\n\r");
        return;
    }
    
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].active) {
            printf("alias %s='%s'\n\r", aliases[i].name, aliases[i].value);
        }
    }
}

char *expand_alias(const char *command) {
    if (command == NULL) {
        return NULL;
    }
    
    // Extract first word (command name)
    char cmd_name[MAX_ALIAS_NAME];
    const char *ptr = command;
    int i = 0;
    
    // Skip leading whitespace
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    // Extract command name
    while (*ptr && *ptr != ' ' && *ptr != '\t' && i < MAX_ALIAS_NAME - 1) {
        cmd_name[i++] = *ptr++;
    }
    cmd_name[i] = '\0';
    
    if (i == 0) {
        return NULL;  // Empty command
    }
    
    // Check if this is an alias
    const char *alias_value = get_alias(cmd_name);
    if (alias_value == NULL) {
        return NULL;  // Not an alias
    }
    
    // Build expanded command
    size_t alias_len = strlen(alias_value);
    size_t rest_len = strlen(ptr);
    size_t total_len = alias_len + rest_len + 1;
    
    char *expanded = malloc(total_len);
    if (expanded == NULL) {
        return NULL;
    }
    
    strcpy(expanded, alias_value);
    strcat(expanded, ptr);
    
    return expanded;
}
