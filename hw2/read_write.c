#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>

// prototype of reader and writer function, these are taking parameter arg as void pointer.
void *reader(void *arg);
void *writer(void *arg);

// semaphores all used as mutex
sem_t mutex; // for reader's count
sem_t db; // for db access


int reader_num; // represent current readers who are interested in db.

char db_file; // db file, size of 1 byte.

char read_db()
{
	// just return the current db value.
	return db_file;
}

void write_db(char value)
{
	// write value to db
	db_file = value;
}


void *reader(void *arg)
{
	int next,*id; // for time waiting value, identity respectively.
	
	id = (int*)(arg); // it is passed from thread creating, which represents producer id.
	char data; // data from db
	
	// loop infinitively
	while(1)
	{
		/*
		 * reader routine
		 * 1. acquire mutex for data modification
		 * 2. increase reader_num variable
		 * 3. if reader_num is increased to one, then lock the db for writing
		 * 4. release mutex
		 * 5. read db
		 * 6. acquire mutex for data modification
		 * 7. decrease reader_num variable
		 * 8. if reader_num is decreased to zero, then unlock the db for writing
		 * 9. release mutex
		 * 10. sleep for random time */
		sem_wait(&mutex);
		reader_num++;
		if(reader_num == 1)
		{
			sem_wait(&db);
		}
		sem_post(&mutex);
		data = read_db();
		sem_wait(&mutex);
		reader_num--;
		if(reader_num == 0)
		{
			sem_post(&db);
		}
		printf("Reader %d : read data %d from DB\n",*id,(int)data);
		sem_post(&mutex);
		
		next = 250 + rand()%(1251*(*id)); // 250ms to 2000ms for waiting
		usleep(next*1000);		
	}
}

void *writer(void *arg)
{
	int *id,coin; // for identity, coin respectively.
	char data; // data for writing
	id = (int*)(arg); // it is passed from thread creating, which represents producer id.
	struct timespec ts; // for nanotime
	
	// loop infinitively
	while(1)
	{
		/*
		 * writer routine
		 * 1. seeding with current time for random variable
		 * 2. throw coin by rand() with modulo 2 operation.
		 * 3. acquire mutex for db writing
		 * 4. write db
		 * 5. release mutex
		 * 6. sleep for one second
		 * 7. if coin is head side, yield for other thread. This is for preventing same sequence of writer activation.
		*/
		clock_gettime(CLOCK_MONOTONIC ,&ts); // use nanotime scale for different seeding.
		//printf("%ld\n",ts.tv_nsec);
		srand((unsigned int)ts.tv_nsec);
		coin = rand()%2;
		//printf("id %d, coin : %d\n",*id,coin);
			
		sem_wait(&db);
		write_db((char)(*id));
		printf("Writer %d : write %d to db\n",*id,*id);
		sem_post(&db);
		
		sleep(1);
		if(coin == 1)
		{
			sched_yield();
		}
	}
}


int main(void)
{
	int i; // for loop
	int p[6]={1,2,3,1,2,3}; // id number for reader and writer.
	pthread_t threads[6]; // thread pointer variables
	int res; // for semaphore initialization
	
	/*
	 * routine for semaphore initalization*/
	res=sem_init(&mutex, 0,1); // operate as mutex
	if(res !=0){
		perror("sem_init failed.\n");
		exit(1);
	}	
	res=sem_init(&db,0,1); // operate as mutex
	if(res!=0){
		perror("sem_init_failed.\n");
		exit(1);
	}
	/*
	 * thread creating routine
	 * each thread creates with parameter which represents its identity by numbering*/
	pthread_create(&threads[0], NULL, reader,(void*)(&p[0]));
	pthread_create(&threads[1], NULL, reader,(void*)(&p[1]));
	pthread_create(&threads[2], NULL, reader,(void*)(&p[2]));
	pthread_create(&threads[3], NULL, writer,(void*)(&p[3]));
	pthread_create(&threads[4], NULL, writer,(void*)(&p[4]));
	pthread_create(&threads[5], NULL, writer,(void*)(&p[5]));

	/*
	 * waiting routine for operation of all threads*/
	for(i=0; i<6; i++)
		pthread_join(threads[i], NULL);

	/*
	 * semaphore destory routine*/
	sem_destroy(&mutex);
	sem_destroy(&db);
	return 0;
}
