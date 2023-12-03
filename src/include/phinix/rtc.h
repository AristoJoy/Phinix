#ifndef PHINIX_RTC_H
#define PHINIX_RTC_H

#include <phinix/types.h>

void set_alarm(u32 secs);
u8 cmos_read(u8 addr);
void cmos_write(u8 addr, u8 value);

#endif