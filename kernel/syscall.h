#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "trap.h"

typedef int (*SYSTEMCALL)(int64_t *argptr);
void init_system_call(void);
void system_call(struct TrapFrame *tf);

#endif
