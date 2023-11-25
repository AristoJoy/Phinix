#ifndef PHINIX_TIME_H
#define PHINIX_TIME_H

#include <phinix/types.h>

typedef struct tm
{
    int sec;    // 秒 [0, 59]
    int min;    // 分钟 [0, 59]
    int hour;   // 小时 [0, 23]
    int mday;   // 一个月的天数 [0, 31]
    int mon;    // 一年中的月份 [0, 11]
    int year;   // 从1970开始的年数
    int wday;   // 一星期的某天 [0, 6]，0从星期天开始
    int yday;   // 一年中的某天 [0, 365]
    int isdst;  // 夏令时标志
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time);

#endif