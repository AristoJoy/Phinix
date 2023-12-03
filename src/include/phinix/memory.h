#ifndef PHINIX_MEMORY_H
#define PHINIX_MEMORY_H

#include <phinix/types.h>

#define PAGE_SIZE 0x1000     // 页大小 4k
#define MEMORY_BASE 0x100000 // 1M 可用内存开始的位置

typedef struct page_entry_t
{
    u8 present : 1;  // 在内存中
    u8 write : 1;    // 0 只读， 1可读可写
    u8 user : 1;     // 1 所有人 0 超级用户 DPL < 3
    u8 pwt : 1;      // page write through 直写模式
    u8 pcd : 1;      // page cache disable 禁止改业缓冲
    u8 accessed : 1; // 被访问过，用于统计使用频率
    u8 dirty : 1;    // 脏页，表示该页被缓冲写过
    u8 pat : 1;      // page attribute table 页大小 4K/4M
    u8 global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    u8 ignored : 3;  // 送给操作系统
    u32 index : 20;  // 页索引
} _packed page_entry_t;

u32 get_cr3();

void set_cr3(u32 pde);

#endif