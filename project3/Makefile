all : task1_dining_philosopher.o task1_training_monkeys.o
	gcc -o task1_dining_philosopher task1_dining_philosopher.o -pthread
	gcc -o task1_training_monkeys task1_training_monkeys.o -pthread

task1_dining_philosopher.o : task1_dining_philosopher.c
	gcc -c task1_dining_philosopher.c

task1_training_monkeys.o : task1_training_monkeys.c
	gcc -c task1_training_monkeys.c


clean : 
	rm task1_dining_philosopher.o task1_dining_philosopher task1_training_monkeys.o task1_training_monkeys

