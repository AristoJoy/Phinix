#ifndef PHINIX_FIFO_H
#define PHINIX_FIFO_H
#include <phinix/types.h>

// fifo
typedef struct fifo_t
{
    char *buf;
    u32 length;
    u32 head;
    u32 tail;
} fifo_t;

// 初始化fifo
void fifo_init(fifo_t *fifo, char *buf, u32 length);

// 判断fifo是否满了
bool fifo_full(fifo_t *fifo);

// 判断fifo是否为空
bool fifo_empty(fifo_t *fifo);

// 获取fifo队头第一个元素
char fifo_get(fifo_t *fifo);

// 将数据放到fifo队尾
void fifo_put(fifo_t *fifo, char byte);

#endif