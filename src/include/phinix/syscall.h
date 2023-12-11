#ifndef PHINIX_SYSCALL_H
#define PHINIX_SYSCALL_H

#include <phinix/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_WRITE = 4,
    SYS_NR_BRK = 45,
    SYS_NR_SLEEP = 158,
    SYS_NR_YIELD = 162,
} syscall_t;

u32 test();
void yield();
void sleep(u32 ms);

// brk调用
int32 brk(void *addr);

// 系统调用write
int32 write(fd_t fd, char *buf, u32 len);

#endif