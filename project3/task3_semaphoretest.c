#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "task3_mysemaphore.h"

#define TARGET_SEMAPHORE 2
#define MAX_THREAD	8
#define MAX_LOOP	1000000

/* if define ATOMIC_OPERATION and rebuild, this program will perform about the atomic operation test.
	otherwise, it will perform the priority test
*/
//#define ATOMIC_OPERATION


int shared_variable = 0;

int priority[8] = {60,90,30,60,50,15,20,0};

/*
 * simple function, which performs
 * 	1. load value from shared one
 * 	2. add 1
 * 	3. store value to shared one*/
void addFunction()
{
	int v = shared_variable;
	v++;
	shared_variable = v;
	
}

/*
 * call addFunction with semaphore */
void *threadfunctionWithSemaphore(void *args)
{
	int i;
	
	for(i=0;i<MAX_LOOP;i++)
	{
		mysema_down(TARGET_SEMAPHORE);
		addFunction();
		mysema_up(TARGET_SEMAPHORE);
	}
}

/*
 * call addFunction without semaphore */
void *threadfunctionWithoutSemaphore(void *args)
{
	int i;
	
	for(i=0;i<MAX_LOOP;i++)
	{
		addFunction();
	}
}

/*
 * call mysema_down_userprio with designated priority from thread creating */
void *priorityTestThreadFunction(void *args)
{
	int pri = *(int*)args;
	
	printf("My priority is %d, I'm going to sleep\n",pri);
	
	mysema_down_userprio(TARGET_SEMAPHORE,pri);
	
	printf("After mysema_up, my priority is %d\n",pri);
}

int main()
{
	int i,k;
	pthread_t thread[MAX_THREAD];
	char *modename[3]={"FIFO","OS","USER"};
	
// if ATOMIC_OPERATION is defined
#ifdef ATOMIC_OPERATION
	/*
	 * perform atomic operation test
	 * 1. without semaphore
	 * 2. with semaphore*/
	mysema_init(TARGET_SEMAPHORE,1,FIFO);
	
	printf("%d threads, each thread performs one addition to shared variable for %d times WITHOUT semaphore\n",MAX_THREAD,MAX_LOOP);
	
	shared_variable = 0;
	
	for(i=0;i<MAX_THREAD;i++)
	{
		pthread_create(&thread[i],NULL,threadfunctionWithoutSemaphore,NULL);
	}
	
	for(i=0;i<MAX_THREAD;i++)
	{
		pthread_join(thread[i],NULL);
	}
	
	printf("Result : %d (expected : %d)\n",shared_variable,MAX_THREAD*MAX_LOOP);
	printf("--------------------------------------------------------------------------\n");
	
	printf("%d threads, each thread performs one addition to shared variable for %d times WITH semaphore\n",MAX_THREAD,MAX_LOOP);
	
	shared_variable = 0;
	
	for(i=0;i<MAX_THREAD;i++)
	{
		pthread_create(&thread[i],NULL,threadfunctionWithSemaphore,NULL);
	}
	
	for(i=0;i<MAX_THREAD;i++)
	{
		pthread_join(thread[i],NULL);
	}
	
	printf("Result : %d (expected : %d)\n",shared_variable,MAX_THREAD*MAX_LOOP);
	
	mysema_release(TARGET_SEMAPHORE);
#else
	/*
	 * perform priority test
	 * 1. FIFO mode
	 * 2. OS mode
	 * 3. USER mode*/
	for(k=FIFO;k<=USER;k++)
	{
		mysema_init(TARGET_SEMAPHORE,0,k);
		
		printf("%s mode semaphore, priority test\n",modename[k]);
		printf("Spawn %d threads...\n",MAX_THREAD);
		for(i=0;i<MAX_THREAD;i++)
		{
			pthread_create(&thread[i],NULL,priorityTestThreadFunction,(void*)(&priority[i]));
			usleep(500*1000);
		}
		
		printf("Spawning done! %d threads are sleeping...\n",MAX_THREAD);
		printf("Now, calling mysema_up function for %d times. once per second\n",MAX_THREAD);
		for(i=0;i<MAX_THREAD;i++)
		{
			mysema_up(TARGET_SEMAPHORE);
			sleep(1);
		}
		
		for(i=0;i<MAX_THREAD;i++)
		{
			pthread_join(thread[i],NULL);
		}
		
		mysema_release(TARGET_SEMAPHORE);
	}
#endif
	return 0;
}
