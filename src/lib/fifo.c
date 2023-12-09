#include <phinix/fifo.h>
#include <phinix/assert.h>

// 获取fifo下一个位置
static _inline u32 fifo_next(fifo_t *fifo, u32 pos)
{
    return (pos + 1) % fifo->length;
}

// 初始化fifo
void fifo_init(fifo_t *fifo, char *buf, u32 length)
{
    fifo->buf = buf;
    fifo->length = length;
    fifo->head = 0;
    fifo->tail = 0;
}

// 判断fifo是否满了
bool fifo_full(fifo_t *fifo)
{
    return (fifo_next(fifo, fifo->head) == fifo->tail);
}

// 判断fifo是否为空
bool fifo_empty(fifo_t *fifo)
{
    return fifo->head == fifo->tail;
}

// 获取fifo队头第一个元素
char fifo_get(fifo_t *fifo)
{
    assert(!fifo_empty(fifo));
    char byte = fifo->buf[fifo->tail];
    fifo->tail = fifo_next(fifo, fifo->tail);
    return byte;
}

// 将数据放到fifo队尾
void fifo_put(fifo_t *fifo, char byte)
{
    while (fifo_full(fifo))
    {
        fifo_get(fifo);
    }
    fifo->buf[fifo->head] = byte;
    fifo->head = fifo_next(fifo, fifo->head);
}