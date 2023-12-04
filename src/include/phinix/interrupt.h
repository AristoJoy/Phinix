#ifndef PHINIX_INTERRUPT_H
#define PHINIX_INTERRUPT_H

#include <phinix/types.h>

#define IDT_SIZE 256

#define IRQ_CLOCK 0     // 时钟
#define IRQ_KEYBOARD 1     // 键盘
#define IRQ_CASCADE 2     // 8259 从片控制器
#define IRQ_SERIAL_2 3     // 串口2
#define IRQ_SERIAL_1 4     // 串口1
#define IRQ_PARALLEL_2 5     // 并口2
#define IRQ_FLOPPY 6     // 软盘控制器
#define IRQ_PARALLEL_1 7 // 并口1
#define IRQ_RTC 8     // 实时时钟
#define IRQ_REDIRECT 9     // 重定向 IRQ2
#define IRQ_MOUSE 12     // 鼠标
#define IRQ_MATH 13     // 协处理器 x87
#define IRQ_HARDDISK 14     // ATA 硬盘第一通道
#define IRQ_HARDDISK2 15     // ATA 硬盘第二通道

#define IRQ_MASTER_NR 0x20 // 主片起始向量号
#define IRQ_SLAVE_NR 0x28 // 从片起始向量号
/**
 * @brief 门结构
 * 
 */
typedef struct gate_t
{
    u16 offset_low;    // 段内偏移 0 ~ 15 位
    u16 selector;      // 代码段选择子
    u8 reserved;       // 保留不用
    u8 type: 4;        // 门类型 任务门/中断门/陷阱门
    u8 segment: 1;     // segment = 0 表示系统段
    u8 DPL : 2;        // 当前代码访问的权限门槛
    u8 present: 1;     // 是否在内存中
    u16 offset_high;   // 段内偏移 16 ~ 32 位
} _packed gate_t;

typedef void *handler_t; // 中断处理函数

void interrupt_init();
void send_eoi(int vector);

// 设置中断处理函数
void set_interrupt_handler(u32 irq, handler_t handler);
void set_interrupt_mask(u32 irq, bool enable);

bool interrupt_disable(); // 清除eflags IF位，并返回设置之前的值
bool get_interrupt_state(); // 获得IF位
void set_interrupt_state(bool state); // 设置IF位

#endif