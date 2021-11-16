#include "lib.h"
#include "stdint.h"

int main(void)
{

	printf("process 3 runs!\n");

	int64_t counter = 0;

	while (1) {
		if(counter % 100000000 == 0)
		//	printf("process3 %d\n", counter);
		counter++;
	}
	printf("process 3 ends!\n");

	return 0;
}
