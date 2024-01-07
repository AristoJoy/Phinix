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
#include <phinix/device.h>
#include <phinix/errno.h>

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

// 分区文件系统
// 参考 https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
typedef enum PART_FS
{
    PART_FS_FAT12 = 1,    // FAT12
    PART_FS_EXTENDED = 5, // 扩展分区
    PART_FS_MINIX = 0x80, // minux
    PART_FS_LINUX = 0x83, // linux
} PART_FS;

typedef struct ide_params_t
{
    u16 config;                 // 0 General configuration bits
    u16 cylinders;              // 01 cylinders
    u16 RESERVED;               // 02
    u16 heads;                  // 03 heads
    u16 RESERVED[5 - 3];        // 05
    u16 sectors;                // 06 sectors per track
    u16 RESERVED[9 - 6];        // 09
    u8 serial[20];              // 10 ~ 19 序列号
    u16 RESERVED[22 - 19];      // 10 ~ 22
    u8 firmware[8];             // 23 ~ 26 固件版本
    u8 model[40];               // 27 ~ 46 模型数
    u8 drq_sectors;             // 47 扇区数量
    u8 RESERVED[3];             // 48
    u16 capabilities;           // 49 能力
    u16 RESERVED[59 - 49];      // 50 ~ 59
    u32 total_lba;              // 60 ~ 61
    u16 RESERVED;               // 62
    u16 mdma_mode;              // 63
    u8 RESERVED;                // 64
    u8 pio_mode;                // 64
    u16 RESERVED[79 - 64];      // 65 ~ 79 参见 ATA specification
    u16 major_version;          // 80 主版本
    u16 minor_version;          // 81 副版本
    u16 commmand_sets[87 - 81]; // 82 ~ 87 支持的命令集
    u16 RESERVED[118 - 87];     // 88 ~ 118
    u16 support_settings;       // 119
    u16 enable_settings;        // 120
    u16 RESERVED[221 - 120];    // 221
    u16 transport_major;        // 222
    u16 transport_minor;        // 223
    u16 RESERVED[254 - 223];    // 254
    u16 integrity;              // 校验和
} _packed ide_params_t;

ide_ctrl_t controllers[IDE_CTRL_NR];

// ide 中断处理程序
static void ide_handler(int vector)
{
    send_eoi(vector); // 向中断控制器发送中断处理完成

    // 得到中断向量对应的控制器
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];
    // 读取常规状态寄存器（会破坏IRQ），表示中断处理结束
    u8 state = in_byte(ctrl->iobase + IDE_STATUS);
    LOGK("hard disk interrupt vector %d state 0x%x\n", vector, state);
    if (ctrl->waiter)
    {
        task_unblock(ctrl->waiter, EOK);
        ctrl->waiter = NULL;
    }
}

static void ide_error(ide_ctrl_t *ctrl)
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
    // todo timeout, reset controller when error
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

// 重置硬盘控制器
void ide_reset_controller(ide_ctrl_t *ctrl)
{
    out_byte(ctrl->iobase + IDE_CONTROL, IDE_CTRL_SRST);
    ide_busy_wait(ctrl, IDE_SR_NULL);
    out_byte(ctrl->iobase + IDE_CONTROL, ctrl->control);
    ide_busy_wait(ctrl, IDE_SR_NULL);
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

// ide磁盘设备控制
int ide_pio_ioctl(ide_disk_t *disk, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return 0;
    case DEV_CMD_SECTOR_COUNT:
        return disk->total_lba;

    default:
        panic("device recommand %d can't recognize!!!", cmd);
        break;
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

        // 阻塞自己等待中断到来，等待磁盘准备数据
        ctrl->waiter = task;
        assert(task_block(task, NULL, TASK_BLOCKED, TIMELESS) == EOK);

        ide_busy_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *)offset);
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

        // 阻塞自己等待中断到来，等待磁盘写数据
        ctrl->waiter = task;
        assert(task_block(task, NULL, TASK_BLOCKED, TIMELESS) == EOK);

        ide_busy_wait(ctrl, IDE_SR_NULL);
    }

    lock_release(&ctrl->lock);

    return 0;
}

// ide磁盘分区设备控制
int ide_pio_part_ioctl(ide_part_t *part, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return part->start;
    case DEV_CMD_SECTOR_COUNT:
        return part->count;

    default:
        panic("device recommand %d can't recognize!!!", cmd);
        break;
    }
}

// 读分区
int ide_pio_part_read(ide_part_t *part, void *buf, u8 count, idx_t lba)
{
    return ide_pio_read(part->disk, buf, count, part->start + lba);
}

// 写分区
int ide_pio_part_write(ide_part_t *part, void *buf, u8 count, idx_t lba)
{
    return ide_pio_write(part->disk, buf, count, part->start + lba);
}

static void ide_swap_pairs(char *buf, u32 len)
{
    for (size_t i = 0; i < len; i += 2)
    {
        register char ch = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = ch;
    }
    buf[len - 1] = '\0';
}

// 识别磁盘
static void ide_part_init(ide_disk_t *disk, u16 *buf)
{
    // 磁盘不可用
    if (!disk->total_lba)
    {
        return;
    }

    // 读取注意到扇区
    ide_pio_read(disk, buf, 1, 0);

    // 初始化主引导扇区
    boot_sector_t *boot = (boot_sector_t *)buf;

    for (size_t i = 0; i < IDE_PART_NR; i++)
    {
        part_entry_t *entry = &boot->entry[i];
        ide_part_t *part = &disk->parts[i];
        if (!entry->count)
        {
            continue;
        }

        sprintf(part->name, "%s%d", disk->name, i + 1);

        LOGK("part %s \n", part->name);
        LOGK("    bootable %d\n", entry->bootable);
        LOGK("    start %d\n", entry->start);
        LOGK("    count %d\n", entry->count);
        LOGK("    system %d\n", entry->system);

        part->disk = disk;
        part->count = entry->count;
        part->system = entry->system;
        part->start = entry->start;

        if (entry->system == PART_FS_EXTENDED)
        {
            LOGK("Unsupported extended partition!!!\n");

            boot_sector_t *eboot = (boot_sector_t *)(buf + SECTOR_SIZE);
            ide_pio_read(disk, (void *)eboot, 1, entry->start);

            for (size_t j = 0; j < IDE_PART_NR; j++)
            {
                part_entry_t *eentry = &eboot->entry[j];
                if (!eentry->count)
                {
                    continue;
                }

                LOGK("part %d  extend %d\n", i, j);
                LOGK("    bootable %d\n", eentry->bootable);
                LOGK("    start %d\n", eentry->start);
                LOGK("    count %d\n", eentry->count);
                LOGK("    system %d\n", eentry->system);
            }
        }
    }
}

// 识别磁盘
static u32 ide_identify(ide_disk_t *disk, u16 *buf)
{
    LOGK("identifing disk %s...\n", disk->name);
    lock_acquire(&disk->ctrl->lock);

    ide_select_drive(disk);

    // 向磁盘发送识别命令
    out_byte(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_IDENTIFY);

    ide_busy_wait(disk->ctrl, IDE_SR_NULL);

    ide_params_t *params = (ide_params_t *)buf;

    ide_pio_read_sector(disk, buf);

    LOGK("disk %s total lba %d\n", disk->name, params->total_lba);

    u32 ret = EOF;
    if (params->total_lba == 0)
    {
        goto rollback;
    }

    // 简单兼容VMware，目前没有找到好的办法
    if (params->total_lba > (1 << 28))
    {
        params->total_lba = 0;
        goto rollback;
    }

    ide_swap_pairs(params->serial, sizeof(params->serial));
    LOGK("disk %s serial number %s\n", disk->name, params->serial);

    ide_swap_pairs(params->firmware, sizeof(params->firmware));
    LOGK("disk %s firmware version %s\n", disk->name, params->firmware);

    ide_swap_pairs(params->model, sizeof(params->model));
    LOGK("disk %s model number %s\n", disk->name, params->model);

    disk->total_lba = params->total_lba;
    disk->cylinders = params->cylinders;
    disk->heads = params->heads;
    disk->sectors = params->sectors;
    ret = 0;

rollback:
    lock_release(&disk->ctrl->lock);
    return ret;
}

// 磁盘控制器初始化
static void ide_ctrl_init()
{
    u16 *buf = (u16 *)alloc_kpage(1);
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;
        ctrl->waiter = NULL;

        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }

        // 读控制字节
        ctrl->control = in_byte(ctrl->iobase + IDE_CONTROL);

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
            else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
            ide_identify(disk, buf);
            ide_part_init(disk, buf);
        }
    }
    free_kpage((u32)buf, 1);
}

// 注册磁盘设备
static void ide_install()
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            if (!disk->total_lba)
            {
                continue;
            }
            dev_t dev = device_install(DEV_BLOCK, DEV_IDE_DISK, disk, disk->name, 0, ide_pio_ioctl, ide_pio_read, ide_pio_write);
            for (size_t i = 0; i < IDE_PART_NR; i++)
            {
                ide_part_t *part = &disk->parts[i];
                if (!part->count)
                {
                    continue;
                }
                device_install(DEV_BLOCK, DEV_IDE_PART, part, part->name, dev, ide_pio_part_ioctl, ide_pio_part_read, ide_pio_part_write);
            }
        }
    }
}

// 控制器初始化
void ide_init()
{
    LOGK("ide init...\n");


    // 注册硬盘中断处理函数，打开屏蔽字
    set_interrupt_handler(IRQ_HARDDISK, ide_handler);
    set_interrupt_handler(IRQ_HARDDISK2, ide_handler);
    set_interrupt_mask(IRQ_HARDDISK, true);
    set_interrupt_mask(IRQ_HARDDISK2, true);
    set_interrupt_mask(IRQ_CASCADE, true);

    ide_ctrl_init();

    ide_install(); // 安装设备
}