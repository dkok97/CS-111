# NAME: Dinkar Khattar
# EMAIL: dinkarkhattar@ucla.edu
# ID: 204818138

CC = gcc
CFLAGS = -g -Wall -Wextra -pthread -lprofiler

default: build

build:
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c

clean:
		rm -rf lab2_list *.tar.gz

dist: clean build graphs profile
		tar -czf lab2b-204818138.tar.gz lab2_list.c SortedList.h SortedList.c *.csv *.png *.gp Makefile README *.out

graphs: tests
		gnuplot lab2_list.gp

profile: clean default
		-rm -f ./raw.gperf
		LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
		pprof --text ./lab2_list ./raw.gperf > profile.out
		pprof --list=lock ./lab2_list ./raw.gperf >> profile.out
		rm -f ./raw.gperf

tests: clean build
	./lab2_list --threads=1  --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=2  --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=4  --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=8  --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=16 --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=24 --iterations=1000 --sync=m >> lab2b_list.csv ;
	./lab2_list --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=16 --iterations=1000 --sync=s >> lab2b_list.csv ;
	./lab2_list --threads=24 --iterations=1000 --sync=s >> lab2b_list.csv ;
	-./lab2_list --iterations=1 --threads=1 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=2 --threads=1 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=4 --threads=1 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=8 --threads=1 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=16 --threads=1 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=1 --threads=4 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=2 --threads=4 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=4 --threads=4 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=8 --threads=4 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=16 --threads=4 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=1 --threads=8 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=2 --threads=8 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=4 --threads=8 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=8 --threads=8 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=16 --threads=8 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=1 --threads=12 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=2 --threads=12 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=4 --threads=12 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=8 --threads=12 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=16 --threads=12 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=1 --threads=16 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=2 --threads=16 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=4 --threads=16 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=8 --threads=16 --yield=id --lists=4 >> lab2b_list.csv ;
	-./lab2_list --iterations=16 --threads=16 --yield=id --lists=4 >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=1 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=1 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=1 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=1 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=4 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=4 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=4 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=4 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=8 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=8 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=8 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=8 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=12 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=12 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=12 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=12 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=16 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=16 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=16 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=16 --yield=id --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=1 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=1 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=1 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=1 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=4 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=4 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=4 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=4 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=8 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=8 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=8 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=8 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=12 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=12 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=12 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=12 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=10 --threads=16 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=20 --threads=16 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=40 --threads=16 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=80 --threads=16 --yield=id --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=2 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=2 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=2 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=2 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=1 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=4 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=8 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=16 --sync=s >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=1 --lists=16 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=2 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=16 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=4 --lists=16 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=8 --lists=16 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=12 --lists=16 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=1 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=4 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=8 --sync=m >> lab2b_list.csv ;
	./lab2_list --iterations=1000 --threads=16 --lists=16 --sync=m >> lab2b_list.csv ;
