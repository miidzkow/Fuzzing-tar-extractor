CC = gcc
CFLAGS = -Wall -Werror -g

fuzzer: fuzzer.o
	$(CC) $(CFLAGS) -o fuzzer fuzzer.o

fuzzer.o: fuzzer.c
	$(CC) $(CFLAGS) -c fuzzer.c

clean:
	rm -f fuzzer fuzzer.o
