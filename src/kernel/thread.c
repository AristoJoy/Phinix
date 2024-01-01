#include <phinix/interrupt.h>
#include <phinix/syscall.h>
#include <phinix/debug.h>
#include <phinix/task.h>
#include <phinix/stdio.h>
#include <phinix/fs.h>
#include <phinix/string.h>



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

extern void dev_init();

// 初始化线程
void init_thread()
{
    char temp[100]; // 为了栈顶有充足的空间，用于存储栈中的局部变量，不和intr_iframe_t冲突
    dev_init();
    task_to_user_mode();
}


void test_thread()
{


    set_interrupt_state(true);
    while (true)
    {
        sleep(10);

    }
}