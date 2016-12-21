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
#define NUMLOOP 100 // max loop count

// WARNING : THIS CODE SHOULD BE COMPILED BY THIS COMMAND.
// gcc -o reader_writer_skel reader_writer_skel.c -lpthread -lrt

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
		// get random number (0 ~ 9)
		buf[0] = (char)getNanoTimeRandomNumber(10);
		// pass to the driver.
		ret = write(targetFd,buf,4);
		
		if(ret == -1)
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
		// write random number to buf[0]
		buf[0] = (char)getNanoTimeRandomNumber(10);
		
		// attempt to read
		ret = read(targetFd,buf,4);
		
		if(ret == -1)
		{
			// there is no node which value is buf[0]
			printf("READER(%d) : Attempt to read %d, but failed\n",id,(int)buf[0]);
		}
		else
		{
			// Target node is acquired from driver. Value is saved in buf[1]
			printf("READER(%d) : Attempt to read %d and succeeded\n",id, (int)buf[1]);
		}
	}
}

int main(void)
{
	int i;
	pthread_t read_thread[NUMREAD]; // readers
	pthread_t write_thread[NUMWRITE]; // writers
	int *id;
	
	targetFd = open(DEVICE_NAME,O_RDWR); // open device driver
	
	if(targetFd == -1)
	{
		printf("Cannot find targetmodule : %s\n",DEVICE_NAME);
		return -1;
	}
	
	if (NUMREAD > NUMWRITE)
		id = (int *)malloc(sizeof(int)*NUMREAD);
	else
		id = (int *)malloc(sizeof(int)*NUMWRITE);

	// create reader threads
	for(i=0;i<NUMREAD;i++)
	{
		id[i] = i+1;
		pthread_create(&read_thread[i], NULL, read_func,(void*)(&id[i]));	
	}
	// create writer threads
	for(i=0;i<NUMWRITE;i++)
	{
		id[i] = i+1;
		pthread_create(&write_thread[i], NULL, write_func,(void*)(&id[i]));	
	}
	
	// wait for threads
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


