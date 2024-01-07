#include <phinix/tty.h>
#include <phinix/device.h>
#include <phinix/assert.h>
#include <phinix/fifo.h>
#include <phinix/task.h>
#include <phinix/mutex.h>
#include <phinix/debug.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

static tty_t typewriter;

// tty读通知
int tty_rx_notify(char *ch, bool ctrl, bool shift, bool alt)
{
    switch (*ch)
    {
    case '\r':
        *ch = '\n';
        break;
    default:
        break;
    }
    if (!ctrl)
    {
        return 0;
    }

    tty_t *tty = &typewriter;
    switch (*ch)
    {
    case 'c':
    case 'C':
        LOGK("CTRL + C Pressed\n");
        break;
    case 'l':
    case 'L':
        // 清屏
        device_write(tty->wdev, "\x1b[2J\x1b[0;0H\r", 12, 0, 0);
        *ch = '\r';
        return 0;
    default:
        return 1;
    }
    return 1;
}

// tty读
int tty_read(tty_t *tty, char *buf, u32 count)
{
    return device_read(tty->rdev, buf, count, 0, 0);
}

// tty写
int tty_write(tty_t *tty, char *buf, u32 count)
{
    return device_write(tty->wdev, buf, count, 0, 0);
}

// tty控制命令
int tty_ioctl(tty_t *tty, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case TIOCSPGRP:
        tty->pgid = (pid_t)args;
        break;
    default:
        break;
    }
    return -EINVAL;
}

int sys_stty()
{
    return -ENOSYS;
}

int sys_gtty()
{
    return -ENOSYS;
}

// 初始化串口
void tty_init()
{
    device_t *device = NULL;

    tty_t *tty = &typewriter;

    // 输入设备是键盘
    device = device_find(DEV_KEYBOARD, 0);
    tty->rdev = device->dev;

    // 输出设备是控制台
    device = device_find(DEV_CONSOLE, 0);
    tty->wdev = device->dev;

    device_install(DEV_CHAR, DEV_TTY, tty, "tty", 0, tty_ioctl, tty_read, tty_write);
}