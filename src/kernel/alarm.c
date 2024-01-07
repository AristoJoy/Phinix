#include <phinix/task.h>
#include <phinix/timer.h>


extern int sys_kill();

// 闹钟函数
static void task_alarm(timer_t *timer)
{
    timer->task->alarm = NULL;
    sys_kill(timer->task->pid, SIGALRM);
}

// 系统闹钟实现
int sys_alarm(int sec)
{
    task_t *task = running_task();
    if (task->alarm)
    {
        timer_put(task->alarm);
    }
    task->alarm = timer_add(sec * 1000, task_alarm, 0);
}