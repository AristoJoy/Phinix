#include <phinix/interrupt.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/syscall.h>
#include <phinix/task.h>
#include <phinix/console.h>
#include <phinix/memory.h>


#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define SYSCALL_SIZE 256

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

#include <phinix/string.h>
#include <phinix/ide.h>

extern ide_ctrl_t controllers[2];

static u32 sys_test()
{
    u16 *buf = (u16 *)alloc_kpage(1);
    BOCHS_MAGIC_BP;
    LOGK("read buffer %x\n", buf);
    ide_pio_read(&controllers[0].disks[0], buf, 4, 0);
    BOCHS_MAGIC_BP;
    memset(buf, 0x5a, SECTOR_SIZE);
    BOCHS_MAGIC_BP;
    ide_pio_write(&controllers[0].disks[0], buf, 1, 1);

    free_kpage((u32)buf, 1);
    return 255;
}

int32 sys_write(fd_t fd, char *buf, u32 len)
{
    if (fd == stdout || fd == stderr)
    {
        return console_write(buf, len);
    }
    // todo
    panic("sys write error!!!");
    return 0;
}

extern time_t sys_time();

void syscall_init()
{

    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = syscall_default;
    }
    
    syscall_table[SYS_NR_TEST] = sys_test;

    syscall_table[SYS_NR_EXIT] = task_exit;
    syscall_table[SYS_NR_FORK] = task_fork;
    syscall_table[SYS_NR_WAITPID] = task_waitpid;
    
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;

    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;

    syscall_table[SYS_NR_BRK] = sys_brk;

    syscall_table[SYS_NR_WRITE] = sys_write;

    syscall_table[SYS_NR_TIME] = sys_time;
}