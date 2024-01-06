#include <phinix/console.h>
#include <phinix/io.h>
#include <phinix/string.h>
#include <phinix/interrupt.h>
#include <phinix/device.h>

#define CRT_ADDR_REG 0x3D4 // CRT (6845)索引寄存器
#define CRT_DATA_REG 0x3D5 // CRT (6845)数据寄存器

#define CRT_START_ADDR_H 0xC // 显示内存起始位置 - 高位
#define CRT_START_ADDR_L 0xD // 显示内存起始位置 - 低位
#define CRT_CURSOR_H 0xE     // 光标位置 - 高位
#define CRT_CURSOR_L 0xF     // 光标位置 - 低位

#define MEM_BASE 0xB8000              // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕字节数

#define ASCII_NULL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07 // \a
#define ASCII_BS 0x08  // \b
#define ASCII_HT 0x09  // \t
#define ASCII_LF 0x0A  // \n
#define ASCII_VT 0x0B  // \v
#define ASCII_FF 0x0C  // \f
#define ASCII_CR 0x0D  // \r
#define ASCII_DEL 0x0F

static u32 screen; // 当前显示器开始的内存位置

static u32 pos; //  记录当前光标的内存位置

static x, y; // 当前光标的坐标

static u8 attr = 7; // 字符样式

static u16 erase = 0x0720; // 空格(20) + 样式

// 获得当前显示器的开始位置
static void get_screen()
{
    out_byte(CRT_ADDR_REG, CRT_START_ADDR_H); // 开始位置高地址
    screen = in_byte(CRT_DATA_REG) << 8;
    out_byte(CRT_ADDR_REG, CRT_START_ADDR_L); // 开始位置低地址
    screen |= in_byte(CRT_DATA_REG);

    screen <<= 1; // screen * 2
    screen += MEM_BASE;
}

// 设置当前显示器的开始位置
static set_screen()
{
    out_byte(CRT_ADDR_REG, CRT_START_ADDR_H); // 开始位置高地址
    out_byte(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);

    out_byte(CRT_ADDR_REG, CRT_START_ADDR_L); // 开始位置低地址
    out_byte(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

// 获得当前光标的位置
static void get_cursor()
{
    out_byte(CRT_ADDR_REG, CRT_CURSOR_H); // 光标高地址
    pos = in_byte(CRT_DATA_REG) << 8;
    out_byte(CRT_ADDR_REG, CRT_CURSOR_L); // 光标低地址
    pos |= in_byte(CRT_DATA_REG);

    get_screen();

    pos <<= 1;
    pos += MEM_BASE;

    u32 delta = (pos - screen) >> 1;

    x = delta % WIDTH;
    y = delta / WIDTH;
}

// 设置光标的位置
static void set_cursor()
{
    out_byte(CRT_ADDR_REG, CRT_CURSOR_H); // 光标高地址
    out_byte(CRT_DATA_REG, ((pos - MEM_BASE) >> 9) & 0xff);

    out_byte(CRT_ADDR_REG, CRT_CURSOR_L); // 光标低地址
    out_byte(CRT_DATA_REG, ((pos - MEM_BASE) >> 1) & 0xff);
}

void console_clear()
{
    screen = MEM_BASE;
    pos = MEM_BASE;
    x = y = 0;
    set_cursor();
    set_screen();

    u16 *ptr = (u16 *)MEM_BASE;
    // 将所有的内容设置为空格
    while (ptr < MEM_END)
    {
        *ptr++ = erase;
    }
}

// 向上滚动
static void scroll_up()
{
    if (screen + SCR_SIZE + ROW_SIZE >= MEM_END)
    {
        memcpy(MEM_BASE, (void *)screen, SCR_SIZE);
        pos -= (screen - MEM_BASE); // 光标移动到对应位置
        screen = MEM_BASE;
    }

    u16 *ptr = (u16 *)(screen + SCR_SIZE); // 没有冒出来的那行清空
    for (size_t i = 0; i < WIDTH; i++)
    {
        *ptr++ = erase;
    }
    screen += ROW_SIZE;
    pos += ROW_SIZE;
    set_screen();
}

// 换行，但是x坐标不变
static void command_lf()
{
    if (y + 1 < HEIGHT)
    {
        y++;
        pos += ROW_SIZE;
        return;
    }
    scroll_up();
}

static void command_cr()
{
    pos -= (x << 1);
    x = 0;
}

static void command_bs()
{
    if (x)
    {
        x--;
        pos -= 2;
        *(u16 *)pos = erase; // 设置前一个字符为空格
    }
}

static void command_del()
{
    *(u16 *)pos = erase;
}

extern void start_beep();

int32 console_write(void *dev, char *buf, u32 count)
{
    // 关闭中断
    bool intr = interrupt_disable();

    char ch;
    int32 nr = 0;
    while (nr++ < count)
    {
        ch = *buf++;
        switch (ch)
        {
        case ASCII_NULL:
            /* code */
            break;
        case ASCII_BEL:
            start_beep();
            break;
        case ASCII_BS:
            command_bs();
            break;
        case ASCII_HT:
            /* code */
            break;
        case ASCII_LF:
            command_lf();
            command_cr();
            break;
        case ASCII_VT:
            /* code */
            break;
        case ASCII_FF:
            command_lf();
            break;
        case ASCII_CR:
            command_cr();
            break;
        case ASCII_DEL:
            command_del();
            break;
        default:
            if (x >= WIDTH)
            {
                x -= WIDTH;
                pos -= ROW_SIZE;
                command_lf();
            }

            // 当前光标设置字符
            *((char *)pos) = ch;
            pos++;
            // 当前光标设置样式
            *((char *)pos) = attr;
            pos++;
            x++;
            break;
        }
    }
    set_cursor();

    // 打开中断
    set_interrupt_state(intr);
    return nr;
}

void console_init()
{
    console_clear();

    device_install(DEV_CHAR, DEV_CONSOLE, NULL, "console", 0, NULL, NULL, console_write);
}
