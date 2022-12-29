#include "debug.h"
#include "print.h"

void error_check(char *file, uint64_t line)
{
	printk("\n------------------------------------------\n");
	printk("             ERROR CHECK");
	printk("\n------------------------------------------\n");
	printk("Assertion Failed [%s:%u]", file, line);

	while (1) { }
}