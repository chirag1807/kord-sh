#ifndef COMMON_H
#define COMMON_H

/* Standard C library headers - commonly used across the project */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Platform-independent headers */
#include <limits.h>
#include <time.h>

/* Unix/POSIX headers - only include on Unix-like systems */
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <dirent.h>
#include <pwd.h>
#endif

/* Project-wide configuration and constants */
#include "config.h"

/* Project-wide utilities */
#include "colors.h"

#endif /* COMMON_H */
