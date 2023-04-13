CC = gcc
CFLAGS = -Wall -Werror -g

fuzzer: src/fuzzer.o
	$(CC) $(CFLAGS) -o fuzzer src/fuzzer.o

fuzzer.o: src/fuzzer.c
	$(CC) $(CFLAGS) -c src/fuzzer.c

clean:
	rm -f fuzzer src/fuzzer.o
