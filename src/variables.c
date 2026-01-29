#include "../include/common.h"
#include "../include/variables.h"

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
    int is_set;  // 1 if this slot is in use, 0 otherwise
} Variable;

static Variable shell_variables[MAX_VARIABLES];
static int variables_initialized = 0;

void init_variables(void) {
    if (variables_initialized) {
        return;
    }
    
    for (int i = 0; i < MAX_VARIABLES; i++) {
        shell_variables[i].is_set = 0;
        shell_variables[i].name[0] = '\0';
        shell_variables[i].value[0] = '\0';
    }
    
    variables_initialized = 1;
}

void cleanup_variables(void) {
    // Nothing to clean up for static arrays
    variables_initialized = 0;
}

int set_variable(const char *name, const char *value) {
    if (name == NULL || value == NULL) {
        return -1;
    }
    
    if (!variables_initialized) {
        init_variables();
    }
    
    // Check if variable exists in environment (was previously exported)
    // If so, update it there instead of creating a shell variable
    if (getenv(name) != NULL) {
        if (setenv(name, value, 1) != 0) {
            perror("setenv");
            return -1;
        }
        return 0;
    }
    
    // Check if variable already exists in shell variables, update it
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (shell_variables[i].is_set && strcmp(shell_variables[i].name, name) == 0) {
            strncpy(shell_variables[i].value, value, MAX_VAR_VALUE - 1);
            shell_variables[i].value[MAX_VAR_VALUE - 1] = '\0';
            return 0;
        }
    }
    
    // Find empty slot to add new variable
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (!shell_variables[i].is_set) {
            strncpy(shell_variables[i].name, name, MAX_VAR_NAME - 1);
            shell_variables[i].name[MAX_VAR_NAME - 1] = '\0';
            
            strncpy(shell_variables[i].value, value, MAX_VAR_VALUE - 1);
            shell_variables[i].value[MAX_VAR_VALUE - 1] = '\0';
            
            shell_variables[i].is_set = 1;
            return 0;
        }
    }
    
    fprintf(stderr, "Error: Maximum number of variables reached\n\r");
    return -1;
}

const char *get_variable(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    if (!variables_initialized) {
        init_variables();
    }
    
    // First check shell variables
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (shell_variables[i].is_set && strcmp(shell_variables[i].name, name) == 0) {
            return shell_variables[i].value;
        }
    }
    
    // Then check environment variables
    return getenv(name);
}

int export_variable(const char *name, const char *value) {
    if (name == NULL) {
        return -1;
    }
    
    if (!variables_initialized) {
        init_variables();
    }
    
    // If value is provided, use it
    // Otherwise, look up the value from shell variables
    const char *export_value = value;
    
    if (export_value == NULL) {
        // Try to find it in shell variables
        for (int i = 0; i < MAX_VARIABLES; i++) {
            if (shell_variables[i].is_set && strcmp(shell_variables[i].name, name) == 0) {
                export_value = shell_variables[i].value;
                break;
            }
        }
        
        // If still not found, can't export
        if (export_value == NULL) {
            fprintf(stderr, "Error: Variable '%s' not set\n\r", name);
            return -1;
        }
    }
    
    // Set in environment
    if (setenv(name, export_value, 1) != 0) {
        perror("setenv");
        return -1;
    }
    
    // Remove from shell variables if it exists there (now it's an env var)
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (shell_variables[i].is_set && strcmp(shell_variables[i].name, name) == 0) {
            shell_variables[i].is_set = 0;
            break;
        }
    }
    
    return 0;
}

int unset_variable(const char *name) {
    if (name == NULL) {
        return -1;
    }
    
    if (!variables_initialized) {
        init_variables();
    }
    
    int found = 0;
    
    // Remove from shell variables
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (shell_variables[i].is_set && strcmp(shell_variables[i].name, name) == 0) {
            shell_variables[i].is_set = 0;
            found = 1;
            break;
        }
    }
    
    // Remove from environment
    if (unsetenv(name) == 0) {
        found = 1;
    }
    
    return found ? 0 : -1;
}

void print_variables(void) {
    if (!variables_initialized) {
        init_variables();
    }
    
    printf("Shell variables:\n\r");
    int count = 0;
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (shell_variables[i].is_set) {
            printf("  %s=%s\n\r", shell_variables[i].name, shell_variables[i].value);
            count++;
        }
    }
    
    if (count == 0) {
        printf("  (none)\n\r");
    }
}

int is_variable_assignment(char **command) {
    if (command == NULL || command[0] == NULL) {
        return 0;
    }
    
    // Only treat as assignment if it's a single argument and contains '='
    if (command[1] == NULL && strchr(command[0], '=') != NULL) {
        return 1;
    }
    
    return 0;
}

int execute_variable_assignment(char **command) {
    if (command == NULL || command[0] == NULL) {
        return 1;
    }
    
    char *equals = strchr(command[0], '=');
    if (equals == NULL) {
        return 1;  // Not a valid assignment
    }
    
    // Split at '='
    *equals = '\0';
    const char *name = command[0];
    const char *value = equals + 1;
    
    // Validate variable name (must start with letter or underscore, contain only alnum or underscore)
    if (!name[0] || (!isalpha((unsigned char)name[0]) && name[0] != '_')) {
        *equals = '=';  // Restore
        return 1;
    }
    
    // Check all characters in name
    for (const char *p = name; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') {
            *equals = '=';  // Restore
            return 1;
        }
    }
    
    // Valid variable name, set it
    int result = set_variable(name, value);
    *equals = '=';  // Restore the original string
    
    return (result == 0) ? 0 : 1;
}
