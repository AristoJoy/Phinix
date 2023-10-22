#include <phinix/assert.h>
#include <phinix/stdarg.h>
#include <phinix/types.h>
#include <phinix/printk.h>
#include <phinix/stdio.h>

static u8 buf[1024];

static void spin(char *name)
{
    printk("spining in %s ...\n", name);
    while (true);
}
void assertion_failure(char *exp, char *file, char *base, int line)
{
    printk(
        "\n--> assert(%s) failed!!!\n"
        "--> file: %s \n"
        "--> base: %s \n"
        "--> line: %d \n",
        exp, file, base, line);

    spin("assertion_failure()");

    // 不可能走到这，否则出错
    asm volatile("ud2");
}

void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);

    printk("!!! panic !!!\n--> %s \n", buf);
    spin("panic");

    // 不可能走到这，否则出错
    asm volatile("ud2");
}
