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

static void user_init_thread()
{


    while (true)
    {

        sleep(1000);

    }
    
}

// 初始化线程
void init_thread()
{
    char temp[100]; // 为了栈顶有充足的空间，用于存储栈中的局部变量，不和intr_iframe_t冲突
    task_to_user_mode(user_init_thread);
}

void test_thread()
{
    char buf[256];
    memset(buf, 'A', sizeof(buf));

    fd_t fd;
    int len = 0;
    fd = open("/hello.txt", O_RDWR, 0755);
    lseek(fd, 5, SEEK_SET);
    len = write(fd, buf, sizeof(buf));
    close(fd);

    set_interrupt_state(true);
    while (true)
    {
        // test();
        char ch;
        read(stdin, &ch, 1);
        write(stdout, &ch, 1);
        sleep(10);

    }
}