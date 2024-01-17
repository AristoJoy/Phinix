#ifndef PHINIX_MIO_H
#define PHINIX_MIO_H

#include <phinix/types.h>

// 映射内存IO

u8 mem_in_byte(u32 addr);   // 输入一个字节
u16 mem_in_word(u32 addr);  // 输入一个字
u32 mem_in_dword(u32 addr); // 输入一个双字

void mem_out_byte(u32 addr, u8 value);   // 输出一个字节
void mem_out_word(u32 addr, u16 value);  // 输出一个字
void mem_out_dword(u32 addr, u32 value); // 输出一个双字

#endif