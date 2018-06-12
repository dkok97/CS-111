# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

CC = gcc
CFLAGS = -Wall -Wextra -lm -lmraa -Wno-unused -g

build:
	$(CC) $(CFLAGS) lab4b.c -o lab4b

default: build

check:
	{ echo "SCALE=C"; sleep 2; echo "PERIOD=2"; sleep 3; echo "STOP"; sleep 1; echo "START"; sleep 3; echo "OFF"; } | ./lab4b --log="log.txt"
	@grep -oE '[0-9]+\.' log.txt > /dev/null || \
	(echo "FAIL: No temperature readings" && exit 1)
	@grep -oE 'SCALE' log.txt > /dev/null || \
	(echo "FAIL: No scale change recorded" && exit 1)
	@grep -oE 'STOP' log.txt > /dev/null || \
	(echo "FAIL: No stop recorded" && exit 1)
	@grep -oE 'START' log.txt > /dev/null || \
	@(echo "FAIL: No start recorded" && exit 1)
	@grep -oE 'OFF' log.txt > /dev/null || \
	(echo "FAIL: No off recorded" && exit 1)
	@grep -oE 'SHUTDOWN' log.txt > /dev/null || \
	(echo "FAIL: No shutdown recorded" && exit 1)
	@echo "ALL TESTS PASSED"

clean:
	rm -rf lab4b *.tar.gz *.txt test* log*

dist: default
	tar -czf lab4b-204818138.tar.gz lab4b.c Makefile README
