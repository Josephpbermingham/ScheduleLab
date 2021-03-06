
CC = gcc
CFLAGS = -g -Wall

default: procprog

procprog: main.o proc.o
	$(CC) $(CFLAGS) -o procprog main.o proc.o -lm

main.o: main.c types.h defs.h
	$(CC) $(CFLAGS) -c main.c

proc.o: proc.c types.h defs.h proc.h
	$(CC) $(CFLAGS) -c proc.c -lm

clean:
	rm -f procprog *.o

