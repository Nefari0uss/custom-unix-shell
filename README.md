# Simple Unix Shell

## Author: [nefari0uss](https://www.github.com/nefari0uss)

![language c](https://img.shields.io/badge/language-C-orange.svg "Language C")
[![Open Source Love](https://badges.frapsoft.com/os/mit/mit.svg?v=102)](https://github.com/ellerbrock/open-source-badge/)
[![Build Status](https://travis-ci.org/Nefari0uss/custom-unix-shell.svg?branch=master)](https://travis-ci.org/Nefari0uss/custom-unix-shell) 

## Summary
Simple UNIX shell with history. This shell run's a process that reads the user input then creates a seperate child process to perform the command. 
The parent process will then wait for the child to finish execution before continuing unless explicitly told to run in the background. The child process
created using the `fork()` system call. The command is executed using the `exec()` from a system call.

## Instructions
* Run `$ make` to compile. 
* Run `$ ./shell`.
* `exit` or `quit` will end the program. 
* Running the shell a second time will allow it to read from the history file (if one exists).
* Enter commands to run.
* `h` will display the history.
* `r` can be used to invoke a command in the history. 
* If a command is repeated, only the latest call is kept in the history.

## Asciicast
[![asciicast](https://asciinema.org/a/37yxcsctn5azwugeq0md9012v.png)](https://asciinema.org/a/37yxcsctn5azwugeq0md9012v)
