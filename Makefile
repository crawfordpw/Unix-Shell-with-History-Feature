# To run, enter 
# make all

all: osh

osh: UnixShell.o
	gcc -pthread -o osh UnixShell.o

UnixShell.o: UnixShell.c
	gcc -pthread -c UnixShell.c
