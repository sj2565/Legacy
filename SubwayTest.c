#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	while(1){
		int seat1 = rand() % 2; 
		int seat2 = rand() % 2;
		int seat3 = rand() % 2;
		int seat4 = rand() % 2;

		printf("%d,%d,%d,%d\n", seat1, seat2, seat3, seat4);
		fflush(stdout);

		sleep(1);
	}
	return 0;
}
