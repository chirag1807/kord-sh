#ifndef CONFIG_H
#define CONFIG_H

/* Shell version */
#define SHELL_VERSION "1.0.0"

/* History configuration */
#define MAX_HISTORY 50

/* Alias configuration */
#define MAX_ALIASES 100
#define MAX_ALIAS_NAME 64
#define MAX_ALIAS_VALUE 1024

/* Variable configuration */
#define MAX_VARIABLES 256
#define MAX_VAR_NAME 128
#define MAX_VAR_VALUE 1024

/* Parser configuration */
#define MAX_COMMANDS 64
#define MAX_ARGS 64

/* Executor configuration */
#define PIPE_READ 0
#define PIPE_WRITE 1

#endif /* CONFIG_H */

