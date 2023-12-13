#ifndef PHINIX_STDIO_H
#define PHINIX_STDIO_H

#include <phinix/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif