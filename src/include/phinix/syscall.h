#ifndef PHINIX_SYSCALL_H
#define PHINIX_SYSCALL_H

#include <phinix/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_WRITE,
    SYS_NR_SLEEP,
    SYS_NR_YIELD,
} syscall_t;

u32 test();
void yield();
void sleep(u32 ms);

// 系统调用write
int32 write(fd_t fd, char *buf, u32 len);

#endif