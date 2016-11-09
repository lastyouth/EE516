#include <stdio.h>

#define __NR_mycall 322

int main()
{
	int a = 20160000, b = 709,res;
	
	res = syscall(__NR_mycall,a,b);
	
	printf("sbhcallforaddition return value : %d\n",res);
	
	return 0;
}
