#ifndef PHINIX_INTERRUPT_H
#define PHINIX_INTERRUPT_H

#include <phinix/types.h>

#define IDT_SIZE 256

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

#endif