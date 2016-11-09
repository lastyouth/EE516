#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

// color
#define RED 	0
#define GREEN 	1
#define BLUE 	2
#define YELLOW 	3

// monkey's state
#define NEW_MONKEY			0x10
#define INSIDE_THE_ROOM		0x11
#define TAKES_FIRST_BALL	0x12
#define TAKES_SECOND_BALL	0x13
#define CHECKING			0x14
#define THINKING			0x15
#define OUTSIDE_THE_ROOM	0x16
#define EAT_BANANAS			0x17
#define EAT_APPLES			0x18
#define END_TRAINING		0x19

// MAXIMUM MONKEYS
#define MAX_MONKEYS			4

// training purpose

#define TARGET_TRAINING		300

char *ballname[4]={"RED","GREEN","BLUE","YELLOW"};

int identity; // for monkey's id, increment by one

int trainingCounter; // for checking current trained monkey's number

struct timeval starttime; // for calculating start time


//struct for monkey's representation
struct monkey{
	int id; // monkey's id
	int requireBalls[2]; // monkey's prefered balls, in order.
	int state; // monkey's state
	int timeforThinking; // monkey's time for thinking
	int usedbowl; // bowl for eating banana
};

sem_t maxMonkeys; // for spawn monkeys

sem_t balls[4]; // balls mutex

sem_t bowls_full; // bowls mutex managed by monkeys

sem_t bowls_empty; // bowls mutex managed by trainers

sem_t ballCheckingMutex; // ballCheckingMutex

sem_t bowlCheckingMutex; // bowlCheckingMutex

sem_t counterMutex; // CounterMutex

sem_t bowls_detail[2]; // represent each bowl.

int bowls_usage[2]; // represent each bowl's usage

void finishTraining(int v)
{
	/*
	 * invoked when trainingCounter meets TARGET_TRAINING
	 * 
	 * calculate the consumed time for training*/
	struct timeval endtime;
	time_t sec,msec;
	double belowpoint;
	double finaltime;
	
	gettimeofday(&endtime,NULL);
	sec = endtime.tv_sec - starttime.tv_sec;
	msec = endtime.tv_usec - starttime.tv_usec;
	
	belowpoint = msec /(1000.0*1000);
	
	if(sec > 0)
	{	
		finaltime = sec+belowpoint;
	}
	else
	{
		finaltime = belowpoint;
	}

	
	printf("Finish training, %d monkeys have been trained (%.5lf secs)\n",v,finaltime);
	exit(0);
}

void modifyValue(int* target, int modifications,int* out)
{
	/*
	 * thread-safe value modifier
	 * 
	 * value pointed by target is modified by adding modifications
	 * 
	 * Then copy it to out*/
	sem_wait(&counterMutex);
	*target += modifications;
	*out = *target;
	sem_post(&counterMutex);
}

int acquireProperBowl()
{
	/*
	 * acquire remained Bowls for monkey
	 * return value is 0 or 1 which means bowl's number
	 * 
	 * invoked after calling sem_wait(&bowls_full) successfully*/
	int properBowl = 0;
	
	sem_wait(&bowlCheckingMutex);
	
	if(bowls_usage[0] == 0)
	{
		bowls_usage[0] = 1;
		properBowl = 0;
	}
	else if(bowls_usage[1] == 0)
	{
		bowls_usage[1] = 1;
		properBowl = 1;
	}
	else
	{
		printf("ERROR,,, in acquireProperBowl : something is wrong\n");
		properBowl = -1;
	}
	sem_post(&bowlCheckingMutex);
	return properBowl;
}

void releaseBowl(int i)
{
	/*
	 * release used bowl when monkey finished to eat banana*/
	sem_wait(&bowlCheckingMutex);
	if(bowls_usage[i] == 1)
	{
			bowls_usage[i] = 0;
	}
	else
	{
		printf("ERROR,,, in releaseBowl : something is wrong\n");
	}
	sem_post(&bowlCheckingMutex);
}

void printMonkeyState(struct monkey m)
{
	/*
	 * print function
	 * 
	 * print monkey state which is designated by 'm' */
	
	int state = m.state;
	int firstball = m.requireBalls[0], secondball = m.requireBalls[1];
	char monkeyName[256]={0,};
	char message[256]={0,};
	char finalMessage[512]={0,};
	
	// ignore this function when state is 'checking' phase
	if(state == CHECKING)
	{
		return;
	}
	
	// generate monkey's info
	sprintf(monkeyName,"Monkey %d (%s,%s) : ",m.id,ballname[firstball],ballname[secondball]);
	
	// then generate message by monkey's state
	switch(state)
	{
		case NEW_MONKEY:
			sprintf(message,"New monkey has been created\n");
		break;
		case INSIDE_THE_ROOM:
			sprintf(message,"comes into the room\n");
		break;
		case TAKES_FIRST_BALL:
			sprintf(message,"takes the %s ball\n",ballname[firstball]);
		break;
		case TAKES_SECOND_BALL:
			sprintf(message,"takes the %s ball\n",ballname[secondball]);
		break;
		case THINKING:
			sprintf(message,"is thinking for %d us\n",m.timeforThinking);
		break;
		case OUTSIDE_THE_ROOM:
			sprintf(message,"goes outside of the room\n");
		break;
		case EAT_BANANAS:
			sprintf(message,"eats banana from bowl %d\n",m.usedbowl);
		break;
		
		case EAT_APPLES:
			sprintf(message,"eats apples\n");
		break;
		case END_TRAINING:
			sprintf(message,"has finished the training, bye~\n");
		break;
	}
	printf("%s%s",monkeyName,message);
}

void thinking(struct monkey mon)
{
	// implement thinking phase. just print current state and sleep for certain amount of time
	struct monkey copiedmon;
	
	copiedmon.id = mon.id;
	copiedmon.requireBalls[0] = mon.requireBalls[0];
	copiedmon.requireBalls[1] = mon.requireBalls[1];
	copiedmon.timeforThinking = mon.timeforThinking;

	copiedmon.state = THINKING;
		
	printMonkeyState(copiedmon);
	
	usleep(copiedmon.timeforThinking);
}


void *monkeySimulatorThread(void *args)
{
	/*
	 * monkeySimulatorThread
	 * 
	 * struct monkey will be passed by 'args'
	 * 
	 * each monkey will operate like automata*/
	struct monkey mon = *(struct monkey*)args;
	int b1,b2; // for checking ball usage
	int semres,bowlNumber,currentTrainingCounter; // temporary variables
	
	while(mon.state != END_TRAINING) // for END_TRAINING state
	{
		printMonkeyState(mon); // first, print current monkey's state
		
		if(mon.state == NEW_MONKEY)
		{
			mon.state = INSIDE_THE_ROOM; // go inside the room
		}else if(mon.state == INSIDE_THE_ROOM)
		{
			
			mon.state = CHECKING; // start checking
		}else if(mon.state == CHECKING)
		{
			// check what if current monkey can acquire the required balls
			sem_wait(&ballCheckingMutex);
			sem_getvalue(&balls[mon.requireBalls[0]],&b1);
			sem_getvalue(&balls[mon.requireBalls[1]],&b2);
			
			// prevent deadlock, monkey pick first ball and reserve secondball
			if(b1 == 1 && b2 == 1)
			{
				// pick first ball
				sem_wait(&balls[mon.requireBalls[0]]);
				// reserve second ball
				sem_wait(&balls[mon.requireBalls[1]]);
				sem_post(&ballCheckingMutex);
			}
			else
			{
				// if monkey can't acquire the ball, try checking phase again
				sem_post(&ballCheckingMutex);
				continue;
			}
			mon.state = TAKES_FIRST_BALL;
		}else if(mon.state == TAKES_FIRST_BALL)
		{
			// think for certain amount of time
			thinking(mon);
			mon.state = TAKES_SECOND_BALL;
		}else if(mon.state == TAKES_SECOND_BALL)
		{
			thinking(mon);
			// release balls
			sem_post(&balls[mon.requireBalls[0]]);
			sem_post(&balls[mon.requireBalls[1]]);
			mon.state = OUTSIDE_THE_ROOM;
		}else if(mon.state == OUTSIDE_THE_ROOM)
		{
			// spawn new monkeythread
			sem_post(&maxMonkeys);
			
			// check current bowls	
			sem_trywait(&bowls_full);
			
			if(errno == EAGAIN)
			{
				// should eats apple, there is no available bowl
				mon.state = EAT_APPLES;
			}
			else
			{
				// succeed acquiring bowls_full semaphore, wait for banana
				bowlNumber = acquireProperBowl();
				mon.usedbowl = bowlNumber+1;
				// make trainer wake up.
				sem_post(&bowls_empty);
				
				// wait for trainer's feeding
				sem_wait(&bowls_detail[bowlNumber]);
				
				// monkey has acquired banana, release the bowl
				releaseBowl(bowlNumber);
			
				mon.state = EAT_BANANAS;
				
				sem_post(&bowls_full);
			}
						
		}else if(mon.state == EAT_APPLES)
		{
			// monkey eats apples
			mon.state = END_TRAINING;
		}
		else if(mon.state == EAT_BANANAS)
		{
			// monkey eats banana
			mon.state = END_TRAINING;
		}
		usleep(10);
	}
	// training has finished
	printMonkeyState(mon);
	
	free(args);
	// modify the trainingCounter value
	modifyValue(&trainingCounter,1,&currentTrainingCounter);
	
	// check whether, total training has been finished.
	if(currentTrainingCounter == TARGET_TRAINING)
	{
		finishTraining(currentTrainingCounter);
	}
}

void *trainerSimulatorThread(void *args)
{
	/*
	 * trainerSimulatorThread
	 * 
	 * simulate the trainer's action*/
	int val;
	
	while(1)
	{
		sem_getvalue(&bowls_empty,&val);
		if(val == 0)
		{
			printf("Trainer : goes to sleep\n");
		}
		sem_wait(&bowls_empty);
		
		// wake up by monkey
		if(bowls_usage[0] == 1)
		{
				printf("Trainer : puts bananas to bowl 1\n");
				sem_post(&bowls_detail[0]);
				usleep(5000);
				continue;
		}
		else if(bowls_usage[1] == 1)
		{
				printf("Trainer : puts bananas to bowl 2\n");
				sem_post(&bowls_detail[1]);
		}
		usleep(5000);
	}
}

int getNanoTimeRandomNumber(int range,int excluded)
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
	
	if(excluded == -1)
	{	
		ret = rand()%range;
	}
	else
	{
		while((ret = rand()%range) == excluded); // prevent duplicated number
	}
	
	return ret;
}

void spawnMonkeyThread()
{
	/*
	 * spawnMonkeyThead
	 * 
	 * generate new struct monkey variable and allocate the value to it
	 * 
	 * then start new thread for it*/
	int firstball,secondball,waitingtime;
	struct monkey* newMonkey = (struct monkey*)malloc(sizeof(struct monkey));
	pthread_t *target;
	int id;

	firstball = getNanoTimeRandomNumber(4,-1);
	secondball = getNanoTimeRandomNumber(4,firstball);
	waitingtime = 1000+getNanoTimeRandomNumber(2500,-1);
	
	modifyValue(&identity,1,&id);
	newMonkey->id = id;
	newMonkey->state = NEW_MONKEY;
	newMonkey->requireBalls[0] = firstball;
	newMonkey->requireBalls[1] = secondball;
	newMonkey->timeforThinking = waitingtime;
	
	target = (pthread_t*)malloc(sizeof(pthread_t));
	
	pthread_create(target, NULL, monkeySimulatorThread,(void*)(newMonkey));	
}

void spawnTrainerThread()
{
	/*
	 * nothing special, just create for trainer's thread
	 * */
	pthread_t *target;
	
	target = (pthread_t*)malloc(sizeof(pthread_t));
	
	pthread_create(target,NULL,trainerSimulatorThread,NULL);
}


void initSemaphore()
{
	
	// for ballchecking
	sem_init(&ballCheckingMutex,0,1);
	
	// for counter
	sem_init(&counterMutex,0,1);
	
	// for bowlchecking
	sem_init(&bowlCheckingMutex,0,1);
	
	// 4 monkeys in the room
	sem_init(&maxMonkeys,0,MAX_MONKEYS);
	
	// 2 bowls for monkeys
	sem_init(&bowls_full,0,2);
	
	// trainer's duty
	sem_init(&bowls_empty,0,0);
	
	// red ball
	sem_init(&balls[RED],0,1);
	// green ball
	sem_init(&balls[GREEN],0,1);
	// blue ball
	sem_init(&balls[BLUE],0,1);
	// yellow ball
	sem_init(&balls[YELLOW],0,1);
	
	// first bowl
	sem_init(&bowls_detail[0],0,0);
	
	// second bowl
	sem_init(&bowls_detail[1],0,0);
	
}

int main()
{
	/*
	 * very simple main function*/
	initSemaphore();
	
	gettimeofday(&starttime,NULL); // for calculating total training time
	
	spawnTrainerThread(); // spawnTrainerThread
	
	// this loop operates like threadPool
	while(1)
	{
		sem_wait(&maxMonkeys);
		spawnMonkeyThread();
	}
	return 0;
}
