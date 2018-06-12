# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

.SILENT:

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused -g

default:
	$(CC) $(CFLAGS) lab1a.c -o lab1a

clean:
	rm -rf lab1a *.tar.gz *.o

dist:
	tar -czf lab1a-204818138.tar.gz lab1a.c Makefile README
