#ifndef PHINIX_IO_H
#define PHINIX_IO_H

#include <phinix/types.h>

extern u8 in_byte(u16 port); // 输入一个字节
extern u16 in_word(u16 port); // 输入一个字
extern u32 in_dword(u16 port); // 输入一个双字

extern void out_byte(u16 port, u8 value);  // 输出一个字节
extern void out_word(u16 port, u16 value); // 输出一个字
extern void out_dword(u16 port, u32 value); // 输出一个双字

#endif
