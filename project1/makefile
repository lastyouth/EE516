all : fs_bench.o
	gcc -o bench fs_bench.o

fs_bench.o : fs_bench.c
	gcc -c fs_bench.c

clean : 
	rm fs_bench.o bench
