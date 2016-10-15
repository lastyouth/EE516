all : pro_con.o read_write.o
	gcc -o pro_con pro_con.o -pthread
	gcc -o read_write read_write.o -pthread

pro_con.o : pro_con.c
	gcc -c pro_con.c

read_write.o : read_write.c
	gcc -c read_write.c

clean : 
	rm pro_con.o pro_con read_write read_write.o

