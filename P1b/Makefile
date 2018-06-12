# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

.SILENT:

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused -g -lz

default:
	$(CC) $(CFLAGS) lab1b-client.c -o lab1b-client
	$(CC) $(CFLAGS) lab1b-server.c -o lab1b-server

client:
	$(CC) $(CFLAGS) lab1b-client.c -o lab1b-client

clean:
	rm -rf lab1b-client lab1b-server *.tar.gz *.o

dist:
	tar -czf lab1b-204818138.tar.gz lab1b-client.c lab1b-server.c Makefile README
