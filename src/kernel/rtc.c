#include <phinix/types.h>
#include <phinix/io.h>
#include <phinix/rtc.h>
#include <phinix/debug.h>
#include <phinix/interrupt.h>
#include <phinix/time.h>
#include <phinix/assert.h>
#include <phinix/stdlib.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

// 下面是CMOS信息的寄存器索引

#define CMOS_SECOND 0x01  // 0 ~ 59
#define CMOS_MINUTE 0x03  // 0 ~ 59
#define CMOS_HOUR 0x5    // 0 ~ 23

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80


u8 cmos_read(u8 addr)
{
    out_byte(CMOS_ADDR, CMOS_NMI | addr);
    return in_byte(CMOS_DATA);
}

void cmos_write(u8 addr, u8 value)
{
    out_byte(CMOS_ADDR, CMOS_NMI | addr);
    out_byte(CMOS_DATA, value);
}

// static u32 volatile counter = 0;
extern void start_beep();

void rtc_handler(int vector)
{
    // 实时时钟中断向量号
    assert(vector == 0x28);

    send_eoi(vector);

    // 读CMOS 寄存器c，运行CMOS继续产生中断
    // cmos_read(CMOS_C);

    // set_alarm(1);

    // LOGK("rtc handler %d...\n", counter++);
    start_beep();
}

// 色泽secs秒后发生实时时钟中断

void set_alarm(u32 secs)
{
    tm time;
    time_read(&time);
    
    u8 sec = secs % 60;
    secs /= 60;
    u8 min = secs % 60;
    secs /= 60;
    u8 hour = secs % 60;

    time.sec += sec;
    if (time.sec >= 60)
    {
        time.sec = time.sec % 60;
        time.min += 1;
    }
    time.min += min;
    if (time.min >= 60)
    {
        time.min = time.min % 60;
        time.hour += 1;
    }

    time.hour += hour;
    if (time.hour >= 24)
    {
        time.hour = time.hour % 24;
    }

    cmos_write(CMOS_HOUR, bin_to_bcd(time.hour));
    cmos_write(CMOS_MINUTE, bin_to_bcd(time.min));
    cmos_write(CMOS_SECOND, bin_to_bcd(time.sec));

    cmos_write(CMOS_B, 0b00100010); // 打开闹钟中断
    cmos_read(CMOS_C); // 读c寄存器,以运行CMOS中断
}

void rtc_init()
{
    // cmos_write(CMOS_B, 0b01000010); // 打开周期中断
    // cmos_write(CMOS_B, 0b00100010);     // 打开闹钟中断 
    // cmos_read(CMOS_C); // 读c寄存器，以允许CMOS中断

    // set_alarm(2);

    // 设置中断频率
    // out_byte(CMOS_A, (in_byte(CMOS_A) & 0xf) | 0b1110);
    // hang();

    set_interrupt_handler(IRQ_RTC, rtc_handler);
    set_interrupt_mask(IRQ_RTC, true);
    set_interrupt_mask(IRQ_CASCADE, true);
}