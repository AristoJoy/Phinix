#include <phinix/fs.h>
#include <phinix/buffer.h>
#include <phinix/stat.h>
#include <phinix/syscall.h>
#include <phinix/string.h>
#include <phinix/task.h>
#include <phinix/assert.h>
#include <phinix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// 判断是否有权限
bool permission(inode_t *inode, u16 mask)
{
    u16 mode = inode->desc->mode;

    if (!inode->desc->nlinks)
    {
        return false;
    }

    task_t *task = running_task();
    if (task->uid == KERNEL_USER)
    {
        return true;
    }

    if (task->uid == inode->desc->uid)
    {
        mode >>= 6;
    }
    else if (task->gid == inode->desc->gid)
    {
        mode >>= 3;
    }

    if ((mode & mask & 0b111) == mask)
    {
        return true;
    }
    return false;
}

// 判断文件名是否相等
static bool match_name(const char *name, const char *entry_name, char **next)
{
    char *lhs = (char *)name;
    char *rhs = (char *)entry_name;
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS)
    {
        lhs++;
        rhs++;
    }
    if (*rhs)
    {
        return false;
    }
    if (*lhs && !IS_SEPARATOR(*lhs))
    {
        return false;
    }
    if (IS_SEPARATOR(*lhs))
    {
        lhs++;
    }
    *next = lhs;
    return true;
}

// 获取dir目录下的name目录所在的dentry_t 和buffer_t
static buffer_t *find_entry(inode_t **dir, const char *name, char **next, dentry_t **result)
{
    // 保证dir是目录
    assert(ISDIR((*dir)->desc->mode));

    // 父目录是根设备
    if (match_name(name, "..", next) && (*dir)->nr == 1)
    {
        super_block_t *sb = get_super((*dir)->dev);
        inode_t *inode = *dir;
        // 将dir指向被挂载的目录，并且其引用计数加1
        (*dir) = sb->imount;
        (*dir)->count++;
        iput(inode);
    }
    

    // 获取目录所在的超级块
    // super_block_t *sb = read_super((*dir)->dev);

    // dir目录最多子目录数量
    u32 entries = ((*dir)->desc->size) / sizeof(dentry_t);

    idx_t i = 0;
    idx_t block = 0;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    idx_t nr = EOF;

    for (; i < entries; i++, entry++)
    {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE)
        {
            brelse(buf);
            block = bmap((*dir), i / BLOCK_DENTRIES, false);
            assert(block);

            buf = bread((*dir)->dev, block);
            entry = (dentry_t *)buf->data;
        }
        if (match_name(name, entry->name, next) && entry->nr)
        {
            *result = entry;
            return buf;
        }
    }
    brelse(buf);
    return NULL;
}

// 在dir目录中添加name目录项
static buffer_t *add_entry(inode_t *dir, const char *name, dentry_t **result)
{
    char *next = NULL;
    buffer_t *buf = find_entry(&dir, name, &next, result);

    if (buf)
    {
        return buf;
    }

    // name中不能有分隔符
    for (size_t i = 0; i < NAME_LEN && name[i]; i++)
    {
        assert(!IS_SEPARATOR(name[i]));
    }

    // super_block_t *sb = get_super(dir->dev);
    // assert(sb);

    idx_t i = 0;
    idx_t block = 0;
    dentry_t *entry;

    for (; true; i++, entry++)
    {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE)
        {
            brelse(buf);
            block = bmap(dir, i / BLOCK_DENTRIES, true);
            assert(block);

            buf = bread(dir->dev, block);
            entry = (dentry_t *)buf->data;
        }
        if (i * sizeof(dentry_t) >= dir->desc->size)
        {
            entry->nr = 0;
            dir->desc->size = (i + 1) * sizeof(dentry_t);
            dir->buf->dirty = true;
        }
        if (entry->nr)
        {
            continue;
        }
        strncpy(entry->name, name, NAME_LEN);
        buf->dirty = true;
        dir->desc->mtime = time();
        dir->buf->dirty = true;
        *result = entry;
        return buf;
    }
}

// 获取pathname对应的父目录inode
inode_t *named(char *pathname, char **next)
{
    inode_t *inode = NULL;
    task_t *task = running_task();
    char *left = pathname;

    if (IS_SEPARATOR(left[0]))
    {
        inode = task->iroot;
        left++;
    }
    else if (left[0])
    {
        inode = task->ipwd;
    }
    else
    {
        return NULL;
    }

    inode->count++;
    *next = left;

    // 没子目录
    if (!*left)
    {
        return inode;
    }

    char *right = strrsep(left);
    if (!right || right < left)
    {
        return inode;
    }

    right++;

    *next = left;
    dentry_t *entry = NULL;
    buffer_t *buf = NULL;

    while (true)
    {
        brelse(buf);
        buf = find_entry(&inode, left, next, &entry);
        if (!buf)
        {
            goto failure;
        }
        dev_t dev = inode->dev;
        iput(inode);
        inode = iget(dev, entry->nr);
        if (!ISDIR(inode->desc->mode) || !permission(inode, P_EXEC))
        {
            goto failure;
        }
        if (right == *next)
        {
            goto success;
        }

        left = *next;
    }
success:
    brelse(buf);
    return inode;

failure:
    brelse(buf);
    iput(inode);
    return NULL;
}

// 获取pathname对应的inode
inode_t *namei(char *pathname)
{
    char *next = NULL;
    inode_t *dir = named(pathname, &next);
    if (!dir)
    {
        return NULL;
    }
    // 剩余路径为空
    if (!(*next))
    {
        return dir;
    }

    char *name = next;
    dentry_t *entry = NULL;
    buffer_t *buf = find_entry(&dir, name, &next, &entry);
    if (!buf)
    {
        iput(dir);
        return NULL;
    }

    inode_t *inode = iget(dir->dev, entry->nr);
    iput(dir);
    brelse(buf);
    return inode;
}

// 创建目录
int sys_mkdir(char *pathname, int mode)
{
    char *next = NULL;
    buffer_t *ebuf = NULL;
    inode_t *dir = named(pathname, &next);

    // 父目录不存在
    if (!dir)
    {
        goto rollback;
    }

    // 目录名为空
    if (!*next)
    {
        goto rollback;
    }

    // 父目录没有写权限
    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    char *name = next;
    dentry_t *entry;

    ebuf = find_entry(&dir, name, &next, &entry);
    // 目录项已存在
    if (ebuf)
    {
        goto rollback;
    }

    // 在父目录下添加一个entry
    ebuf = add_entry(dir, name, &entry);
    ebuf->dirty = true;
    // 将目录项inode索引执行新分配的inode的block
    entry->nr = ialloc(dir->dev);

    task_t *task = running_task();

    // 获取索引的inode
    inode_t *inode = new_inode(dir->dev, entry->nr);
    inode->desc->mode = (mode & 0777 & ~task->umask) | IFDIR;
    inode->desc->size = sizeof(dentry_t) * 2; // 当前目录和父目录两个目录项
    inode->desc->nlinks = 2;

    // 父目录
    dir->buf->dirty = true;
    dir->desc->nlinks++; // 当前目录的..目录项指向父目录

    // 写入inode目录中的默认目录项(bmap先创建块)
    buffer_t *zbuf = bread(inode->dev, bmap(inode, 0, true));
    zbuf->dirty = true;

    entry = (dentry_t *)zbuf->data;

    strcpy(entry->name, ".");
    entry->nr = inode->nr;

    entry++;
    strcpy(entry->name, "..");
    entry->nr = dir->nr;

    // 申请和释放倒序
    iput(inode);
    iput(dir);

    brelse(ebuf);
    brelse(zbuf);
    return 0;

rollback:
    brelse(ebuf);
    iput(dir);
    return EOF;
}

// 判断目录inode是否为空
static bool is_empty(inode_t *inode)
{
    assert(ISDIR(inode->desc->mode));

    int entries = inode->desc->size / sizeof(dentry_t);
    // 正常目录至少有2个目录项，且第一个zone必定不为空（用于存放目录项)
    if (entries < 2 || !inode->desc->zone[0])
    {
        LOGK("bad directory on dev %d\n", inode->dev);
        return false;
    }

    idx_t block = 0;
    buffer_t *buf = NULL;
    dentry_t *entry;
    int count = 0;

    for (idx_t i = 0; i < entries; i++, entry++)
    {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE)
        {
            brelse(buf);
            block = bmap(inode, i / BLOCK_DENTRIES, false);
            assert(block);

            buf = bread(inode->dev, block);
            entry = (dentry_t *)buf->data;
        }
        // 存在指向目录的inode的block索引，就认为这是一个目录，不判断真实的inode
        if (entry->nr)
        {
            count++;
        }
    }

    brelse(buf);

    if (count < 2)
    {
        LOGK("bad directory on dev %d\n", inode->dev);
        return false;
    }

    return count == 2;
}

// 删除目录
int sys_rmdir(char *pathname)
{
    char *next = NULL;
    buffer_t *ebuf = NULL;
    inode_t *dir = named(pathname, &next);
    inode_t *inode = NULL;
    int ret = EOF;

    // 如果父目录不存在
    if (!dir)
    {
        goto rollback;
    }
    // 目录名为空
    if (!*next)
    {
        goto rollback;
    }

    // 父目录无权限写
    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    char *name = next;
    dentry_t *entry;

    ebuf = find_entry(&dir, name, &next, &entry);
    // 目录项不存在
    if (!ebuf)
    {
        goto rollback;
    }
    inode = iget(dir->dev, entry->nr);
    if (!inode)
    {
        goto rollback;
    }

    if (inode == dir)
    {
        goto rollback;
    }

    if (!ISDIR(inode->desc->mode))
    {
        goto rollback;
    }

    task_t *task = running_task();
    // 非文件用户不能删除
    if ((dir->desc->mode & ISVTX) && task->uid != inode->desc->uid)
    {
        goto rollback;
    }

    // 设备不同不能删除；还有其他引用不能删除
    if (dir->dev != inode->dev || inode->count > 1)
    {
        goto rollback;
    }

    if (!is_empty(inode))
    {
        goto rollback;
    }

    assert(inode->desc->nlinks == 2);

    // 释放文件块（删除目录项）
    inode_truncate(inode);
    // 释放inode
    ifree(inode->dev, inode->nr);

    // 链接数、block索引置空
    inode->desc->nlinks = 0;
    inode->buf->dirty = true;
    inode->nr = 0;

    // 父目录
    dir->desc->nlinks--;
    dir->ctime = dir->atime = dir->desc->mtime = time();
    dir->buf->dirty = true;
    assert(dir->desc->nlinks > 0);

    entry->nr = 0;
    ebuf->dirty = true;

    ret = 0;
rollback:
    iput(inode);
    iput(dir);
    brelse(ebuf);
    return ret;
}

// 链接文件
int sys_link(char *oldname, char *newname)
{
    int ret = EOF;
    buffer_t *buf = NULL;
    inode_t *dir = NULL;
    inode_t *inode = namei(oldname);
    if (!inode)
    {
        goto rollback;
    }

    if (ISDIR(inode->desc->mode))
    {
        goto rollback;
    }

    char *next = NULL;
    dir = named(newname, &next);
    if (!dir)
    {
        goto rollback;
    }

    if (!(*next))
    {
        goto rollback;
    }

    if (dir->dev != inode->dev)
    {
        goto rollback;
    }

    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    char *name = next;
    dentry_t *entry;

    buf = find_entry(&dir, name, &next, &entry);
    // 目录项存在
    if (buf)
    {
        goto rollback;
    }

    buf = add_entry(dir, name, &entry);
    entry->nr = inode->nr;
    buf->dirty = true;

    inode->desc->nlinks++;
    inode->ctime = time();
    inode->buf->dirty = true;
    ret = 0;

rollback:
    brelse(buf);
    iput(inode);
    iput(dir);
    return ret;
}

// 删除链接文件
int sys_unlink(char *filename)
{
    int ret = EOF;
    char *next = NULL;
    inode_t *inode = NULL;
    buffer_t *buf = NULL;
    inode_t *dir = named(filename, &next);
    if (!dir)
    {
        goto rollback;
    }
    if (!(*next))
    {
        goto rollback;
    }

    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    char *name = next;
    dentry_t *entry;

    buf = find_entry(&dir, name, &next, &entry);
    if (!buf)
    {
        goto rollback;
    }

    inode = iget(dir->dev, entry->nr);
    if (ISDIR(inode->desc->mode))
    {
        goto rollback;
    }

    task_t *task = running_task();
    if ((inode->desc->mode & ISVTX) && task->uid != inode->desc->uid)
    {
        goto rollback;
    }

    if (!inode->desc->nlinks)
    {
        LOGK("deleting non exists file (%04x:%d)\n", inode->dev, inode->nr);
    }

    entry->nr = 0;
    buf->dirty = true;

    inode->desc->nlinks--;
    inode->buf->dirty = true;

    if (inode->desc->nlinks == 0)
    {
        inode_truncate(inode);
        ifree(inode->dev, inode->nr);
    }

    ret = 0;

rollback:
    brelse(buf);
    iput(inode);
    iput(dir);
    return ret;
}

// 打开文件，返回inode
inode_t *inode_open(char *pathname, int flag, int mode)
{
    inode_t *dir = NULL;
    inode_t *inode = NULL;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    char *next = NULL;
    dir = named(pathname, &next);
    if (!dir)
    {
        goto rollback;
    }

    if (!(*next))
    {
        return dir;
    }

    if ((flag & O_TRUNC) && ((flag & O_ACCMODE) == O_RDONLY))
    {
        flag |= O_RDWR;
    }

    char *name = next;
    buf = find_entry(&dir, name, &next, &entry);
    if (buf)
    {
        inode = iget(dir->dev, entry->nr);
        goto makeup;
    }

    if (!(flag & O_CREAT))
    {
        goto rollback;
    }

    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    buf = add_entry(dir, name, &entry);
    entry->nr = ialloc(dir->dev);
    inode = new_inode(dir->dev, entry->nr);

    task_t *task = running_task();

    mode &= (0777 & ~task->umask);
    mode |= IFREG;
    inode->desc->mode = mode;

makeup:
    if (!permission(inode, ACC_MODE(flag & O_ACCMODE)))
    {
        goto rollback;
    }

    if (ISDIR(inode->desc->mode) && ((flag & O_ACCMODE) != O_RDONLY))
    {
        goto rollback;
    }
    inode->atime = time();

    if (flag & O_TRUNC)
        inode_truncate(inode);

    brelse(buf);
    iput(dir);
    return inode;
rollback:
    brelse(buf);
    iput(dir);
    iput(inode);
    return NULL;
}

// 获取当前路径
char *sys_getcwd(char *buf, size_t size)
{
    task_t *task = running_task();
    strncpy(buf, task->pwd, size);
    return buf;
}

// 计算当前路径pwd和新路径pathname，存入pwd
void abspath(char *pwd, const char *pathname)
{
    char *cur = NULL;
    char *ptr = NULL;
    if (IS_SEPARATOR(pathname[0]))
    {
        cur = pwd + 1;
        *cur = 0;
        pathname++;
    }
    else
    {
        cur = strrsep(pwd) + 1;
        *cur = 0;
    }

    while (pathname[0])
    {
        ptr = strsep(pathname);
        if (!ptr)
        {
            break;
        }

        int len = (ptr - pathname) + 1;
        *ptr = '/';
        if (!memcmp(pathname, "./", 2))
        {
            /* code */
        }
        else if (!memcmp(pathname, "../", 3))
        {
            if (cur - 1 != pwd)
            {
                *(cur - 1) = 0;
                cur = strrsep(pwd) + 1;
                *cur = 0;
            }
        }
        else
        {
            strncpy(cur, pathname, len + 1);
            cur += len;
        }
        pathname += len;
    }

    if (!pathname[0])
        return;

    if (!strcmp(pathname, "."))
        return;

    if (strcmp(pathname, ".."))
    {
        strcpy(cur, pathname);
        cur += strlen(pathname);
        *cur = '/';
        *(cur + 1) = '\0';
        return;
    }
    if (cur - 1 != pwd)
    {
        *(cur - 1) = 0;
        cur = strrsep(pwd) + 1;
        *cur = 0;
    }
}

// 切换当前目录
int sys_chdir(char *pathname)
{
    task_t *task = running_task();
    inode_t *inode = namei(pathname);
    if (!inode)
    {
        goto rollback;
    }
    if (!ISDIR(inode->desc->mode) || inode == task->ipwd)
    {
        goto rollback;
    }

    abspath(task->pwd, pathname);

    iput(task->ipwd);
    task->ipwd = inode;
    return 0;
rollback:
    iput(inode);
    return EOF;
}

// 切换根目录
int sys_chroot(char *pathname)
{
    task_t *task = running_task();
    inode_t *inode = namei(pathname);
    if (!inode)
    {
        goto rollback;
    }
    if (!ISDIR(inode->desc->mode) || inode == task->iroot)
    {
        goto rollback;
    }

    iput(task->iroot);
    task->iroot = inode;
    return 0;
rollback:
    iput(inode);
    return EOF;
}

int sys_mknod(char *filename, int mode, int dev)
{
    char *next = NULL;
    inode_t *dir = NULL;
    inode_t *inode = NULL;
    buffer_t *buf = NULL;
    int ret = EOF;

    dir = named(filename, &next);
    if (!dir)
    {
        goto rollback;
    }
    if (!(*next))
    {
        goto rollback;
    }
    if (!permission(dir, P_WRITE))
    {
        goto rollback;
    }

    char *name = next;
    dentry_t *entry;
    buf = find_entry(&dir, name, &next, &entry);
    if (buf)
    {
        goto rollback;
    }

    buf = add_entry(dir, name, &entry);
    buf->dirty = true;
    entry->nr = ialloc(dir->dev);

    inode = new_inode(dir->dev, entry->nr);

    inode->desc->mode = mode;
    if (ISBLK(mode) || ISCHR(mode))
    {
        inode->desc->zone[0] = dev;
    }

    ret = 0;

rollback:
    brelse(buf);
    iput(inode);
    iput(dir);
    return ret;
}