## TCSH - A small UNIX shell written in C.

__compile__: <br />
gcc tcsh.c -o tcsh <br />
./tcsh
---
### Some Commands:
- "bg cmd" to run a process in background, EX: bg sleep 5
- "processes" to view zombie threads
- "exit" command exits the shell
- "< >" redirection supported

### Limitations & TODO:
- Help menu for supported commands
- SIGCHLD and signals to make background processes table current
- Hotkey support
- History table for commands

### Example:
<img src="https://github.com/tmcarmichael/TCSH-Small-UNIX-Shell/blob/master/tcsh_ex.png" height="512" width="512">
