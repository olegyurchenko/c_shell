# c_shell - Lightweight shell for embedded projects

Somehow I needed to embed a nice wrapper in a hardware project for administrative purposes like setting date, setting up network, etc.

Main library modules - 

- c_tty.c - Character User Interface. Provides command line editing, saving command history, etc.

- c_sh.c - shell, provides command inlining and execution of primitive shell-like scripts.

## c_tty -  Character User Interface

c_tty supports basic line editing, understanding the keys:
- backspace to delete characters,
- Del to delete characters,
- left and right arrow to move the insertion point,
-  Ctrl-C to ditch the entire line,
- Up and Down arrow for select line from history buffer.<br>
- Tab  to tab Completion

## c_sh - shell for embedding your commands

- Built-in commands: "exit", "quit", "break", "continue", "if", "then", "fi", "else", "elif", "true", "false", "while", "until", "for", "do", "done", "echo", "test", "[", "[[", "set", "read", "tee", "seq", "let"

- FIFO channel support:  cmd1 | cmd2 | cmd3 (with user defined external handler)

- Redirect STDIN/STDOUT/STDERR support: cmd1 > file 2> file, cmd1 2>file >&2  cmd < file (with user defined external handler)

- Variable substitutions: $var ${var}

- Command substitutions: $(cmd)  \`cmd\`

- Arithmetic substitutions: $(( expression ))

  

  



