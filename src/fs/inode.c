#include <phinix/fs.h>
#include <phinix/syscall.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/buffer.h>
#include <phinix/arena.h>
#include <phinix/string.h>
#include <phinix/stdlib.h>
#include <phinix/stat.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define INODE_NR 64

static inode_t inode_table[INODE_NR];

// 申请一个inode
static inode_t *get_free_inode()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        if (inode->dev == EOF)
        {
            return inode;
        }
    }
    panic("no more inode!!!");
}

// 释放一个inode
static void put_free_inode(inode_t *inode)
{
    assert(inode != inode_table);
    assert(inode->count == 0);
    inode->dev = EOF;
}

// 获取根目录inode
inode_t *get_root_inode()
{
    return inode_table;
}

// 计算inode nr对应的块号
static inline idx_t inode_block(super_block_t *sb, idx_t nr)
{
    // inode 编号从1开始
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + (nr - 1) / BLOCK_INODES;
}

// 从已有inode中查找编号为nr的inode
static inode_t *find_inode(dev_t dev, idx_t nr)
{
    super_block_t *sb = get_super(dev);
    assert(sb);
    list_t *list = &sb->inode_list;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        inode_t *inode = element_entry(inode_t, node, node);
        if (inode->nr == nr)
        {
            return inode;
        }
    }
    return NULL;
}

// 获取设备dev的nr inode
inode_t *iget(dev_t dev, idx_t nr)
{
    inode_t *inode = find_inode(dev, nr);
    if (inode)
    {
        inode->count++;
        inode->atime = time();
        return inode;
    }

    super_block_t *sb = get_super(dev);
    assert(sb);

    assert(nr <= sb->desc->inodes);

    inode = get_free_inode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count = 1;

    // 加入超级块的inode链表
    list_push(&sb->inode_list, &inode->node);

    idx_t block = inode_block(sb, inode->nr);
    buffer_t *buf = bread(inode->dev, block);

    inode->buf = buf;

    // 将一个缓冲视为一个inode描述符数据
    inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = inode->desc->mtime;
    inode->atime = time();

    return inode;
}

// 释放inode
void iput(inode_t *inode)
{
    if (!inode)
    {
        return;
    }

    // todo need write ?
    if (inode->buf->dirty)
    {
        bwrite(inode->buf);
    }

    inode->count--;

    if (inode->count)
    {
        return;
    }

    // 释放inode对应的缓冲
    brelse(inode->buf);

    // 从超级块链表中移除
    list_remove(&inode->node);

    // 释放inode内存
    put_free_inode(inode);
}

// 从 inode 的 offset 处，读 len 个字节到 buf
int inode_read(inode_t *inode, char *buf, u32 len, off_t offset)
{
    assert(ISFILE(inode->desc->mode) || ISDIR(inode->desc->mode));

    // 如果偏移量超过文件大小，返回EOF
    if (offset >= inode->desc->size)
    {
        return EOF;
    }
    // 开始读取的位置
    u32 begin = offset;

    // 剩余字节数
    u32 left = MIN(len, inode->desc->size - offset);
    while (left)
    {
        // 找到对应的文件偏移，所在文件块
        idx_t nr = bmap(inode, offset / BLOCK_SIZE, false);
        assert(nr);

        // 读取文件块缓冲
        buffer_t *bf = bread(inode->dev, nr);

        // 文件块中的偏移量
        u32 start = offset % BLOCK_SIZE;

        // 本次需要读取的字节数
        u32 chars = MIN(BLOCK_SIZE - start, left);

        // 更新偏移量和剩余字节数
        offset += chars;
        left -= chars;

        // 文件块中的指针
        char *ptr = bf->data + start;

        // 拷贝内容
        memcpy(buf, ptr, chars);

        // 更新缓存位置
        buf += chars;

        // 释放文件块缓冲
        brelse(bf);
    }

    // 更新访问时间
    inode->atime = time();

    // 返回读取的数量
    return offset - begin;
}

// 从 inode 的 offset 处，将 buf 的 len 个字节写入磁盘
int inode_write(inode_t *inode, char *buf, u32 len, off_t offset)
{
    assert(ISFILE(inode->desc->mode));

    // 开始的位置
    u32 begin = offset;

    // 剩余数量
    u32 left = len;

    while (left)
    {
        // 找到文件块，如果不存在则创建
        idx_t nr = bmap(inode, offset / BLOCK_SIZE, true);
        assert(nr);

        // 读取文件块
        buffer_t *bf = bread(inode->dev, nr);
        bf->dirty = true;

        // 块中的偏移
        u32 start = offset % BLOCK_SIZE;
        // 文件块中的指针
        char *ptr = bf->data + start;

        // 读取的数量
        u32 chars = MIN(BLOCK_SIZE - start, left);

        // 更新偏移量
        offset += chars;

        // 更新剩余字节数
        left-= chars;

        // 如果偏移量大于文件大小，这更新
        if (offset > inode->desc->size)
        {
            inode->desc->size = offset;
            inode->buf->dirty = true;
        }
        
        // 拷贝内容
        memcpy(ptr, buf, chars);

        // 更新缓存偏移
        buf += chars;

        // 释放文件块
        brelse(bf);
    }
    
    // 更新修改时间
    inode->desc->mtime = time();
    inode->atime = inode->desc->mtime;

    // todo 写入磁盘 ？
    bwrite(inode->buf);

    // 返回写大小
    return offset - begin;
}

void inode_init()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inode_table[i];
        inode->dev = EOF;
    }
}