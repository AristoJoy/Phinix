#include <phinix/arena.h>
#include <phinix/memory.h>
#include <phinix/string.h>
#include <phinix/stdlib.h>
#include <phinix/assert.h>

#define BUF_COUNT 4 // 堆内存缓存页数量

extern u32 free_pages;
static arena_descriptor_t descriptors[DESC_COUNT];

// 初始化堆内存块, 分成几级，16 ~ 1024大小
void arena_init()
{
    u32 block_size = 16;
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        arena_descriptor_t *desc = &descriptors[i];
        desc->block_size = block_size;
        desc->page_count = 0;
        desc->total_block = (PAGE_SIZE - sizeof(arena_t)) / block_size;
        list_init(&desc->free_list);
        block_size <<= 1;
    }
    
}

// 获取arena的block
static void *get_arena_block(arena_t *arena, u32 idx)
{
    assert(arena->desc->total_block > idx);
    void *addr = (void *)(arena + 1);
    u32 gap = idx * arena->desc->block_size;
    return addr + gap;
}

// 获取block的arena
static arena_t *get_block_arena(block_t *block)
{
    return (arena_t *)((u32)block & 0xfffff000);
}

// 内核堆内存分配
void *kmalloc(size_t size)
{
    arena_descriptor_t *desc = NULL;
    arena_t *arena;
    block_t *block;

    char *addr;

    // 大块内存直接分配页
    if (size > 1024)
    {
        u32 asize = size + sizeof(arena_t);
        u32 count  = div_round_up(asize, PAGE_SIZE);

        arena = (arena_t *)alloc_kpage(count);
        memset(arena, 0, count * PAGE_SIZE);
        arena->large = true;
        arena->count = count;
        arena->desc = NULL;
        arena->magic = PHINIX_MAGIC;

        addr = (char *)((u32)arena + sizeof(arena_t));
        return addr;
    }
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        desc = &descriptors[i];
        if (desc->block_size >= size)
        {
            break;
        }
    }

    assert(desc != NULL);

    // 如果当前描述符空闲链表没有空闲块，这分配一页内存划分块
    if (list_empty(&desc->free_list))
    {
        arena = (arena_t *)alloc_kpage(1);
        memset(arena, 0, PAGE_SIZE);

        desc->page_count++;

        arena->desc = desc;
        arena->large = false;
        arena->count = desc->total_block;
        arena->magic = PHINIX_MAGIC;

        // 将空闲块添加到空闲链表中
        for (size_t i = 0; i < desc->total_block; i++)
        {
            block = get_arena_block(arena, i);
            assert(!list_search(&arena->desc->free_list, block));
            list_push(&arena->desc->free_list, block);
            assert(list_search(&arena->desc->free_list, block));
        }
    }
    
    block = list_pop(&desc->free_list);

    arena = get_block_arena(block);

    assert(arena->magic == PHINIX_MAGIC && !arena->large);

    // 空闲块减1
    arena->count--;
    return block;
}
// 内核堆内存回收，假设ptr从kmalloc分配
void kfree(void *ptr)
{
    assert(ptr);

    block_t *block = (block_t *)ptr;
    arena_t *arena = get_block_arena(block);

    assert(arena->large == 1 || arena->large == 0);
    assert(arena->magic == PHINIX_MAGIC);

    // 回收大块内存
    if (arena->large)
    {
        free_kpage((u32)arena, arena->count);
        return;
    }
    
    // 将块重新链接到空闲链表
    list_push(&arena->desc->free_list, block);
    arena->count++;

    // 如果所有内存块都空闲，将空闲块从空闲链表移除,则像操作系统返回这一页
    if (arena->count == arena->desc->total_block && arena->desc->page_count > BUF_COUNT)
    {
        for (size_t i = 0; i < arena->desc->total_block; i++)
        {
            block = get_arena_block(arena, i);
            assert(list_search(&arena->desc->free_list, block));
            list_remove(block);
            assert(!list_search(&arena->desc->free_list, block));
        }
        free_kpage((u32)arena, 1);
        arena->desc->page_count--;
        assert(arena->desc->page_count >= BUF_COUNT);
    }
    
}
