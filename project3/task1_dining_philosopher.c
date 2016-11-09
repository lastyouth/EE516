#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_PHILOSOPHER_NUM 16
#define THINKING 0
#define HUNGRY 1
#define EATING 2

// semaphores
sem_t mutex; // use as mutex
sem_t sem[MAX_PHILOSOPHER_NUM+1]; //semaphore for each philosopher

// state
int state[MAX_PHILOSOPHER_NUM];


int getLeft(int i)
{
	/*
	 * return left number of philosopher i*/
	if(i == 1)
	{
		return MAX_PHILOSOPHER_NUM;
	}
	return i-1;
}

int getRight(int i)
{
	/*
	 * return right number of philosopher i*/
	if(i == MAX_PHILOSOPHER_NUM)
	{
		return 1;
	}
	return i+1;
}

int pickNanotimeSeedRandomNumber()
{
	/*
	 * To enrich the variability, this function uses nanotime part for srand function
	 * 
	 * context switching scale is higher than nanotime scale,
	 * 
	 * this can make different random value for invoking from different threads*/
	int msScaleWaitTime = 0;
	struct timespec ts; // for nanotime
	
	clock_gettime(CLOCK_MONOTONIC ,&ts); // use nanotime scale for different seeding.
	
	srand((unsigned int)ts.tv_nsec);
	
	msScaleWaitTime = 1000+rand()%2499;
	
	msScaleWaitTime*=100;
	
	return msScaleWaitTime;
}

void think(int i)
{
	/*
	 * simulate think action by invoking sleep function
	 * 
	 * sleeping time is determined by nanoscale seed random function*/
	int waittime = pickNanotimeSeedRandomNumber();
	
	printf("PHILOSOPHER %d : think for %d us\n",i,waittime);
	
	usleep(waittime);
}

void eat(int i)
{
	/*
	 * simulate eat action by invoking sleep function
	 * 
	 * eating time is determined by nanoscale seed random function*/
	int waittime = pickNanotimeSeedRandomNumber();
	
	printf("PHILOSOPHER %d : eat for %d us\n",i,waittime);
	
	usleep(waittime);
}

void test(int i)
{
	/*
	 * check function
	 * 
	 * if philosopher i is hungry and left and right philosophers aren't eating*/
	if(state[i] == HUNGRY && state[getLeft(i)] != EATING && state[getRight(i)] != EATING)
	{
		// then acquire the forks.
		state[i] = EATING;
		sem_post(&sem[i]);
		printf("PHILOSOPHER %d : acquire the forks\n",i);
	}
}

void take_forks(int i)
{
	/*
	 *simulate take forks action
	 * 
	 * change philosopher i state to HUNGRY
	 * and try to acquire the forks
	 * 
	 * if succeed, this function end immediately otherwise, it goes to sleep*/
	sem_wait(&mutex);
	state[i] = HUNGRY;
	test(i);
	sem_post(&mutex);
	sem_wait(&sem[i]);
}

void put_forks(int i)
{
	/*
	 *simulate put forks action
	 * 
	 * change philosopher i state to THINKING
	 * and put down the acquired forks
	 * 
	 * notify current philosopher's neighbors that forks are available for them */
	sem_wait(&mutex);
	state[i] = THINKING;
	test(getLeft(i));
	test(getRight(i));
	sem_post(&mutex);
	printf("PHILOSOPHER %d : put down the forks\n",i);
}

void *philosopher(void *args)
{
	/*
	 * thread function for simulating philosopher*/
	int id = *(int*)args;
	
	while(1)
	{
		think(id);
		take_forks(id);
		eat(id);
		put_forks(id);
	}
}

int main()
{
	int i,res;
	int p[MAX_PHILOSOPHER_NUM]={0,};
	pthread_t philo_threads[MAX_PHILOSOPHER_NUM] = {0,};
	
	/*
	 * routine for semaphore initalization*/
	res=sem_init(&mutex, 0,1); // operate as mutex
	
	if(res !=0){
		perror("sem_init failed.\n");
		exit(1);
	}
	for(i=0;i<MAX_PHILOSOPHER_NUM;i++)
	{
		res=sem_init(&sem[i],0,0); // operate as mutex for each philosopher
		
		if(res!=0){
			perror("sem_init_failed.\n");
			exit(1);
		}
	}	
	// spawn threads for philosopher
	for(i=0;i<MAX_PHILOSOPHER_NUM;i++)
	{
		p[i] = i+1;
		pthread_create(&philo_threads[i], NULL, philosopher,(void*)(&p[i]));
	}
	// wait for philosopher's thread
	for(i=0; i<MAX_PHILOSOPHER_NUM; i++)
	{
		pthread_join(philo_threads[i], NULL);
	}

	/*
	 * semaphore destory routine*/
	sem_destroy(&mutex);
	
	for(i=0;i<MAX_PHILOSOPHER_NUM;i++)
	{
		sem_destroy(&sem[i]);
	}
	return 0;
}
