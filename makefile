# makefile for custom unix shell
all: shell.c
	gcc shell.c -Wall -o shell

clean:
	$(RM) shell history.shell

