# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

.SILENT:

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused -g

default:
	$(CC) $(CFLAGS) lab0.c -o lab0

check: clean default check-tests

clean:
	rm -rf lab0 *.o *.tar.gz *.txt

dist: default
	tar -czf lab0-204818138.tar.gz lab0.c Makefile backtrace.png breakpoint.png README

copyFromIntoOut:
	echo "abcded" > input.txt; \
	./lab0 --input input.txt --output output.txt &>/dev/null; \
	cmp input.txt output.txt
	if [[ $$? -eq 0 ]]; then \
	echo "PASS: Copied from input.txt to output.txt"; \
	else \
	echo "FAIL: input.txt does not match output.txt"; \
	fi; \
	rm -rf *.txt

inputErrorTest:
	./lab0 --input input.txt --output output.txt &>/dev/null; \
	if [[ $$? -eq 2 ]]; then \
	echo "PASS: Input file open error. Program exited as expected with exit code 2"; \
	else \
	echo "FAIL: Program did not exit with exit code 2 when encountered a read error"; \
	fi; \
	rm -rf *.txt

outputErrorTest:
	echo "abcded" > input.txt; \
	touch output.txt; \
	chmod u-w output.txt; \
	./lab0 --input input.txt --output output.txt &>/dev/null; \
	if [[ $$? -eq 3 ]]; then \
	echo "PASS: Ouput file write error. Program exited as expected with exit code 3"; \
	else \
	echo "FAIL: Program did not exit with exit code 3 when encountered a write error"; \
	fi; \
	rm -rf *.txt

incorrectUsageTest:
	echo "abcded" > input.txt; \
	./lab0 --input input.txt --x &>/dev/null; \
	if [[ $$? -eq 1 ]]; then \
	echo "PASS: Incorrect usage detected. Program exited as expected with exit code 1"; \
	else \
	echo "FAIL: Program did not exit with exit code 1 when invoked incorrectly"; \
	fi; \
	rm -rf *.txt

catchTest:
	./lab0 --s --c &>/dev/null; \
	if [[ $$? -eq 4 ]]; then \
	echo "PASS: Segmentation fault caught. Program exited as expected with exit code 4"; \
	else \
	echo "FAIL: Program did not exit with exit code 4 when a segmentation fault was "; \
	fi; \
	rm -rf *.txt

check-tests: copyFromIntoOut inputErrorTest outputErrorTest incorrectUsageTest catchTest
