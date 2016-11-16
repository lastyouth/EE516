#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#define MAX_ALLOCATION_SIZE 16
#define NEXT_ALLOCATION_SIZE 32

int main()
{
	char *ptr = 0;
	int i;

	ptr = (char*)malloc(sizeof(char)*MAX_ALLOCATION_SIZE);

	printf("malloc with size %d bytes : actual allocated size is %d bytes\n",MAX_ALLOCATION_SIZE,malloc_usable_size(ptr));

	for(i=0;i<MAX_ALLOCATION_SIZE;i++)
	{
		*(ptr+i) = i+1;
		printf("[0x%X] - value(%d)\n",(unsigned int)(ptr+i),*(ptr+i));
	}
	
	ptr = (char*)realloc(ptr,NEXT_ALLOCATION_SIZE);
	printf("after realloc with size %d bytes : actual re-allocated size is %d bytes\n",NEXT_ALLOCATION_SIZE,malloc_usable_size(ptr));
	
	for(i=0;i<NEXT_ALLOCATION_SIZE;i++)
	{
		*(ptr+i) = i+1;
		printf("[0x%X] - value(%d)\n",(unsigned int)(ptr+i),*(ptr+i));
	}
	
	free(ptr);
	return 0;
}
