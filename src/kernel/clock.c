#include <phinix/io.h>
#include <phinix/interrupt.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/task.h>

#define PIT_CHAN0_REG 0x40
#define PIT_CHAN2_REG 0x42
#define PIT_CTRL_REG 0x43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0X61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)

// 时间片计数器
u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

u32 volatile beeping = 0;

void start_beep()
{
    if (!beeping)
    {
        out_byte(SPEAKER_REG, in_byte(SPEAKER_REG) | 0b11);
    }
    beeping = jiffies + 5;
}

void stop_beep()
{
    if (beeping && jiffies > beeping)
    {
        out_byte(SPEAKER_REG, in_byte(SPEAKER_REG) & 0xfc);
        beeping = 0;
    }
}

extern void task_wakeup();

void clock_handler(int vector)
{
    assert(vector == 0x20);
    send_eoi(vector); // 发送中断处理结束

    stop_beep();    // 检测并停止蜂鸣器

    task_wakeup(); // 唤醒睡眠结束的任务
    
    jiffies++;
    // DEBUGK("clock jiffies %d ...\n", jiffies);

    task_t *task = running_task();
    assert(task->magic == PHINIX_MAGIC);

    task->jiffies = jiffies;
    task->ticks--;
    if (!task->ticks)
    {
        task->ticks = task->priority;
        schedule();
    }
    
}

void pit_init()
{
    // 配置计数器0 时钟
    out_byte(PIT_CTRL_REG, 0b00110100);
    out_byte(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    out_byte(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);

    // 配置计数器2 蜂鸣器
    out_byte(PIT_CTRL_REG, 0b10110110);
    out_byte(PIT_CHAN2_REG, (u8)BEEP_COUNTER);
    out_byte(PIT_CHAN2_REG, (u8)(BEEP_COUNTER >> 8));
}

void clock_init()
{
    pit_init();
    set_interrupt_handler(IRQ_CLOCK, clock_handler);
    set_interrupt_mask(IRQ_CLOCK, true);
}