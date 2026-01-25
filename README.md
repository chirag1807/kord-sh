# 🧶 Kord

> **A lightweight, feature-rich POSIX shell implemented in C**

Kord is a Unix shell that bridges user commands with kernel operations through system calls, providing efficient process management, I/O redirection, and a modern interactive terminal experience.

---

## 🌟 Features

### Core Shell Capabilities
- **Command Execution**: Execute external programs via `fork()` and `execvp()`
- **Pipeline Support**: Chain multiple commands with `|` operator
- **I/O Redirection**: Full support for `<`, `>`, and `>>` operators
- **Built-in Commands**: `cd`, `pwd`, `echo`, `exit`, `help`, and more
- **Signal Handling**: Proper `Ctrl+C` (SIGINT) management for shell and child processes

### Advanced Features
- **Shell Variables**: Define and expand variables with `$VAR` syntax
- **Environment Export**: `export` variables to child processes
- **Command Aliases**: Create shortcuts for frequently used commands
- **Persistent History**: Commands saved to `~/.kord_history` with navigation via arrow keys

### Interactive Terminal
- **Raw Mode Input**: Custom terminal handling with ANSI escape sequences
- **Line Editing**: 
  - Cursor navigation with arrow keys (`←`, `→`, `Home`, `End`)
  - Word-level navigation (`Ctrl+←`, `Ctrl+→`)
  - Character/word deletion (`Backspace`, `Delete`, `Ctrl+W`, `Ctrl+Backspace`)
- **Tab Completion**: Intelligent file/directory completion with case-insensitive matching
- **History Navigation**: Browse previous commands with `↑`/`↓` arrow keys
- **Multi-line Support**: Insert characters anywhere in the input line

---

## 🔧 Terminal Architecture

Kord implements a sophisticated terminal I/O system that directly interfaces with the TTY subsystem:

### TTY Subsystem & Raw Mode
The shell operates in **raw mode** by manipulating terminal attributes through `termios`:

```c
// Disable canonical mode, echo, and signal generation
tcgetattr(STDIN_FILENO, &original_termios);
raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
```

**Raw mode** allows Kord to:
- Read input byte-by-byte without waiting for newlines
- Handle ANSI escape sequences (arrow keys, function keys)
- Implement custom line editing with immediate visual feedback
- Control cursor position and text rendering directly

### PTY Master-Slave Architecture
While Kord primarily operates on a standard TTY, it's designed to work seamlessly in PTY (pseudo-terminal) environments:

- **PTY Master**: Owned by terminal emulators (e.g., xterm, gnome-terminal)
  - Handles rendering and user input events
  - Manages terminal window properties (size, colors)
  
- **PTY Slave**: Connected to shell processes like Kord
  - Receives input from master via kernel TTY driver
  - Sends output back through the TTY line discipline
  
- **TTY Line Discipline**: Kernel layer that processes terminal I/O
  - In cooked mode: handles backspace, `Ctrl+C`, line buffering
  - In raw mode (Kord's default): passes all input directly to the application

When a child process is spawned, Kord temporarily disables raw mode to ensure the child inherits standard terminal behavior, then re-enables it afterward for continued interactive control.

---

## 📁 Project Structure

```
kord-sh/
├── src/
│   ├── main.c          # Entry point and main loop
│   ├── raw_input.c     # Raw mode terminal I/O and line editing
│   ├── parser.c        # Command parsing and variable expansion
│   ├── executor.c      # Process execution, pipes, and I/O redirection
│   ├── builtins.c      # Built-in command implementations
│   ├── prompt.c        # Dynamic shell prompt rendering
│   ├── history.c       # Command history management
│   ├── variables.c     # Shell variable storage
│   └── aliases.c       # Alias management
├── include/            # Header files
├── Makefile            # Build configuration
└── LICENSE             # MIT License
```

---

## 🚀 Getting Started

### Prerequisites
- **GCC** or any C compiler
- **POSIX-compliant** system (Linux, macOS, BSD)
- **Make** build tool

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/kord-sh.git
cd kord-sh

# Build the shell
make

# Run Kord
make run
# or directly
./bin/main
```

### Usage

```bash
# Basic commands
$ pwd
$ ls -la
$ echo "Hello from Kord!"

# Pipelines
$ cat file.txt | grep "pattern" | sort

# I/O Redirection
$ echo "Log entry" >> log.txt
$ cat < input.txt > output.txt

# Variables
$ set USERNAME=kord
$ echo $USERNAME
$ export PATH=$PATH:/custom/bin

# Aliases
$ alias ll="ls -lah"
$ ll

# History
$ history
$ <press ↑ to navigate history>

# Tab completion
$ cd /usr/loc<TAB>  # Completes to /usr/local/
```

---

## 🎯 Built-in Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `cd` | Change directory | `cd [path]` |
| `pwd` | Print working directory | `pwd` |
| `echo` | Print arguments | `echo [args...]` |
| `exit` | Exit shell | `exit [code]` |
| `set` | Set shell variable | `set VAR=value` |
| `export` | Export environment variable | `export VAR=value` |
| `unset` | Remove variable | `unset VAR` |
| `alias` | Define command alias | `alias name='command'` |
| `unalias` | Remove alias | `unalias name` |
| `history` | Show command history | `history` |
| `help` | Display help information | `help [command]` |

---

## 🔍 Implementation Highlights

### Process Management
- **`fork()`**: Creates child processes by duplicating the parent's memory space
- **`exec()` family**: Replaces child process image with new program via `execvp()`
- Parent ignores `SIGINT` while child processes run
- Proper cleanup with `waitpid()` and signal restoration

### Pipeline Implementation
```c
// Create pipes between commands
pipe(fd_pipe);
// Child 1: stdout → pipe write end
// Child 2: pipe read end → stdin
```

### Variable Expansion
- Parses `$VAR` syntax during command parsing
- Supports both shell-local and environment variables
- Quote handling preserves variable expansion: `"$VAR"` expands, `'$VAR'` literal

### Terminal Control
- ANSI escape sequences for cursor movement: `\033[C`, `\033[D`
- Immediate visual feedback on edits via `dup2()` and terminal redraw
- Context-aware tab completion with directory traversal

---

## 📝 Configuration Files

- **`~/.kord_history`**: Stores last 50 commands, automatically loaded on shell startup and saved on exit. Supports up/down arrow navigation through command history.

- **`~/.kordrc`**: Alias configuration file loaded at startup. Define persistent aliases that survive shell restarts.
  
  **Note**: Aliases defined with the `alias` command during runtime are not persisted to `.kordrc`. To make aliases permanent, manually edit this file.

---

## 🛠️ Development

### Cleaning Build Artifacts
```bash
make clean
```

### Debugging
To debug with `gdb`:
```bash
gdb ./bin/main
```

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 🐛 Known Limitations

- No command substitution (`` `command` `` or `$(command)`)
- No globbing/wildcards (`*.txt`)
- Tab completion limited to files/directories (no command/variable completion)

Future enhancements welcome via pull requests!

---

**Built with ❤️ in C • Kord - Your friendly neighborhood shell 🧶**
