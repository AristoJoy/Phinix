#ifndef PHINIX_FS_H
#define PHINIX_FS_H

#include <phinix/types.h>
#include <phinix/list.h>
#include <phinix/buffer.h>

#define BLOCK_SIZE 1024 // 块大小
#define SECTOR_SIZE 512 // 扇区大小

#define MINIX1_MAGIC 0x137f // 文件系统魔数
#define NAME_LEN 14         // 文件名长度

#define IMAP_NR 8 // inode 位图块，最大值
#define ZMAP_NR 8 // 块位图块，最大值

#define BLOCK_BITS (BLOCK_SIZE * 8) // 块位图大小

// inode描述符
typedef struct inode_desc_t
{
    u16 mode;    // 文件类型和属性 (rwx)
    u16 uid;     // 用户id（文件拥有者标识符）
    u32 size;    // 文件大小（字节数）
    u32 mtime;   // 修改时间戳
    u8 gid;      // 组id (文件拥有者所在组)
    u8 nlinks;   // 连接数（多少个文件目录项指向该结点）
    u16 zone[9]; // 直接块(0-6)、间接块（7）和双重间接块（8）
} inode_desc_t;

// inode 内存
typedef struct inode_t
{
    inode_desc_t *desc;   // inode 描述符
    struct buffer_t *buf; // inode描述符对应 buffer
    dev_t dev;            // 设备号
    idx_t nr;             // i节点号
    u32 count;            // 引用计数
    time_t atime;         // 访问时间
    time_t ctime;         // 创建时间
    list_node_t node;     // 链表结点
    dev_t mount;          // 安装设备
} inode_t;

// 超级快
typedef struct super_desc_t
{
    u16 inodes;         // 节点数
    u16 zones;          // 逻辑块数
    u16 imap_blocks;    // i节点位图所占用的数据块数
    u16 zmap_blocks;    // 逻辑块位图所占用的数据块数
    u16 first_datazone; // 第一个数据逻辑块号
    u16 log_zone_size;  // log2(每逻辑块数据块)
    u32 max_size;       // 文件最大长度
    u16 magic;          // 文件系统魔数
} super_desc_t;

// 超级块 内存
typedef struct super_block_t
{
    super_desc_t *desc;              // 超级快描述符
    struct buffer_t *buf;            // 超级块描述符 Buffer
    struct buffer_t *imaps[IMAP_NR]; // inode位图缓冲
    struct buffer_t *zmaps[ZMAP_NR]; // 块位图缓冲
    dev_t dev;                       // 设备号
    list_t inode_list;               // 使用中 inode链表
    inode_t *iroot;                  // 根目录 inode
    inode_t *imount;                 // 安装到的 inode
} super_block_t;

// 文件目录项结构
typedef struct dentry_t
{
    u16 nr;              // i节点
    char name[NAME_LEN]; // 文件名
} dentry_t;

super_block_t *get_super(dev_t dev);  // 获取dev对应的超级块
super_block_t *read_super(dev_t dev); // 读取dev对应的超级块

idx_t balloc(dev_t dev);          // 分配一个文件块
void bfree(dev_t dev, idx_t idx); // 释放一个文件块
idx_t ialloc(dev_t dev);          // 分配一个文件系统 inode
void ifree(dev_t dev, idx_t idx); // 释放一个文件系统 inode

#endif