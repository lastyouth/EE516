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
		int r = 60000+rand()%250000;
		int p = time(0)%r;
		p%=127;
		buf[0] = p;
		int s = write(fd,buf,15);
		if(s==0x7fffffff)
		{
			printf("Provider : Stack is full. Wait for comsumption\n");
		}
		else
		{
			printf("Provider : %d is pushed to stack. Current size is %d\n",p,s);
		}
		usleep(r);
	}

	close(fd);

	return 0;
}
