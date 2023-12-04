#ifndef PHINIX_SYSCALL_H
#define PHINIX_SYSCALL_H

#include <phinix/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_YIELD,
} syscall_t;

u32 test();
void yield();

#endif