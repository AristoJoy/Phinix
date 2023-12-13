#include <phinix/ide.h>
#include <phinix/io.h>
#include <phinix/printk.h>
#include <phinix/stdio.h>
#include <phinix/memory.h>
#include <phinix/interrupt.h>
#include <phinix/task.h>
#include <phinix/string.h>
#include <phinix/assert.h>
#include <phinix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器地址偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERROR 0x0001      // 错误寄存器
#define IDE_FEATURE 0x0001    // 特性寄存器
#define IDE_SECTOR 0x0002     // 扇区数量寄存器
#define IDE_LBA_LOW 0x0003    // LBA 0~7
#define IDE_LBA_MID 0x0004    // LBA 8~15
#define IDE_LBA_HIGH 0x0005   // LBA 16 ~ 23
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令
#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xEC // IDENTIFY命令

// IDE 控制器状态寄存器偏移
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected Error
#define IDE_SR_DRQ 0x08  // Data Request
#define IDE_SR_DSC 0x10  // Drive seek Complete
#define IDE_SR_DWF 0x20  // Drive Write Fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head(not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change request
#define IDE_ER_IDNF 0x10  // Sector ID not found
#define IDE_ER_MC 0x20    // Media changed
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad Block

#define IDE_LBA_MASTER 0b11100000 // LBA 主盘
#define IDE_LBA_SLAVE 0b11110000  // LBA 从盘

ide_ctrl_t controllers[IDE_CTRL_NR];

// ide 中断处理程序
void ide_handler(int vector)
{
    send_eoi(vector); // 向中断控制器发送中断处理完成

    // 得到中断向量对应的控制器
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];
    // 读取常规状态寄存器（会破坏IRQ），表示中断处理结束
    u8 state = in_byte(ctrl->iobase + IDE_STATUS);
    LOGK("hard disk interrupt vector %d state 0x%x\n", vector, state);
    if (ctrl->waiter)
    {   
        task_unblock(ctrl->waiter);
        ctrl->waiter = NULL;
    }
}

static u32 ide_error(ide_ctrl_t *ctrl)
{
    u8 error = in_byte(ctrl->iobase + IDE_ERROR);
    if (error & IDE_ER_BBK)
    {
        LOGK("bad block\n");
    }
    if (error & IDE_ER_UNC)
    {
        LOGK("Uncorrectable data error\n");
    }
    if (error & IDE_ER_MC)
    {
        LOGK("Media changed\n");
    }
    if (error & IDE_ER_IDNF)
    {
        LOGK("ID not found\n");
    }
    if (error & IDE_ER_MCR)
    {
        LOGK("Media change request\n");
    }
    if (error & IDE_ER_ABRT)
    {
        LOGK("Aborted command\n");
    }
    if (error & IDE_ER_TK0NF)
    {
        LOGK("Track zero not found\n");
    }
    if (error & IDE_ER_AMNF)
    {
        LOGK("Address mark not found\n");
    }
}

static u32 ide_busy_wait(ide_ctrl_t *ctrl, u8 mask)
{
    while (true)
    {
        u8 status = in_byte(ctrl->iobase + IDE_ALT_STATUS);
        if (status & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
        }
        if (status & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((status & mask) == mask) // 等待的状态完成
        {
            return 0;
        }
    }
}

// 选择磁盘
static void ide_select_drive(ide_disk_t *disk)
{
    out_byte(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

// 选择扇区
static void ide_select_sector(ide_disk_t *disk, u32 lba, u8 count)
{
    // 输出功能，可忽略
    out_byte(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 设置扇区数量
    out_byte(disk->ctrl->iobase + IDE_SECTOR, count);

    // 设置LBA 低8位
    out_byte(disk->ctrl->iobase + IDE_LBA_LOW, (lba & 0xff));

    // 设置LBA 中8位
    out_byte(disk->ctrl->iobase + IDE_LBA_MID, ((lba >> 8) & 0xff));

    // 设置LBA 高8位
    out_byte(disk->ctrl->iobase + IDE_LBA_HIGH, ((lba >> 16) & 0xff));

    // LBA 最高4位和磁盘选择
    out_byte(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);

    disk->ctrl->active = disk;
}

// 从磁盘读取一个扇区到buf
void ide_pio_read_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        buf[i] = in_word(disk->ctrl->iobase + IDE_DATA);
    }
}

// 从buf写入一个扇区到磁盘
void ide_pio_write_sector(ide_disk_t *disk, u16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        out_word(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

// pio 读磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba)
{
    assert(count > 0);
    assert(!get_interrupt_state()); // 不允许中断，否则会永久阻塞线程

    ide_ctrl_t *ctrl = disk->ctrl;

    lock_acquire(&ctrl->lock);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择磁盘扇区
    ide_select_sector(disk, lba, count);

    // 发送读命令
    out_byte(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        task_t *task = running_task();
        if (task->state = TASK_RUNNING)
        {
            // 阻塞自己等待中断到来，等待磁盘准备数据
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }
        
        ide_busy_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *) offset);
    }
    

    lock_release(&ctrl->lock);

    return 0;
}

// pio 写磁盘
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba)
{
    assert(count > 0);
    assert(!get_interrupt_state()); // 不允许中断，否则会永久阻塞线程

    ide_ctrl_t *ctrl = disk->ctrl;

    lock_acquire(&ctrl->lock);

    LOGK("write lba 0x%x\n", lba);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择磁盘扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    out_byte(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (u16 *)offset);

        task_t *task = running_task();
        if (task->state = TASK_RUNNING)
        {
            // 阻塞自己等待中断到来，等待磁盘写数据
            ctrl->waiter = task;
            task_block(task, NULL, TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_NULL);
    }

    lock_release(&ctrl->lock);

    return 0;
}

// 磁盘控制器初始化
static void ide_ctrl_init()
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;

        if (cidx)   // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else        // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }

        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
            disk->ctrl = ctrl;
            if (didx) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else    // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
        }
    }
}

// 控制器初始化
void ide_init()
{
    LOGK("ide init...\n");
    ide_ctrl_init();

    // 注册硬盘中断处理函数，打开屏蔽字
    set_interrupt_handler(IRQ_HARDDISK, ide_handler);
    set_interrupt_handler(IRQ_HARDDISK2, ide_handler);
    set_interrupt_mask(IRQ_HARDDISK, true);
    set_interrupt_mask(IRQ_HARDDISK2, true);
    set_interrupt_mask(IRQ_CASCADE, true);

}