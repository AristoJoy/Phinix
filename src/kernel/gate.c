#include <phinix/interrupt.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/syscall.h>
#include <phinix/task.h>





#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define SYSCALL_SIZE 64

handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 func_code)
{   
    if (func_code >= SYSCALL_SIZE)
    {
        panic("syscall func code error!!!");
    }
    
}

static void syscall_default()
{
    panic("syscall not implemented!!!");
}

task_t *task = NULL;

static u32 sys_test()
{
    // LOGK("syscall test...\n");
    if (!task)
    {
        task = running_task();
        task_block(task, NULL, TASK_BLOCKED);
    }
    else
    {
        task_unblock(task);
        task = NULL;
    }
    
    return 255;
}

void syscall_init()
{

    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = syscall_default;
    }
    
    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;
}