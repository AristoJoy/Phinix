#include <phinix/phinix.h>
#include <phinix/types.h>
#include <phinix/io.h>
#include <phinix/string.h>
#include <phinix/console.h>
#include <phinix/stdarg.h>
#include <phinix/printk.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/gdt.h>
#include <phinix/task.h>
#include <phinix/interrupt.h>
#include <phinix/stdlib.h>


void kernel_init()
{
    console_init();
    gdt_init();
    interrupt_init();
    task_init();

    return;
}