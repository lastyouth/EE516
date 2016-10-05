#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	int fd = open("hw1_door",O_RDWR);
	if(fd <= 0)
	{
		perror("open");
		return -1;
	}
	srand(time(0));
	while(1)
	{
		int r = 1 + rand()%5;

		if(r == 2)
		{
			printf("Cleaner : Make stack be clean\n");
			ioctl(fd,0);
		}
		usleep(1500000);
	}
	return 0;
}
