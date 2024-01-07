#ifndef PHINIX_TTY_H
#define PHINIX_TTY_H

#include <phinix/types.h>

// tty
typedef struct tty_t        
{
    dev_t rdev; // 读设备号
    dev_t wdev; // 写设备号
    pid_t pgid; // 进程组
} tty_t;

// ioctl 设置进程组命令
#define TIOCSPGRP 0x5410

#endif