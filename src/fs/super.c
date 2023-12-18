#include <phinix/fs.h>
#include <phinix/buffer.h>
#include <phinix/device.h>
#include <phinix/assert.h>
#include <phinix/string.h>
#include <phinix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define SUPER_NR 16

static super_block_t super_table[SUPER_NR]; // 超级块表
static super_block_t *root;                 // 根文件系统超级块

// 从超级块表中查找一个空闲块
static super_block_t *get_free_super()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == EOF)
        {
            return sb;
        }
    }
    panic("no more super block!!!");
}

// 获取dev对应的超级块
super_block_t *get_super(dev_t dev)
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        if (sb->dev == dev)
        {
            return sb;
        }
        
    }
    return NULL;
    
}
// 读取dev对应的超级块
super_block_t *read_super(dev_t dev)
{
    super_block_t *sb = get_super(dev);
    if (sb)
    {
        return sb;
    }
    
    LOGK("Reading super block of device %d\n", dev);

    // 获取空闲超级块
    sb = get_free_super();

    // 读取超级块
    buffer_t *buf = bread(dev, 1);

    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINIX1_MAGIC);

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    // 读取inode 位图
    int idx = 2; // 块位图从第二个块开始，第0块引导块， 第1块超级块

    for (size_t i = 0; i < sb->desc->imap_blocks; i++)
    {
        assert(i < IMAP_NR);
        if ((sb->imaps[i] = bread(dev, idx)))
        {
            idx++;
        }
        else
        {
            break;
        }
        
    }

    for (size_t i = 0; i < sb->desc->zmap_blocks; i++)
    {
        assert(i < ZMAP_NR);
        if ((sb->zmaps[i] = bread(dev, idx)))
        {
            idx++;
        }
        else
        {
            break;
        }
    }
    return sb;
}

// 挂载根文件系统
static void mount_root()
{
    LOGK("Mount root file system...\n");
    // 假设第一个分区时根文件系统
    device_t *device = device_find(DEV_IDE_PART, 0);
    assert(device);

    // 读根文件系统超级块
    root = read_super(device->dev);

    root->iroot = iget(device->dev, 1);// 获取根目录inode
    root->imount = iget(device->dev, 1); // 根目录挂载inode

    idx_t idx =0;
    inode_t *inode = iget(device->dev, 1);

    // 直接块
    idx = bmap(inode, 3, true);

    // 一级间接块
    idx = bmap(inode, 7 + 7, true);

    // 二级间接块
    idx = bmap(inode,  7 + 512 * 3 + 510, true);

    iput(inode);
}

void super_init()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &super_table[i];
        sb->dev = EOF;
        sb->desc = NULL;
        sb->buf = NULL;
        sb->iroot = NULL;
        sb->imount = NULL;
        list_init(&sb->inode_list);
    }
    mount_root();


    // device_t *device = device_find(DEV_IDE_PART, 0);
    // assert(device);

    // buffer_t *boot = bread(device->dev, 0);
    // buffer_t *super = bread(device->dev, 1);

    // super_desc_t *sb = (super_desc_t *)super->data;
    // assert(sb->magic == MINIX1_MAGIC);

    // // inode 位图
    // buffer_t *imap = bread(device->dev, 2);

    // // 块位图
    // buffer_t *zmap = bread(device->dev, 2 + sb->imap_blocks);

    // // 读取第一个inode块
    // buffer_t *buf1 = bread(device->dev, 2 + sb->imap_blocks + sb->zmap_blocks);
    // inode_desc_t *inode = (inode_desc_t *)buf1->data;

    // buffer_t *buf2 = bread(device->dev, inode->zone[0]);

    // dentry_t *dir = (dentry_t *)buf2->data;
    // inode_desc_t *helloi = NULL;
    // while (dir->nr)
    // {
    //     LOGK("inode %4d, name %s\n", dir->nr, dir->name);
    //     if (!strcmp(dir->name, "hello.txt"))
    //     {
    //         helloi = &((inode_desc_t *)buf1->data)[dir->nr - 1];
    //         strcpy(dir->name, "world.txt");
    //         buf2->dirty = true;
    //         bwrite(buf2);
    //     }
    //     dir++;
    // }

    // buffer_t *buf3 = bread(device->dev, helloi->zone[0]);
    // LOGK("content %s", buf3->data);

    // strcpy(buf3->data, "This is modified content!!!\n");
    // buf3->dirty = true;
    // bwrite(buf3);

    // helloi->size = strlen(buf3->data);
    // buf1->dirty = true;
    // bwrite(buf1);
}