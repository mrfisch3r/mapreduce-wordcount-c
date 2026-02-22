CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -O2

SRC=src/main.c src/pairio.c src/tokenize.c src/walk.c src/wordmap.c
OBJ=$(SRC:.c=.o)

all: wordcount

wordcount: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -f wordcount $(OBJ)

.PHONY: all clean
