#ifndef PHINIX_ARENA_H
#define PHINIX_ARENA_H

#include <phinix/types.h>
#include <phinix/list.h>


#define DESC_COUNT 7

typedef list_node_t block_t; // 内存块

// 内存描述符
typedef struct arena_descriptor_t
{
    u32 total_block; // 一页内存分成了多少块
    u32 block_size; // 块大小
    int page_count; // 空闲页数量
    list_t free_list; // 空闲链表
} arena_descriptor_t;

// 一页或多页内存
typedef struct arena_t
{
    arena_descriptor_t *desc; // 该arena描述符
    u32 count;      // 当前剩余多少块或页数
    u32 large;      // 表示是不是超过1024个字节
    u32 magic;      // 魔数
} arena_t;

// 内核堆内存分配
void *kmalloc(size_t size);
// 内核堆内存回收
void kfree(void *ptr);


#endif