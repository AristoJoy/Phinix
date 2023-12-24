#include <phinix/time.h>
#include <phinix/debug.h>
#include <phinix/io.h>
#include <phinix/stdlib.h>
#include <phinix/rtc.h>


#define LOGK(fmt, args...) DEBUGK(fmt, ##args);

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

// 下面是CMOS信息的寄存器索引

#define CMOS_SECOND 0x00    // 0 ~ 59
#define CMOS_MINUTE 0x02    // 0 ~ 59
#define CMOS_HOUR 0x04      // 0 ~ 23
#define CMOS_WEEKDAY 0x06   // 1 ~ 7
#define CMOS_DAY 0x07       // 1 ~ 31
#define CMOS_MONTH 0x08     // 1 ~ 12
#define CMOS_YEAR 0x09      // 0 ~ 99
#define CMOS_CENTURY 0x32    // 可能不存在
#define CMOS_NMI 0x80


#define MINUTE 60           // 每分钟的秒数
#define HOUR (60 * MINUTE)  // 每小时的秒数
#define DAY (24 * HOUR)     // 每天的秒数
#define YEAR (365 * DAY)    // 每年的秒数

// 每个月开始时已经过去的天数
static int month[13] = {
    0, // 占位，从1月开始
    0,
    (31),
    (31 + 29),
    (31 + 29 + 31),
    (31 + 29 + 31 + 30),
    (31 + 29 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
};

time_t startup_time;
int century;

int get_yday(tm *time)
{
    int res = month[time->mon];
    res += time->mday;

    int year;
    if (time->year >= 70)
        year = time->year - 70;
    else
        year = time->year -70 + 100;
    
    // 如果不是闰年，并且二月已经过去了，则减去一天
    // 注： 1972年是润年，这样算不太精确，忽略了100年的平年
    if ((year + 2) % 4 && time->mon > 2)
    {
        res -= 1;
    }

    return res;
}

/**
 * @brief CMOS的访问速度很慢，为了减小时间误差，在读取了下面循环中所有数值后，
 * 若此时CMOS中的秒值发生了变化，那么就重新读取所有值。
 * 这样内核就能与CMOS的时间误差缩小在1秒之内
 * @param time 
 */
void time_read_bcd(tm *time)
{
    do
    {
        time->sec  = cmos_read(CMOS_SECOND);
        time->min  = cmos_read(CMOS_MINUTE);
        time->hour = cmos_read(CMOS_HOUR);
        time->wday = cmos_read(CMOS_WEEKDAY);
        time->mday = cmos_read(CMOS_DAY);
        time->mon  = cmos_read(CMOS_MONTH);
        time->year = cmos_read(CMOS_YEAR);
        century = cmos_read(CMOS_CENTURY);
    } while (time-> sec != cmos_read(CMOS_SECOND));
    
}

void time_read(tm *time)
{
    time_read_bcd(time);
    time->sec = bcd_to_bin(time->sec);
    time->min = bcd_to_bin(time->min);
    time->hour = bcd_to_bin(time->hour);
    time->wday = bcd_to_bin(time->wday);
    time->mday = bcd_to_bin(time->mday);
    time->mon = bcd_to_bin(time->mon);
    time->year = bcd_to_bin(time->year);
    time->yday = get_yday(time);
    time->isdst = -1;
    century = bcd_to_bin(century);
}

int elapsed_leap_years(int year)
{
    int result = 0;
    result += (year - 1) / 4;
    result -= (year - 1) / 100;
    result += (year + 299) / 400;
    result -= (1970 - 1900) / 4;
    return result;
}

bool is_leap_year(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || ((year + 1900) % 400 == 0);
}

void localtime(time_t stamp, tm *time)
{
    time->sec = stamp % 60;

    time_t remain = stamp / 60;

    time->min = remain % 60;
    remain /= 60;

    time->hour = remain % 24;
    time_t days = remain / 24;

    time->wday = (days + 4) % 7; // 1970-01-01 是星期四

    // 这里产生误差显然需要 365 个闰年，不管了
    int years = days / 365 + 70;
    time->year = years;
    int offset = 1;
    if (is_leap_year(years))
        offset = 0;

    days -= elapsed_leap_years(years);
    time->yday = days % (366 - offset);

    int mon = 1;
    for (; mon < 13; mon++)
    {
        if ((month[mon] - offset) > time->yday)
            break;
    }

    time->mon = mon - 1;
    time->mday = time->yday - month[time->mon] + offset + 1;
}

// 这里生成的时间可能和UTC时间有出入
// 与系统具体的时区相关，不过也不要紧，顶多差几个小时
time_t mktime(tm *time)
{
    time_t res;
    int year; // 1970年开始的年数
    if (time->year >= 70)
    {
        year = time->year - 70;
    }
    else
    {
        year = time->year - 70 + 100;
    }

    // 这些年经过的秒数
    res = YEAR * year;

    // 已经过去的闰年，每一个加一天
    res += DAY * ((year + 1) / 4);

    // 已经过去的月份的时间
    res += month[time->mon] * DAY;

    // 如果二月已经过了，并且当你不是闰年，那么减去一天
    if (time->mon > 2 && ((year + 2) % 4))
    {
        res -= DAY;
    }

    // 这个月已经过去的填
    res += DAY * ((time->mday - 1));

    // 当天过去的小时
    res += HOUR * time->hour;

    // 这个小时过去的分钟
    res += MINUTE * time->min;

    // 这个分钟过去的秒数
    res += time->sec;
    
    return res;
}

void time_init()
{
    tm time;
    time_read(&time);
    startup_time = mktime(&time);
    LOGK("startup time : %d%d-%02d-%02d %02d:%02d:%02d\n",
        century,
        time.year,
        time.mon,
        time.mday,
        time.hour,
        time.min,
        time.sec);
}

