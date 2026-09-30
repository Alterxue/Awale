CC = gcc
CFLAGS = -Wall -g

all: server client

server: server.c session.c network.c
	$(CC) $(CFLAGS) -o server server.c session.c network.c

client: client.c network.c
	$(CC) $(CFLAGS) -o client client.c network.c

clean:
	rm -f server client