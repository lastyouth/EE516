#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

int main()
{
	int fd = open("hw1_door",O_RDWR);
	char buf[20]={0,};

	if(fd <= 0)
	{
		perror("open");
		return -1;
	}

	srand(22);

	while(1)
	{
		int r = 1500000+rand()%1000000;
		int f = read(fd,buf,15);
		if(f == -1)
		{
			printf("Consumer : Stack is empty. Wait for production\n");
		}
		else
		{
			printf("Consumer : %d is popped from stack\n",f);
		}
		usleep(r);
	}

	close(fd);

	return 0;
}
