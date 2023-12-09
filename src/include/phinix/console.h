#ifndef PHINIX_CONSOLE_H
#define PHINIX_CONSOLE_H

#include <phinix/types.h>

void console_init();
void console_clear();
int32 console_write(char *buf, u32 count);

#endif