#include "lib.h"
#include "stdint.h"

int main(void)
{
	int total = get_total_memoryu();
	printf("total memory is %dmb\n", total);
	return 0;
}
