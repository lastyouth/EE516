#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>

// prototype of producer and consumer function, these are taking parameter arg as void pointer.
void *producer(void *arg);
void *consumer(void *arg);

// semaphores
sem_t mutex; // use as mutex
sem_t empty; 
sem_t full;

// data variables, operated as stack with size 100.
int buffer[100];
int count=0; // as stack pointer.



int produce_item()
{
	// initialize item variable with random number (0~255)
	int item = rand()%256;
	
	return item;
}

void insert_item(int v)
{
	// insert item and increase count variable.
	buffer[count++] = v;
}

int remove_item()
{
	// acquire items, and decrease count variable.
	int ret = buffer[--count];
	return ret;
}

void consume_item(int v)
{
	// consume, meaningless routine for expressing consuming
	v += v;
	v -= v;
	v++;
	v--;
}

void *producer(void *arg)
{
	int i,item,*id; // for loop iteration, item saving, identity respectively.
	
	id = (int*)(arg); // it is passed from thread creating, which represents producer id.
	
	// loop for 100 times
	for( i=1; i<=100; i++){
		/*
		 * produce routine
		 * 1. produce item
		 * 2. decrease empty semaphore, empty semaphore guarantees 100 items for maximum
		 * 3. acquire mutex for data modification
		 * 4. insert item
		 * 5. release mutex
		 * 6. increase full semaphore
		 * 7. sleep 200 ms */
		item = produce_item();
		printf("PRODUCER %d (%d) : produce item : %d\n",*id,i,item);
		sem_wait(&empty); // for 
		sem_wait(&mutex);
		insert_item(item);
		sem_post(&mutex);
		sem_post(&full);
		usleep(200*1000);
	}
}

void *consumer(void *arg)
{
	int i,item,*id; // for loop iteration, item saving, identity respectively.
	id = (int*)(arg); // it is passed from thread creating, which represents consumer id.

	// loop for 100 times
	for( i=1; i<=100; i++){
		/*
		 * produce routine
		 * 1. decrease full semaphore, full semaphore makes consumer sleep, if there is no item to consume.
		 * 3. acquire mutex for data modification
		 * 4. remove item
		 * 5. release mutex
		 * 6. increase empty semaphore
		 * 7. consume item
		 * 8. sleep 200 ms */
		sem_wait(&full);
		sem_wait(&mutex);
		item = remove_item();
		sem_post(&mutex);
		sem_post(&empty);
		consume_item(item);
		printf("CONSUMER %d (%d) : consume item : %d\n",*id,i,item);
		usleep(200*1000);
	}
}


int main(void)
{
	int i; // for loop
	int p[4]={1,2,1,2}; // id number for producer and consumer.
	pthread_t threads[4]; // thread pointer array
	int res; // for semaphore initialization
	
	srand(time(0)); // seeding for rand function
	
	/*
	 * routine for semaphore initalization*/
	res=sem_init(&mutex, 0,1); // operate as mutex
	if(res !=0){
		perror("mutex_init failed.\n");
		exit(1);
	}	
	res=sem_init(&empty,0,100); // maximum 100 to enter the critial section initially
	if(res!=0){
		perror("empty_init failed.\n");
		exit(1);
	}
	
	res=sem_init(&full, 0,0); // no one can enter the critical section initally
	if(res!=0){
		perror("full_init failed.\n");
		exit(1);
	}
	/*
	 * thread creating routine
	 * each thread creates with parameter which represents its identity by numbering*/
	pthread_create(&threads[0], NULL, producer,(void*)(&p[0]));
	pthread_create(&threads[2], NULL, producer,(void*)(&p[1]));
	pthread_create(&threads[1], NULL, consumer, (void*)(&p[2]));
	pthread_create(&threads[3], NULL, consumer, (void*)(&p[3]));
	
	/*
	 * waiting routine for operation of all threads*/
	for(i=0; i<4; i++)
		pthread_join(threads[i], NULL);
	
	// verification routine, count should be zero finally.
	printf("Final count value : %d\n",count);
	
	/*
	 * semaphore destory routine*/
	sem_destroy(&mutex);
	sem_destroy(&empty);
	sem_destroy(&full);
	return 0;
}
