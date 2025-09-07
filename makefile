CC = gcc
CFLAGS = -Wall -g


httpser: main.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm httpser
	
