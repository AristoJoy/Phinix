#include <phinix/syscall.h>

static _inline u32 _syscall0(u32 func_code)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(func_code)
    );
    return ret;
}

static _inline u32 _syscall1(u32 nr, u32 arg)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        :"=a"(ret)
        :"a"(nr), "b"(arg)
    );
    return ret;
}

static _inline u32 _syscall2(u32 nr, u32 arg1, u32 arg2)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

static _inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

u32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void exit(int status)
{
    return _syscall1(SYS_NR_EXIT, (u32)status);
}

pid_t fork()
{
    return _syscall0(SYS_NR_FORK);
}

pid_t waitpid(pid_t pid, int32 *status)
{
    return _syscall2(SYS_NR_WAITPID, pid, (u32) status);
}

void yield()
{
    _syscall0(SYS_NR_YIELD);
}

void sleep(u32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}

// 获取任务id
pid_t getpid()
{
    return _syscall0(SYS_NR_GETPID);
}

// 获取父任务id
pid_t getppid()
{
    return _syscall0(SYS_NR_GETPPID);
}

// brk调用
int32 brk(void *addr)
{
    return _syscall1(SYS_NR_BRK, (u32) addr);
}

// 系统调用write
int32 write(fd_t fd, char *buf, u32 len)
{
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

// 获取从1970 1 1 00:00:00 开始的秒数
time_t time()
{
    return _syscall0(SYS_NR_TIME);
}

mode_t umask(mode_t mask)
{
    return _syscall1(SYS_NR_UMASK, (u32)mask);
}
