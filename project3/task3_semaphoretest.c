#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "task3_mysemaphore.h"

#define TARGET_SEMAPHORE 4
#define MAX_THREAD	8

int test = 0;

int priority[8] = {90,80,70,60,50,30,20,0};

void lockfunction(int priority)
{
	int v = test;
	v++;
	test = v;
}

void *threadfunction(void *args)
{
	int i;
	
	for(i=0;i<1000000;i++)
	{
		mysema_down(TARGET_SEMAPHORE);
		lockfunction(0);
		mysema_up(TARGET_SEMAPHORE);
	}
}

void *priorityTestThreadFunction(void *args)
{
	int pri = *(int*)args;
	
	printf("My priority is %d\n",pri);
	
	mysema_down_userprio(TARGET_SEMAPHORE,pri);
	
	printf("After up, my priority is %d\n",pri);
}

int main()
{
	int i,res;
	pthread_t thread[MAX_THREAD];
	
	res = mysema_init(TARGET_SEMAPHORE,0,FIFO);
	
	printf("mysema_init result : %d\n",res);
	
	for(i=0;i<MAX_THREAD;i++)
	{
		//pthread_create(&thread[i],NULL,threadfunction,NULL);
		pthread_create(&thread[i],NULL,priorityTestThreadFunction,(void*)(&priority[i]));
	}
	
	sleep(3);
	
	for(i=0;i<MAX_THREAD;i++)
	{
		mysema_up(TARGET_SEMAPHORE);
		sleep(1);
	}
	
	for(i=0;i<MAX_THREAD;i++)
	{
		pthread_join(thread[i],NULL);
	}

	
	res = mysema_release(TARGET_SEMAPHORE);
	
	printf("mysema_release : %d\n",res);
	
	printf("Final value : %d\n",test);
	return 0;
}
