#ifndef PHINIX_IDE_H
#define PHINIX_IDE_H

#include <phinix/types.h>
#include <phinix/mutex.h>

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2  // 控制器数量，固定为2
#define IDE_DISK_NR 2  // 每个控制器可挂磁盘数量，固定为2

// IDE 磁盘
typedef struct ide_disk_t
{
    char name[8];   // 磁盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;            // 磁盘选择
    bool master;            // 主盘
    u32 total_lba;          // 可用扇区数量
    u32 cylinders;          // 柱面数
    u32 heads;              // 磁头数
    u32 sectors;            // 扇区数
} ide_disk_t;       

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];  // 控制器名称
    lock_t lock;    // 控制器锁
    u16 iobase;     // IO寄存器基地址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *active;     // 当前选择的磁盘
    u8 control;             // 控制字节
    struct task_t *waiter;  // 等待控制器的进程
} ide_ctrl_t;

// pio 读磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

// pio 写磁盘
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif