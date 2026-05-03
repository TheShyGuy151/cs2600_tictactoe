CC=gcc
CFLAGS=-Wall -Wextra
LIBS=-lmosquitto

all: tictactoe_server

tictactoe_server: tictactoe_server.c
	$(CC) $(CFLAGS) tictactoe_server.c -o tictactoe_server $(LIBS)

clean:
	rm -f tictactoe_server
