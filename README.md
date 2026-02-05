# Aggie Shell (ash)

A lightweight Unix-like shell written in C++ that supports interactive and batch execution, a few built-in commands, PATH-based program launching, and basic output redirection.

## Features

### Modes
- **Interactive mode**: runs as a REPL with a prompt.
- **Batch mode**: executes commands line-by-line from a file:
  ```bash
  ./ash commands.txt
