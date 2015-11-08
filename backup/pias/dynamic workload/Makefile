CC = gcc
CFLAGS = -Wall -pthread
TARGETS = server client

all: $(TARGETS)

server:
	$(CC) $(CFLAGS) server.c -o server.o

client:
	$(CC) $(CFLAGS) client.c -o client.o
	
clean:
	rm client.o server.o
