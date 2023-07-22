CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = -lssl -lcrypto

all: server client

server: echo_server.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

client: echo_client.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f server client
