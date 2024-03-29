#include <phinix/io.h>
#include <phinix/interrupt.h>
#include <phinix/fifo.h>
#include <phinix/task.h>
#include <phinix/mutex.h>
#include <phinix/assert.h>
#include <phinix/device.h>
#include <phinix/debug.h>
#include <phinix/stdarg.h>
#include <phinix/stdio.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define COM1_IOBASE 0x3F8 // 串口1基地址
#define COM2_IOBASE 0x2F8 // 串口2基地址

#define COM_DATA 0          // 数据寄存器
#define COM_INTR_ENABLE 1   // 中断允许
#define COM_BAUD_LSB 0      // 波特率低字节
#define COM_BAUD_MSB 1      // 波特率高字节
#define COM_INTR_IDENTIFY 2 // 中断识别
#define COM_LINE_CONTROL 3  // 线控制
#define COM_MODEM_CONTROL 4 // 调制解调器控制
#define COM_LINE_STATUS 5   // 线状态
#define COM_MODEM_STATUS 6  // 调制解调器状态

// 线状态
#define LSR_DR 0x1
#define LSR_OE 0x2
#define LSR_PE 0x4
#define LSR_FE 0x8
#define LSR_BI 0x10
#define LSR_THRE 0x20
#define LSR_TEMT 0x40
#define LSR_IE 0x80

#define BUF_LEN 64

// 串口设备
typedef struct serial_t
{
    u16 iobase;           // 端口号基地址
    fifo_t rx_fifo;       // 读fifo
    char rx_buf[BUF_LEN]; // 读缓冲
    lock_t rlock;         // 读锁
    task_t *rx_waiter;    /// 读等待任务
    lock_t wlock;         // 写锁
    task_t *tx_waiter;    // 写等待任务
} serial_t;

static serial_t serials[2];

// 读取串口数据
void recv_data(serial_t *serial)
{
    char ch = in_byte(serial->iobase);
    // 特殊处理，回车直接换行
    if (ch == '\r')
    {
        ch = '\n';
    }
    fifo_put(&serial->rx_fifo, ch);
    if (serial->rx_waiter != NULL)
    {
        task_unblock(serial->rx_waiter, EOK);
        serial->rx_waiter = NULL;
    }
    
}

// 串口中断处理函数
void serial_handler(int vector)
{
    u32 irq = vector - 0x20;
    assert(irq == IRQ_SERIAL_1 || irq == IRQ_SERIAL_2);
    send_eoi(vector);

    serial_t *serial = &serials[IRQ_SERIAL_1 - irq];
    u8 state = in_byte(serial->iobase + COM_LINE_STATUS);

    // 数据可读
    if (state & LSR_DR)
    {
        recv_data(serial);
    }

    // 如果可以发送数据，并且写进程阻塞
    if ((state & LSR_THRE) && serial->tx_waiter)
    {
        task_unblock(serial->tx_waiter, EOK);
        serial->tx_waiter = NULL;
    }
    
    
}

// 串行设备读取数据
int serial_read(serial_t *serial, char *buf, u32 count)
{
    lock_acquire(&serial->rlock);
    int nr = 0;
    while (nr < count)
    {
        while (fifo_empty(&serial->rx_fifo))
        {
            assert(serial->rx_waiter == NULL);
            serial->rx_waiter = running_task();
            task_block(serial->rx_waiter, NULL, TASK_BLOCKED, TIMELESS);
        }
        buf[nr++] = fifo_get(&serial->rx_fifo);
    }
    
    lock_release(&serial->rlock);

    return nr;
}

// 串行设备写数据
int serial_write(serial_t *serial, char *buf, u32 count)
{
    lock_acquire(&serial->wlock);
    int nr = 0;

    while (nr < count)
    {
        u8 state = in_byte(serial->iobase + COM_LINE_STATUS);
        if (state & LSR_THRE)
        {
            out_byte(serial->iobase, buf[nr++]);
            continue;
        }
        // task_t *task = running_task();
        // serial->tx_waiter = task;
        // task_block(task, NULL, TASK_BLOCKED, TIMELESS);
    }

    lock_release(&serial->wlock);
    return nr;
}

// 初始化串口
void serial_init()
{
    for (size_t i = 0; i < 2; i++)
    {
        serial_t *serial = &serials[i];
        fifo_init(&serial->rx_fifo, serial->rx_buf, BUF_LEN);
        serial->rx_waiter = NULL;
        lock_init(&serial->rlock);
        serial->tx_waiter = NULL;
        lock_init(&serial->wlock);

        u16 irq;
        if (!i)
        {
            irq = IRQ_SERIAL_1;
            serial->iobase = COM1_IOBASE;
        }
        else
        {
            irq = IRQ_SERIAL_2;
            serial->iobase = COM2_IOBASE;
        }
        
        // 禁用中断
        out_byte(serial->iobase + COM_INTR_ENABLE, 0);

        // 激活DLAB 
        out_byte(serial->iobase + COM_LINE_CONTROL, 0x80);

        // 设置波特率因子 0x0030
        // 波特率为115200 / 0x30 = 115200 / 48 = 2400
        out_byte(serial->iobase + COM_BAUD_LSB, 0x30);
        out_byte(serial->iobase + COM_BAUD_MSB, 0x00);

        // 复位DLAB位，数据位为8位
        out_byte(serial->iobase + COM_LINE_CONTROL, 0x03);

        // 启用 FIFO, 清空 FIFO, 14 字节触发电平
        out_byte(serial->iobase + COM_INTR_IDENTIFY, 0xC7);

        // 设置回环模式，测试串口芯片
        out_byte(serial->iobase + COM_MODEM_CONTROL, 0b11011);

        // 发送字节
        out_byte(serial->iobase, 0xAE);
        
        // 收到的内容与发送的内容不一致，这串口不可用
        if (in_byte(serial->iobase) != 0xAE)
        {
            continue;
        }
        
        // 设置回原料的模式
        out_byte(serial->iobase + COM_MODEM_CONTROL, 0b1011);

        // 注册中断处理函数
        set_interrupt_handler(irq, serial_handler);
        
        // 打开中断屏蔽
        set_interrupt_mask(irq, true);

        // 0x0f = 0b1111
        // 数据可用 + 中断/错误 + 状态变化 + 传输器空 都发送中断
        out_byte(serial->iobase + COM_INTR_ENABLE, 0x0F);

        char name[16];

        sprintf(name, "com%d", i + 1);

        device_install(DEV_CHAR, DEV_SERIAL, serial, name, 0, NULL, serial_read, serial_write);

        LOGK("Serial 0x%x init...\n", serial->iobase);
    }
    
}