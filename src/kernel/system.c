#include <phinix/task.h>
#include <phinix/errno.h>

extern task_t *task_table[TASK_NR];

mode_t sys_umask(mode_t mask)
{
    task_t *task = running_task();
    mode_t old = task->umask;
    task->umask = mask & 0777;
    return old;
}

// 设置进程组
int sys_setpgid(int pid, int pgid)
{
    task_t *current = running_task();
    if (!pid)
    {
        pid = current->pid;
    }

    if (!pgid)
    {
        pgid = current->pid;
    }

    for (int i = 0; i < TASK_NR; i++)
    {
        task_t *task = task_table[i];
        if (!task)
        {
            continue;
        }
        if (task->pid != pid)
        {
            continue;
        }
        if (task_leader(task))
        {
            return -EPERM;
        }
        if (task->sid != current->sid)
        {
            return -EPERM;
        }
        task->pgid = pgid;
        return EOK;
    }
    return -ESRCH;
}
// 获取进程组
pid_t sys_getpgrp()
{
    task_t *task = running_task();
    return task->pgid;
}
// 设置会话
pid_t sys_setsid()
{
    task_t *task = running_task();
    if (task_leader(task))
    {
        return -EPERM;
    }
    task->sid = task->pgid = task->pid;
    return task->sid;
}