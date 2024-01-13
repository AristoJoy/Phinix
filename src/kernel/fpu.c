#include <phinix/fpu.h>
#include <phinix/task.h>
#include <phinix/cpu.h>
#include <phinix/interrupt.h>
#include <phinix/arena.h>
#include <phinix/debug.h>
#include <phinix/assert.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

task_t *last_fpu_task = NULL;

bool fpu_check()
{
    cpu_version_t ver;
    cpu_version(&ver);
    if (!ver.FPU)
    {
        return false;
    }
    u32 testword = 0x55AA;
    u32 ret;

    asm volatile(
        "movl %%cr0, %%edx\n" // 获取cr0寄存器
        "andl %%ecx, %%edx\n" // 清楚EM TS 保证 FPU可用
        "movl %%edx, %%cr0\n" // 设置cr0寄存器

        "fninit\n"    // 初始化 FPU
        "fnstsw %1\n" // 保持状态字到testword

        "movl %1,%%eax\n" // 将状态字保持到 eax
        : "=a"(ret)       // 将eax写入 ret;
        : "m"(testword), "c"(-1 - CR0_EM - CR0_TS));

    return ret == 0;
}

// 得到 cr0 寄存器
u32 get_cr0()
{
    // 直接将 mov eax, cr0，返回值在 eax 中
    asm volatile("movl %cr0, %eax\n");
}

// 设置 cr0 寄存器，参数是页目录的地址
void set_cr0(u32 cr0)
{
    asm volatile("movl %%eax, %%cr0\n" ::"a"(cr0));
}

// 激活 fpu
void fpu_enable(task_t *task)
{
    LOGK("fpu enable...\n");
    set_cr0(get_cr0() & ~(CR0_EM | CR0_TS));

    // 如果使用的任务没有变化，则无需恢复浮点环境
    if (last_fpu_task == task)
    {
        return;
    }

    // 如果存在使用过浮点处理单元的经常，则保持浮点环境
    if (last_fpu_task && last_fpu_task->flags & TASK_FPU_ENABLED)
    {
        assert(last_fpu_task->fpu);
        asm volatile("fnsave (%%eax) \n" ::"a"(last_fpu_task->fpu));
        last_fpu_task->flags &= ~TASK_FPU_ENABLED;
    }

    last_fpu_task = task;

    // 如果fpu不为空，则恢复浮点环境
    if (task->fpu)
    {
        asm volatile("frstor (%%eax) \n" ::"a"(task->fpu));
    }
    else
    {
        // 否则，初始化浮点环境
        asm volatile("fnclex \n"
                     "fninit \n");
        LOGK("FPU create state for task 0x%p\n", task);
        task->fpu = (fpu_t *)kmalloc(sizeof(fpu_t));
        task->flags |= (TASK_FPU_ENABLED | TASK_FPU_USED);
    }
}

// 禁用fpu
void fpu_disable(task_t *task)
{
    set_cr0(get_cr0() | (CR0_EM | CR0_TS));
}

// 浮点错误处理器
void fpu_handler(int vector)
{
    LOGK("fpu handler...\n");
    assert(vector == INTR_NM);
    task_t *task = running_task();
    assert(task->uid);

    fpu_enable(task);
}

// 初始化 FPU
void fpu_init()
{
    LOGK("fpu init...\n");

    bool exist = fpu_check();
    last_fpu_task = NULL;
    assert(exist);

    if (exist)
    {
        // 设置异常处理函数，非常类似于中断
        set_exception_handler(INTR_NM, fpu_handler);
        // 设置cr0寄存器
        set_cr0(get_cr0() | (CR0_EM | CR0_TS | CR0_NE));
    }
    else
    {
        LOGK("fpu not exists...\n");
    }
}