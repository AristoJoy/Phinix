#include <phinix/syscall.h>

static _inline u32 _syscall0(u32 func_code)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(func_code)
    );
    return ret;
}

static _inline u32 _syscall1(u32 nr, u32 arg)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        :"=a"(ret)
        :"a"(nr), "b"(arg)
    );
    return ret;
}

static _inline u32 _syscall2(u32 nr, u32 arg1, u32 arg2)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

static _inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3)
{
    u32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

u32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void exit(int status)
{
    return _syscall1(SYS_NR_EXIT, (u32)status);
}

pid_t fork()
{
    return _syscall0(SYS_NR_FORK);
}

pid_t waitpid(pid_t pid, int32 *status)
{
    return _syscall2(SYS_NR_WAITPID, pid, (u32) status);
}

void yield()
{
    _syscall0(SYS_NR_YIELD);
}

void sleep(u32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}

// 获取任务id
pid_t getpid()
{
    return _syscall0(SYS_NR_GETPID);
}

// 获取父任务id
pid_t getppid()
{
    return _syscall0(SYS_NR_GETPPID);
}

// brk调用
int32 brk(void *addr)
{
    return _syscall1(SYS_NR_BRK, (u32) addr);
}

// 打开文件
fd_t open(char *filename, int flags, int mode)
{
    return _syscall3(SYS_NR_OPEN, (u32)filename, (u32)flags, (u32)mode);
}

// 创建普通文件
fd_t create(char *filename, int mode)
{
    return _syscall2(SYS_NR_CREAT, (u32)filename, (u32)mode);
}

// 关闭文件
void close(fd_t fd)
{
    return _syscall1(SYS_NR_CLOSE, (u32)fd);
}

// 读文件
int read(fd_t fd, char *buf, int len)
{
    return _syscall3(SYS_NR_READ, (u32)fd, (u32)buf, (u32)len);
}

// 系统调用write
int write(fd_t fd, char *buf, u32 len)
{
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

// 读取目录
int readdir(fd_t fd, void *dir, int count)
{
    return _syscall3(SYS_NR_READDIR, fd, (u32)dir, (u32)count);
}

// 设置文件偏移量
int lseek(fd_t fd, off_t offset, int whence)
{
    return _syscall3(SYS_NR_LSEEK, fd, offset, whence);
}

// 获取当前路径
char *getcwd(char *buf, size_t size)
{
    return _syscall2(SYS_NR_GETCWD, (u32)buf, (u32)size);
}

// 切换当前目录
int chdir(char *pathname)
{
    return _syscall1(SYS_NR_CHDIR, (u32)pathname);
}

// 切换根目录
int chroot(char *pathname)
{
    return _syscall1(SYS_NR_CHROOT, (u32)pathname);
}

// 创建目录
int mkdir(char *pathname, int mode)
{
    return _syscall2(SYS_NR_MKDIR, (u32)pathname, (u32)mode);
}

// 删除目录
int rmdir(char *pathname)
{
    return _syscall1(SYS_NR_RMDIR, (u32)pathname);
}

// 链接文件
int link(char *oldname, char *newname)
{
    return _syscall2(SYS_NR_LINK, (u32)oldname, (u32)newname);
}

// 删除链接文件
int unlink(char *filename)
{
    return _syscall1(SYS_NR_UNLINK, (u32)filename);
}

// 获取从1970 1 1 00:00:00 开始的秒数
time_t time()
{
    return _syscall0(SYS_NR_TIME);
}

mode_t umask(mode_t mask)
{
    return _syscall1(SYS_NR_UMASK, (u32)mask);
}

void clear()
{
    _syscall0(SYS_NR_CLEAR);
}

// 获取文件状态
int stat(char *filename, stat_t *statbuf)
{
    return _syscall2(SYS_NR_STAT, (u32)filename, (u32)statbuf);
}
// 通过文件描述符获取文件状态
int fstat(fd_t fd, stat_t *statbuf)
{
    return _syscall2(SYS_NR_FSTAT, (u32)fd, (u32)statbuf);
}