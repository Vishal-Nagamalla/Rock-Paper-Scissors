CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

all: rpsd test

rpsd: rpsd.o rps_game.o
	$(CC) $(CFLAGS) -o rpsd rpsd.o rps_game.o

rpsd.o: rpsd.c rps_game.h
	$(CC) $(CFLAGS) -c rpsd.c

rps_game.o: rps_game.c rps_game.h
	$(CC) $(CFLAGS) -c rps_game.c

test: test.o
	$(CC) $(CFLAGS) -o test test.o

test.o: test.c
	$(CC) $(CFLAGS) -c test.c

clean:
	rm -f *.o rpsd test