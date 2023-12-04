#include <phinix/interrupt.h>
#include <phinix/syscall.h>
#include <phinix/debug.h>

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

#include <phinix/mutex.h>

mutex_t mutex;

void init_thread()
{
    mutex_init(&mutex);
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        mutex_lock(&mutex);
        LOGK("init task %d...\n", counter++);
        // sleep(500);
        mutex_unlock(&mutex);
    }
    
}

void test_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;

    while (true)
    {
        mutex_lock(&mutex);
        LOGK("test task %d...\n", counter++);
        // sleep(709);
        mutex_unlock(&mutex);
    }
}