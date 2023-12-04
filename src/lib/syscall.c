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

u32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void yield()
{
    _syscall0(SYS_NR_YIELD);
}

void sleep(u32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}