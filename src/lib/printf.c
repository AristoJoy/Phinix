#include <phinix/stdarg.h>
#include <phinix/stdio.h>
#include <phinix/syscall.h>

static char buf[1024];

int printf(const char *fmt, ...)
{
    va_list args;
    int i;
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    write(STDOUT_FILENO, buf, i);

    return i;
}