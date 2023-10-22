# smallsh - A Simple Shell Implementation

## Description

`smallsh` is a custom, minimalistic shell designed to demonstrate the fundamental concepts of operating systems, processes, and shell programming. While it doesn't aim to replace more feature-rich shells like `bash` or `zsh`, `smallsh` serves as an excellent academic tool to showcase an understanding of system calls, process management, and I/O redirection in a Unix-based environment.

## Features

1. **Foreground & Background Processes**: `smallsh` supports running processes both in the foreground and background.
2. **Signal Handling**: Custom signal handling for `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z).
3. **Input & Output Redirection**: Users can redirect the standard input and output of their commands to and from files.
4. **Command Parsing**: Efficient command parsing to handle arguments, input and output redirection, and background execution.
5. **Memory Management**: Emphasis on preventing memory leaks and ensuring optimal memory usage.

## Getting Started

### Prerequisites

- A Unix-based system (Linux/MacOS)
- GCC for compilation

### Installation

```bash
git clone https://github.com/[Your-Github-Username]/smallsh.git
cd smallsh
gcc -o smallsh smallsh.c
```

### Usage

Simply run:

```bash
./smallsh
```

You will be presented with a custom command prompt where you can start entering commands.

## Limitations

- `smallsh` does not support scripting or advanced shell features such as job control or aliases.
- Only a subset of built-in commands is implemented.

## Testing

You can set a temporary modified `PS1` variable for testing:

```bash
PS1="$ " ./smallsh
```

## Contact

Haley Krueger - h.elaine.krueger@gmail.com

Project Link: https://github.com/haleyekrueger/smallsh
