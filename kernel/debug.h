#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "stdint.h"

#define ASSERT(e) do {                  \
	if (!(e))                           \
		error_check(__FILE__,__LINE__); \
} while (0) 

void error_check(char *file, uint64_t line);

#endif