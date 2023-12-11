#include <phinix/interrupt.h>
#include <phinix/syscall.h>
#include <phinix/debug.h>
#include <phinix/task.h>
#include <phinix/stdio.h>



#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void idle_thread()
{
    // 设置中断开关
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        // LOGK("idle task... %d\n", counter++);
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭cpu，进入暂停状态，等待外中断的到来
        );
        yield(); // 放弃执行权，调度执行其他程序
    }
    
}

static void user_init_thread()
{

    u32 counter = 0;
    char ch;
    while (true)
    {
        // test(); 
        BOCHS_MAGIC_BP;
        char *ptr = (char *)0x900000;
        brk(ptr);

        BOCHS_MAGIC_BP;
        ptr -=0x1000;
        ptr[3] = 0xff;

        BOCHS_MAGIC_BP;
        brk((char *)0x800000);
        
        sleep(10000);
        // printf("task in in user mode %d\n, counter++");
    }
    
}

// 初始化线程
void init_thread()
{
    // set_interrupt_state(true);
    char temp[100]; // 为了栈顶有充足的空间，用于存储栈中的局部变量，不和intr_iframe_t冲突
    task_to_user_mode(user_init_thread);
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        LOGK("test task %d...\n", counter++);
        // BOCHS_MAGIC_BP;
        sleep(2000);

    }
}