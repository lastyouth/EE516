#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>


#define NUMREAD 10			// Number of Readers
#define NUMWRITE 10			// Number of Writers

#define DEVICE_NAME "/dev/JUSTICERAINSFROM"
#define NUMLOOP 100

int targetFd;

int getNanoTimeRandomNumber(int range)
{
	/*
	 * To enrich the variability, this function uses nanotime part for srand function
	 * 
	 * context switching scale is higher than nanotime scale,
	 * 
	 * this can make different random value for invoking from different threads*/
	int ret = 0;
	struct timespec ts; // for nanotime
	
	clock_gettime(CLOCK_MONOTONIC ,&ts); // use nanotime scale for different seeding.
	
	srand((unsigned int)ts.tv_nsec);
		
	ret = rand()%range;

	return ret;
}

/* Writer function */
void *write_func(void *arg) {
	int i,ret,id = *((int*)arg);
	char buf[4];

	
	for(i=0;i<NUMLOOP;i++)
	{
		buf[0] = (char)getNanoTimeRandomNumber(10);
		ret = write(targetFd,buf,4);
		
		if(ret == 0)
		{
			printf("WRITER(%d) : write failed\n",id);
		}
		else
		{
			printf("WRITER(%d) : write %d has succeeded\n",id, (int)buf[0]);
		}
	}
}

/* Reader function */
void *read_func(void *arg) {
	int i,ret,id = *((int*)arg);
	char buf[4];
	
	for(i=0;i<NUMLOOP;i++)
	{
		buf[0] = (char)getNanoTimeRandomNumber(10);
		ret = read(targetFd,buf,4);
		
		if(ret == 0)
		{
			printf("READER(%d) : Attempt to read %d, but failed\n",id,(int)buf[0]);
		}
		else
		{
			printf("READER(%d) : Attempt to read %d and succeeded\n",id, (int)buf[1]);
		}
	}
}

int main(void)
{
	int i;
	pthread_t read_thread[NUMREAD];
	pthread_t write_thread[NUMWRITE];
	int id[] = {1,2,3,4,5,6,7,8,9,10};
	
	targetFd = open(DEVICE_NAME,O_RDWR);
	
	if(targetFd == -1)
	{
		printf("Cannot find targetmodule : %s\n",DEVICE_NAME);
		return -1;
	}

	for(i=0;i<NUMREAD;i++)
	{
		pthread_create(&read_thread[i], NULL, read_func,(void*)(&id[i]));	
	}
	for(i=0;i<NUMWRITE;i++)
	{
		pthread_create(&write_thread[i], NULL, write_func,(void*)(&id[i]));	
	}
	
	for(i=0;i<NUMREAD;i++)
	{
		pthread_join(read_thread[i], NULL);
	}
	for(i=0;i<NUMWRITE;i++)
	{
		pthread_join(write_thread[i], NULL);
	}
	close(targetFd);
	return 0;
}

