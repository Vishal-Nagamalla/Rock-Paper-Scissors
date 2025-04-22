CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

all: rpsd test

rpsd: rpsd.o rps_utils.o
	$(CC) $(CFLAGS) -o rpsd rpsd.o rps_utils.o

rpsd.o: rpsd.c rps_utils.h
	$(CC) $(CFLAGS) -c rpsd.c

rps_utils.o: rps_utils.c rps_utils.h
	$(CC) $(CFLAGS) -c rps_utils.c

test: test.o
	$(CC) $(CFLAGS) -o test test.o

test.o: test.c
	$(CC) $(CFLAGS) -c test.c -o test.o

clean:
	rm -f *.o rpsd test test.o