#include <phinix/memory.h>
#include <phinix/types.h>
#include <phinix/debug.h>
#include <phinix/assert.h>
#include <phinix/stdlib.h>
#include <phinix/string.h>
#include <phinix/bitmap.h>
#include <phinix/multiboot2.h>
#include <phinix/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID 1  // ards 可用内存区域
#define ZONE_RESERVED // ards 不可用区域

#define IDX(addr) ((u32)addr >> 12)                   // 获取addr的页索引
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)        // 获取addr的页目录索引
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff)        // 获取addr的页表索引
#define PAGE(idx) ((u32)idx << 12)                    // 获取页索引idx对应的页开始的位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0) // 判断是否是一页的起始位置

// 页目录地址mask
#define PDE_MASK 0xFFC00000

// 内核页表索引
static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
};

#define KERNEL_MAP_BITS 0x4000

bitmap_t kernel_map;

#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

typedef struct ards_t
{
    u64 base; // 内存基地址
    u64 size; // 内存长度
    u32 type; // 类型
} _packed ards_t;

static u32 memory_base = 0; // 可用内存基地址，应该等于 1M
static u32 memory_size = 0; // 可用内存大小
static u32 total_pages = 0; // 所有内存页数
static u32 free_pages = 0;  // 空闲内存页数

#define used_pages (total_pages - free_pages) // 已用页数

void memory_init(u32 magic, u32 addr)
{
    u32 count;

    if (magic == PHINIX_MAGIC)
    {
        count = *(u32 *)addr;
        ards_t *ptr = (ards_t *)(addr + 4);

        for (size_t i = 0; i < count; i++, ptr++)
        {
            LOGK("memory base 0x%p\n, size 0x%p type %d\n", (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size)
            {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    }
    else if (magic == MULTIBOOT2_MAGIC)
    {
        u32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        LOGK("Announced mbi size 0x%x\n", size);
        while (tag->type != MULTIBOOT_TAG_TYPE_END)
        {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
            {
                break;
            }
            // 下一个tag对齐到8字节
            tag = (multi_tag_t *)((u32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
        multi_mmap_entry_t *entry = mtag->entries;
        while ((u32)entry < (u32)tag + tag->size)
        {
            LOGK("Memory base 0x%p size 0x%p type %d\n",
                 (u32)entry->addr, (u32)entry->len, (u32)entry->type);
            count++;
            if (entry->type = ZONE_VALID && entry->len > memory_size)
            {
                memory_base = (u32)entry->addr;
                memory_size = (u32)entry->len;
            }
            entry = (multi_mmap_entry_t *)((u32)entry + mtag->entry_size);
        }
    }
    else
    {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    LOGK("ARDS count %d\n", count);
    LOGK("Memory base 0x%p\n", (u32)memory_base);
    LOGK("Memory size 0x%p\n", (u32)memory_size);

    assert(memory_base == MEMORY_BASE); // 要求内存开始位置为1M
    assert((memory_size & 0xfff) == 0); // 要求页对齐

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    LOGK("Total pages %d\n", total_pages);
    LOGK("Free pages %d\n", free_pages);

    if (memory_size < KERNEL_MEMORY_SIZE)
    {
        panic("System memory is %dM too small, at least %dM needed\n",
              memory_size / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static u32 start_page = 0;   // 可分配物理内存起始位置
static u8 *memory_map;       // 物理内存数组
static u32 memory_map_pages; // 物理内存数字占用的页数

void memory_map_init()
{
    // 初始化物理内存数组
    memory_map = (u8 *)memory_base;

    // 计算物理内存数组占用的页数
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);

    LOGK("Memory map page count %d\n", memory_map_pages);

    // 清零物理内存数组
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    // 前1M的内存位置以及物理内存数组已占用的页，已被占用
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    for (size_t i = 0; i < start_page; i++)
    {
        memory_map[i] = 1;
    }

    LOGK("Total pages %d free page %d\n", total_pages, free_pages);

    // 初始化内核虚拟内存位图，需要8位对齐
    u32 length = ((IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8);
    bitmap_init(&kernel_map, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, memory_map_pages);
}

static u32 get_page()
{
    for (size_t i = start_page; i < total_pages; i++)
    {
        if (!memory_map[i])
        {
            memory_map[i] = 1;
            free_pages--;
            assert(free_pages >= 0);
            u32 page = PAGE(i);
            LOGK("Get page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of Memory!!!");
}

// 释放一些物理内存
static void put_page(u32 addr)
{
    ASSERT_PAGE(addr);

    u32 idx = IDX(addr);

    // idx大于1M且小于总页面数
    assert(idx >= start_page && idx < total_pages);

    // 保证只有一个引用
    assert(memory_map[idx] >= 1);

    // 物理引用减1
    memory_map[idx]--;

    if (!memory_map[idx])
    {
        free_pages++;
    }
    assert(free_pages > 0 && free_pages < total_pages);

    LOGK("Put page 0x%p\n", addr);
}

// 得到cr3寄存器
u32 get_cr2()
{
    // 直接将mov eax, cr2，返回值在eax中
    asm volatile("movl %cr2, %eax\n");
}

// 得到cr3寄存器
u32 get_cr3()
{
    // 直接将mov eax, cr3，返回值在eax中
    asm volatile("movl %cr3, %eax\n");
}

// 设置cr3寄存器
void set_cr3(u32 pde)
{
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

// 将cr0寄存器最高位PE置为1，启用分页
static _inline void enable_page()
{
    // 0b1000_0000_0000_0000_0000_0000_0000_0000
    // 0x80000000
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0\n");
}

// 初始化页表项
static void entry_init(page_entry_t *entry, u32 index)
{
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

// 初始化内存映射
void mapping_init()
{
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;

    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++)
    {
        // 初始化页表
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);
        // 设置页目录项
        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX((u32)pte));

        for (size_t tidx = 0; tidx < 1024; tidx++, index++)
        {
            // 第0页不映射，为造成空指针访问，缺页异常，便于排查错误
            if (index == 0)
            {
                continue;
            }

            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1; //  设置物理内存数组，该页被占用
        }
    }

    // 将最后一个页表指向页目录自己，方便修改
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    // 设置cr3寄存器
    set_cr3((u32)pde);

    // 分页启用
    enable_page();
}

// 获取页目录
static page_entry_t *get_pde()
{
    return (page_entry_t *)(0xfffff000);
}

// 获取虚拟地址vaddr对应的页表
static page_entry_t *get_pte(u32 vaddr, bool create)
{
    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || (!create && entry->present));

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    if (!entry->present)
    {
        LOGK("Get and create page table entry for 0x%p\n", vaddr);
        u32 page = get_page();
        // 初始化页目录项
        entry_init(entry, IDX(page));
        // 页表置空
        memset(table, 0, PAGE_SIZE);
    }
    return table;
}

// 刷新虚拟地址vaddr的快表 TLB
static void flush_tlb(u32 vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

// 从位图中扫描count个连续的页
static u32 scan_page(bitmap_t *map, u32 count)
{
    assert(count > 0);
    int32 index = bitmap_scan(map, count);

    if (index == EOF)
    {
        panic("Scan page fail!!!");
    }

    u32 addr = PAGE(index);
    LOGK("Scan page 0x%p count %d\n", addr, count);
    return addr;
}

// 与scan_page相对，重置对应的页
static void reset_page(bitmap_t *map, u32 addr, u32 count)
{
    ASSERT_PAGE(addr);
    assert(count > 0);
    u32 index = IDX(addr);
    for (size_t i = 0; i < count; i++)
    {
        assert(bitmap_test(map, index + i));
        bitmap_set(map, index + i, false);
    }
}

// 分配count个连续的内核页
u32 alloc_kpage(u32 count)
{
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    LOGK("ALLOC kernel pages 0x%p count %d\n", vaddr, count);
    return vaddr;
}

// 释放count个连续的内核页
void free_kpage(u32 vaddr, u32 count)
{
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
    LOGK("FREE kernel pages 0x%p count %d\n", vaddr, count);
}

void memory_test()
{
    // 测试物理内存分配
    // u32 pages[10];
    // for (size_t i = 0; i < 10; i++)
    // {
    //     pages[i] = get_page();
    // }

    // for (size_t i = 0; i < 10; i++)
    // {
    //     put_page(pages[i]);
    // }

    // BOCHS_MAGIC_BP;

    // 将 20 M 0x1400000 内存映射到 64M 0x4000000 的位置

    // 我们还需要一个页表，0x900000

    // u32 vaddr = 0x4000000; // 线性地址几乎可以是任意的
    // u32 paddr = 0x1400000; // 物理地址必须要确定存在
    // u32 table = 0x900000;  // 页表也必须是物理地址

    // page_entry_t *pde = get_pde();

    // page_entry_t *dentry = &pde[DIDX(vaddr)];
    // entry_init(dentry, IDX(table));

    // page_entry_t *pte = get_pte(vaddr);
    // page_entry_t *tentry = &pte[TIDX(vaddr)];

    // entry_init(tentry, IDX(paddr));

    // BOCHS_MAGIC_BP;

    // char *ptr = (char *)(0x4000000);
    // ptr[0] = 'a';

    // BOCHS_MAGIC_BP;

    // entry_init(tentry, IDX(0x1500000));
    // flush_tlb(vaddr);

    // BOCHS_MAGIC_BP;

    // ptr[2] = 'b';

    // BOCHS_MAGIC_BP;

    u32 *pages = (u32 *)(0x200000);
    u32 count = 0x6fe;
    for (size_t i = 0; i < count; i++)
    {
        pages[i] = alloc_kpage(1);
        LOGK("0x%x\n", i);
    }

    for (size_t i = 0; i < count; i++)
    {
        free_kpage(pages[i], 1);
    }
}

// 将vaddr映射物理内存
void link_page(u32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    // 如果页面已存在，直接返回
    if (entry->present)
    {
        assert(bitmap_test(map, index));
        return;
    }
    // 设置物理内存位图分配
    assert(!bitmap_test(map, index));
    bitmap_set(map, index, true);

    u32 paddr = get_page();
    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);

    LOGK("Link from 0x%p to 0x%p\n", vaddr, paddr);
}

// 去掉vaddr对应的物理内存映射
void unlink_page(u32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = get_pte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    // 如果页面不存在，直接返回
    if (!entry->present)
    {
        assert(!bitmap_test(map, index));
        return;
    }
    assert(entry->present && bitmap_test(map, index));

    entry->present = false;
    bitmap_set(map, index, false);

    u32 paddr = PAGE(entry->index);
    LOGK("Unlink from 0x%p to 0x%p\n", vaddr, paddr);

    put_page(paddr);

    flush_tlb(vaddr);
}

// 拷贝一页，返回物理地址
static u32 copy_page(void *page)
{
    u32 paddr = get_page();

    // 利用0这一项页表没有被初始化，拷贝数据后
    page_entry_t *entry = get_pte(0, false);
    entry_init(entry, IDX(paddr));
    // 访问0对应的虚拟地址，也就是paddr，将page的一页数据拷贝到paddr上
    memcpy((void *)0, (void *)page, PAGE_SIZE);

    // 将这一页表项还原回去
    entry->present = false;
    return paddr;
}

// 拷贝pde
page_entry_t *copy_pde()
{
    task_t *task = running_task();
    page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
    memcpy(pde, (void *)task->pde, PAGE_SIZE);

    // 将最后一个页目录指向自己
    page_entry_t *entry = &pde[1023];
    // 这里由于从内核分配，物理地址和线性地址相同所以使用pde的idx
    entry_init(entry, IDX(pde));

    page_entry_t *dentry;

    // 0 和1 是内核态的8M内存，下面拷贝的是用户态的页目录
    for (size_t didx = 2; didx < 1023; didx++)
    {
        dentry = &pde[didx];
        if (!dentry->present)
        {
            continue;
        }
        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            entry = &pte[tidx];
            if (!entry->present)
            {
                continue;
            }
            
            // 对应的物理内存引用大于0
            assert(memory_map[entry->index] > 0);
            // 置为只读
            entry->write = false;
            // 对应的物理页引用加1
            memory_map[entry->index]++;

            assert(memory_map[entry->index] < 255);
        }
        
        u32 paddr = copy_page((pte));
        dentry->index = IDX(paddr);
    }
    
    set_cr3(task->pde);

    return pde;
}

// 释放页目录
void free_pde()
{
    task_t *task = running_task();

    assert(task->uid != KERNEL_USER);

    page_entry_t *pde = get_pde();

    for (size_t didx = 2; didx < 1023; didx++)
    {
        page_entry_t *dentry = &pde[didx];
        if (!dentry->present)
        {
            continue;
        }
        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            page_entry_t *entry = &pte[tidx];
            if (!entry->present)
            {
                continue;
            }

            // 对应的物理内存引用大于0
            assert(memory_map[entry->index] > 0);
            put_page(PAGE(entry->index));
        }

        // 释放页表
        put_page(PAGE(dentry->index));
    }
    free_kpage(task->pde, 1);
    LOGK("free pages %d\n", free_pages);
}

// 缺页错误编码（缺页中断会设置32位错误码）
typedef struct page_error_code_t
{
    u8 present : 1;    // 由内存不存在导致
    u8 write : 1;      // 由写操作导致
    u8 user : 1;       // 用户或系统操作导致
    u8 reserved0 : 1;  // 保留
    u8 fetch : 1;      // 指令提取导致
    u8 protection : 1; // 保护key导致
    u8 shadow : 1;     // 隐藏stack访问
    u16 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
} _packed page_error_code_t;

// 系统调用brk
int32 sys_brk(void *addr)
{
    LOGK("task brk 0x%p\n", addr);
    u32 brk = (u32)addr;

    ASSERT_PAGE(brk);

    task_t *task = running_task();

    assert(task->uid != KERNEL_USER);

    assert(KERNEL_MEMORY_SIZE < brk < USER_STACK_BOTTOM);

    u32 old_brk = task->brk;

    if (old_brk > brk)
    {
        // 需要释放内存
        for(; brk < old_brk; brk += PAGE_SIZE)
        {
            unlink_page(brk);
        }
    }
    else if (IDX(brk - old_brk) > free_pages)
    {
        // out of memory
        return -1;
    }
    task->brk = brk;
    
    return 0;
}

// 缺页中断处理
void page_fault(
    int vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);
    // 导致缺页异常的地址从cr2寄存器中获取
    u32 vaddr = get_cr2();

    LOGK("fault address 0x%p...\n", vaddr);

    // 转换错误码
    page_error_code_t *code = (page_error_code_t *)&error;

    // 获取当前执行的任务
    task_t *task = running_task();

    assert(KERNEL_MEMORY_SIZE <= vaddr < USER_STACK_TOP);

    // 用户只读内存(写时复制)
    if (code->present)
    {
        // 由于写内存导致的缺页异常
        assert(code->write);

        page_entry_t *pte = get_pte(vaddr, false);
        page_entry_t *entry = &pte[TIDX(vaddr)];

        assert(entry->present);
        assert(memory_map[entry->index] > 0);
        if (memory_map[entry->index] == 1)
        {
            entry->write = true;
            LOGK("WRITE page for 0x%p\n", vaddr);
        }
        else
        {
            void *page = (void *)PAGE(IDX(vaddr));
            u32 paddr = copy_page(page);
            memory_map[entry->index]--;
            entry_init(entry, IDX(paddr));
            flush_tlb(vaddr);
            LOGK("COPY page for 0x%p\n", vaddr);
        }
        return;
    }
    

    // 分配用户栈或堆内存
    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM))
    {
        // 获取页的开始地址
        u32 page = PAGE(IDX(vaddr));
        // 建立物理内存和虚拟地址的映射
        link_page(page);
        // BOCHS_MAGIC_BP;
        return;
    }
    panic("page fault!!!");
}