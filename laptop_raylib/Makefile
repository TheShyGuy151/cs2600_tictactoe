CC=gcc
CFLAGS=-Wall -Wextra
LIBS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lmosquitto

all: raylib_tictactoe_client

raylib_tictactoe_client: raylib_tictactoe_client.c
	$(CC) $(CFLAGS) raylib_tictactoe_client.c -o raylib_tictactoe_client $(LIBS)

clean:
	rm -f raylib_tictactoe_client
