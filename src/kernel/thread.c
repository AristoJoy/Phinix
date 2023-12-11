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
    int status;
    while (true)
    {
        // test(); 
        // printf("init thread %d %d %d...\n", getpid(), getppid(), counter++);
        pid_t pid = fork();

        if (pid)
        {
            printf("fork after parent %d %d %d...\n", pid, getpid(), getppid());
            sleep(1000);
            pid_t child = waitpid(pid, &status);
            printf("fork after parent %d %d %d...\n", child, status, counter++);
        }
        else
        {
            printf("fork after child %d %d %d...\n", pid, getpid(), getppid());
            // sleep(1000);
            exit(0);
        }

        sleep(1000);
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
        // printf("test thread %d %d %d...\n", getpid(), getppid(), counter++);

        // BOCHS_MAGIC_BP;
        sleep(2000);

    }
}