#include <phinix/time.h>
#include <phinix/stdlib.h>

#define MINUTE 60          // 每分钟的秒数
#define HOUR (60 * MINUTE) // 每小时的秒数
#define DAY (24 * HOUR)    // 每天的秒数
#define YEAR (365 * DAY)   // 每年的秒数，以 365 天算

// 每个月开始时的已经过去天数
static int month[13] = {
    0, // 这里占位，没有 0 月，从 1 月开始
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
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};

time_t startup_time;
int century;

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