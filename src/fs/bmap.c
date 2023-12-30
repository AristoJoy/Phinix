#include <phinix/fs.h>
#include <phinix/debug.h>
#include <phinix/bitmap.h>
#include <phinix/assert.h>
#include <phinix/string.h>
#include <phinix/buffer.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// 分配一个文件块
idx_t balloc(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    idx_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        buf = sb->zmaps[i];
        assert(buf);

        // 将整个缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS + sb->desc->first_datazone - 1);

        // 从位图中扫描一位
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {
            // 如果扫描成功， 这标记缓冲区脏，中止查找
            assert(bit < sb->desc->zones);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf); // todo 调试期间强同步
    return bit;
}

// 释放一个文件块
void bfree(dev_t dev, idx_t idx)
{
    super_block_t *sb = get_super(dev);
    assert(sb != NULL);
    assert(idx < sb->desc->zones);

    buffer_t *buf;
    bitmap_t map;
    for (size_t i = 0; i < ZMAP_NR; i++)
    {
        // 跳过开始的块
        if (idx > BLOCK_BITS * (i + 1))
        {
            continue;
        }
        buf = sb->zmaps[i];
        assert(buf);

        // 将整个缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS + sb->desc->first_datazone - 1);

        // 将idx对应的位图置为0
        assert(bitmap_test(&map, idx));
        bitmap_set(&map, idx, 0);

        // 标记缓冲区脏
        buf->dirty = true;
        break;
    }
    bwrite(buf); // todo 调试期间强同步
}
// 分配一个文件系统 inode
idx_t ialloc(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    assert(sb);

    buffer_t *buf = NULL;
    idx_t bit = EOF;
    bitmap_t map;

    for (size_t i = 0; i < IMAP_NR; i++)
    {
        buf = sb->imaps[i];
        assert(buf);

        // 将整个缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_BITS, i * BLOCK_BITS);

        // 从位图中扫描一位
        bit = bitmap_scan(&map, 1);
        if (bit != EOF)
        {
            assert(bit < sb->desc->inodes);
            buf->dirty = true;
            break;
        }
    }
    bwrite(buf); // todo 调试期间强同步
    return bit;
}
// 释放一个文件系统 inode
void ifree(dev_t dev, idx_t idx)
{
    super_block_t *sb = get_super(dev);
    assert(sb != NULL);
    assert(idx < sb->desc->zones);

    buffer_t *buf;
    bitmap_t map;
    for (size_t i = 0; i < IMAP_NR; i++)
    {
        // 跳过开始的块
        if (idx > BLOCK_BITS * (i + 1))
        {
            continue;
        }
        buf = sb->imaps[i];
        assert(buf);

        // 将整个缓冲区作为位图
        bitmap_make(&map, buf->data, BLOCK_SIZE, i * BLOCK_BITS);

        // 将idx对应的位图置为0
        assert(bitmap_test(&map, idx));
        bitmap_set(&map, idx, 0);

        // 标记缓冲区脏
        buf->dirty = true;
        break;
    }
    bwrite(buf); // todo 调试期间强同步
}

// 获取inode第block块的索引值
// 如果不存在且create为true，则创建
idx_t bmap(inode_t *inode, idx_t block, bool create)
{
    // 确保block合法
    assert(block >= 0 && block < TOTAL_BLOCK);

    // 数组索引
    u16 index = block;

    // 数组
    u16 *array = inode->desc->zone;

    // 缓冲区
    buffer_t *buf = inode->buf;

    // 用于下面的brelse，传入参数inode的buf不应该释放
    buf->count += 1;

    // 当前处理级别
    int level = 0;

    // 当前子级别块数量
    int divider = 1;

    // 直接块不做处理

    if (DIRECT_BLOCK <= block && block < INDIRECT1_BLOCK)
    {
        block -= DIRECT_BLOCK;
        index = DIRECT_BLOCK;
        level = 1;
        divider = 1;
    }
    else if (block >= INDIRECT1_BLOCK)
    {
        block -= DIRECT_BLOCK + INDIRECT1_BLOCK;
        assert(block < INDIRECT2_BLOCK);
        index = DIRECT_BLOCK + 1;
        level = 2;
        divider = BLOCK_INDEXES;
    }

    for (; level >= 0; level--)
    {
        // 如果不存在且create则申请一个文件块
        if (!array[index] && create)
        {
            array[index] = balloc(inode->dev);
            buf->dirty = true;
        }
        brelse(buf);

        // 如果level为0，或者索引不存在，直接返回
        if (level == 0 || !array[index])
        {
            return array[index];
        }
        // 如果level不为0，处理下一级索引
        buf = bread(inode->dev, array[index]);
        index = block / divider;
        block = block % divider;
        divider /= BLOCK_INDEXES;
        array = (u16 *)buf->data;
    }
}
