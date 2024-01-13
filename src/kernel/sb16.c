#include <phinix/types.h>
#include <phinix/io.h>
#include <phinix/sb16.h>
#include <phinix/syscall.h>
#include <phinix/interrupt.h>
#include <phinix/memory.h>
#include <phinix/isa.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/device.h>
#include <phinix/string.h>
#include <phinix/buffer.h>
#include <phinix/task.h>
#include <phinix/stdlib.h>
#include <phinix/stat.h>
#include <phinix/fs.h>
#include <phinix/mutex.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define SB_MIXER 0x224      // DSP 混合器端口
#define SB_MIXER_DATA 0x225 // DSP 混合器数据端口
#define SB_RESET 0x226      // DSP 重置
#define SB_READ 0x22A       // DSP 读
#define SB_WRITE 0x22C      // DSP 写
#define SB_STATE 0x22E      // DSP 读状态
#define SB_INTR16 0x22F     // DSP 16 位中断响应

#define CMD_STC 0x40  // Set Time Constant
#define CMD_SOSR 0x41 // Set Output Sample Rate
#define CMD_SISR 0x42 // Set Input Sample Rate

#define CMD_SINGLE_IN8 0xC8   // Transfer mode 8bit input
#define CMD_SINGLE_OUT8 0xC0  // Transfer mode 8bit output
#define CMD_SINGLE_IN16 0xB8  // Transfer mode 16bit input
#define CMD_SINGLE_OUT16 0xB0 // Transfer mode 16bit output

#define CMD_AUTO_IN8 0xCE   // Transfer mode 8bit input auto
#define CMD_AUTO_OUT8 0xC6  // Transfer mode 8bit output auto
#define CMD_AUTO_IN16 0xBE  // Transfer mode 16bit input auto
#define CMD_AUTO_OUT16 0xB6 // Transfer mode 16bit output auto

#define CMD_ON 0xD1      // Turn speaker on
#define CMD_OFF 0xD3     // Turn speaker off
#define CMD_SP8 0xD0     // Stop playing 8 bit channel
#define CMD_RP8 0xD4     // Resume playback of 8 bit channel
#define CMD_SP16 0xD5    // Stop playing 16 bit channel
#define CMD_RP16 0xD6    // Resume playback of 16 bit channel
#define CMD_VERSION 0xE1 // Turn speaker off

#define MODE_MONO8 0x00
// #define MODE_STEREO8 0x20
// #define MODE_MONO16 0x10
#define MODE_STEREO16 0x30

#define STATUS_READ 0x80  // read buffer status
#define STATUS_WRITE 0x80 // write buffer status

#define DMA_BUF_ADDR 0x40000 // 必须 64K 字节对齐
#define DMA_BUF_SIZE 0x4000  // 缓冲区长度

#define SAMPLE_RATE 44100 // 采样率

typedef struct sb_t
{
    task_t *waiter;
    lock_t lock;
    char *addr; // DMA地址
    u8 mode;    // 模式
    u8 channel; // DMA通道
} sb_t;

static sb_t sb16;

// 声卡中断处理器
static void sb_handler(int vector)
{
    send_eoi(vector);
    sb_t *sb = &sb16;

    in_byte(SB_INTR16);

    u8 state = in_byte(SB_STATE);

    LOGK("sb16 handler state 0x%X...\n", state);
    if (sb->waiter)
    {
        task_unblock(sb->waiter, EOK);
        sb->waiter = NULL;
    }
}

// 重置声卡
static void sb_reset(sb_t *sb)
{
    out_byte(SB_RESET, 1);
    sleep(1);
    out_byte(SB_RESET, 0);
    u8 state = in_byte(SB_READ);
    LOGK("sb16 reset state 0x%x\n", state);
}

// 设置声卡中断
static void sb_intr_irq(sb_t *sb)
{
    out_byte(SB_MIXER, 0x80);
    u8 data = in_byte(SB_MIXER_DATA);
    if (data != 2)
    {
        out_byte(SB_MIXER, 0x80);
        out_byte(SB_MIXER, 0x02);
    }
}

// 向声卡发出命令
static void sb_out(u8 cmd)
{
    while (in_byte(SB_WRITE) & 128)
        ;
    out_byte(SB_WRITE, cmd);
}

// 声卡开关
static void sb_turn(sb_t *sb, bool on)
{
    if (on)
    {
        sb_out(CMD_ON);
    }
    else
    {
        sb_out(CMD_OFF);
    }
}

// 计算声卡时间常量
static u32 sb_time_constant(u8 channels, u16 sample)
{
    return 65536 - (256000000 / (channels * sample));
}

// 设置声卡声量
static void sb_set_volume(u8 level)
{
    LOGK("set sb16 volume to 0x%02X\n", level);
    out_byte(SB_MIXER, 0x22);
    out_byte(SB_MIXER_DATA, level);
}

// 声卡控制命令
int sb16_ioctl(sb_t *sb, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case SB16_CMD_ON:
        sb_reset(sb);    // 重置DSP
        sb_intr_irq(sb); // 设置中断
        sb_out(CMD_ON);  // 打开声霸卡
        return EOK;
    case SB16_CMD_OFF:
        sb_out(CMD_OFF); // 关闭声霸卡
        return EOK;
    case SB16_CMD_MONO8:
        sb->mode = MODE_MONO8;
        sb->channel = 1;
        isa_dma_reset(sb->channel);
        return EOK;
    case SB16_CMD_STEREO16:
        sb->mode = MODE_STEREO16;
        sb->channel = 5;
        isa_dma_reset(sb->channel);
        return EOK;
    case SB16_CMD_VOLUME:
        sb_set_volume((u8)args);
        return EOK;
    default:
        break;
    }
    return -EINVAL;
}

// 声卡写操作
int sb16_write(sb_t *sb, char *data, size_t size)
{
    lock_acquire(&sb->lock);
    assert(size <= DMA_BUF_SIZE);
    memcpy(sb->addr, data, size);

    isa_dma_mask(sb->channel, false);

    // 设置地址和大小
    isa_dma_addr(sb->channel, sb->addr);
    isa_dma_size(sb->channel, size);

    // 设置采样率
    sb_out(CMD_SOSR);                  // 44100 = 0xAC44
    sb_out((SAMPLE_RATE >> 8) & 0xFF); // 0xAC
    sb_out(SAMPLE_RATE & 0xFF);        // 0x44

    if (sb->mode == MODE_MONO8)
    {
        isa_dma_mode(sb->channel, DMA_MODE_SINGLE | DMA_MODE_WRITE);
        sb_out(CMD_SINGLE_OUT8);
        sb_out(MODE_MONO8);
    }
    else
    {
        isa_dma_mode(sb->channel, DMA_MODE_SINGLE | DMA_MODE_WRITE);
        sb_out(CMD_SINGLE_OUT16);
        sb_out(MODE_STEREO16);
        size >>= 2; // size /= 4
    }

    sb_out((size - 1) & 0xFF);
    sb_out(((size - 1) >> 8) & 0xFF);

    isa_dma_mask(sb->channel, true);

    assert(sb->waiter == NULL);
    sb->waiter = running_task();
    assert(task_block(sb->waiter, NULL, TASK_BLOCKED, TIMELESS) == EOK);

    lock_release(&sb->lock);
    return size;
}

// 声卡初始化
void sb16_init()
{
    LOGK("Sound Blaster 16 init...\n");

    sb_t *sb = &sb16;

    sb->addr = (char *)DMA_BUF_ADDR;
    sb->mode = MODE_STEREO16;
    sb->channel = 5;
    lock_init(&sb->lock);

    set_interrupt_handler(IRQ_SB16, sb_handler);
    set_interrupt_mask(IRQ_SB16, true);

    device_install(DEV_CHAR, DEV_SB16, sb, "sb16", 0, sb16_ioctl, NULL, sb16_write);
}