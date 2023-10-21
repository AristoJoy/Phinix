#ifndef PHINIX_PRINTK_H
#define PHINIX_PRINTK_H

#include <phinix/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);

#endif