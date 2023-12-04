#include <phinix/mutex.h>
#include <phinix/task.h>
#include <phinix/interrupt.h>
#include <phinix/assert.h>


// 初始化互斥量
void mutex_init(mutex_t *mutex)
{
    mutex->value = false;
    list_init(&mutex->waiters);
}

// 尝试持有互斥量
void mutex_lock(mutex_t *mutex)
{
    // 关闭中断，保证原子性
    bool intr = interrupt_disable();

    task_t *current = running_task();

    while (mutex->value == true)
    {
        // 若value为true，表示锁已经被别人持有
        task_block(current, &mutex->waiters, TASK_BLOCKED);
    }

    // 无人持有
    assert(mutex->value == false);

    // 持有
    mutex->value++;
    assert(mutex->value == true);

    set_interrupt_state(intr);
}

// 释放互斥量
void mutex_unlock(mutex_t *mutex)
{
    // 关闭中断，保证原子性
    bool intr = interrupt_disable();

    // 已持有互斥量
    assert(mutex->value == true);

    // 取消持有
    mutex->value--;

    assert(mutex->value == false);

    // 如果等待队列不为空，这恢复执行
    if (!list_empty(&mutex->waiters))
    {
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
        assert(task->magic == PHINIX_MAGIC);

        task_unblock(task);
        // 保证新检测（因为unblock后马上去block了）能够获得互斥量，不然可能饿死
        task_yield();
    }
    
    set_interrupt_state(intr);
}