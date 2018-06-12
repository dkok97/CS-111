# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

CC = gcc
CFLAGS = -Wall -Wextra -lm -lmraa -Wno-unused -g

default: build

build:
	$(CC) $(CFLAGS) lab4c_tcp.c -o lab4c_tcp
	$(CC) $(CFLAGS) -lcrypto -lssl lab4c_tls.c -o lab4c_tls

clean:
	rm -rf lab4c_tcp lab4c_tls *.tar.gz *.txt test* log* *.o

dist: default
	tar -czf lab4c-204818138.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
