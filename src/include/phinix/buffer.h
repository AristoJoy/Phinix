#ifndef PHINIX_BUFFER_H
#define PHINIX_BUFFER_H

#include <phinix/types.h>
#include <phinix/list.h>
#include <phinix/mutex.h>

#define BLOCK_SIZE 1024                       // 块大小
#define SECTOR_SIZE 512                       // 扇区大小
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE) // 一个块占2个扇区

// buffer
typedef struct buffer_t
{
    char *data;        // 数据区
    dev_t dev;         // 设备号
    idx_t block;       // 块号
    int count;         // 引用计数
    list_node_t hnode; // 哈希表拉链节点
    list_node_t rnode; // 缓冲节点
    lock_t lock;       // 锁
    bool dirty;        // 释放与磁盘不一致
    bool valid;        // 是有有效
} buffer_t;

buffer_t *getblk(dev_t dev, idx_t block);
buffer_t *bread(dev_t dev, idx_t block);
void bwrite(buffer_t *bf);
void brelse(buffer_t *bf);

#endif