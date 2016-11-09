all : task1_dining_philosopher.o task1_training_monkeys.o task2_syscall_invoker.o task3_semaphoretest.o
	gcc -o task1_dining_philosopher task1_dining_philosopher.o -pthread
	gcc -o task1_training_monkeys task1_training_monkeys.o -pthread
	gcc -o task2_syscall_invoker task2_syscall_invoker.o
	gcc -o task3_semaphoretest task3_semaphoretest.o -pthread

task1_dining_philosopher.o : task1_dining_philosopher.c
	gcc -c task1_dining_philosopher.c

task1_training_monkeys.o : task1_training_monkeys.c
	gcc -c task1_training_monkeys.c
	
task2_syscall_invoker.o : task2_syscall_invoker.c
	gcc -c task2_syscall_invoker.c

task3_semaphoretest.o : task3_semaphoretest.c
	gcc -c task3_semaphoretest.c

clean : 
	rm task1_dining_philosopher.o task1_dining_philosopher task1_training_monkeys.o task1_training_monkeys task3_semaphoretest task3_semaphoretest.o task2_syscall_invoker.o task2_syscall_invoker

