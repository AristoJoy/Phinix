#ifndef PHINIX_STRING_H
#define PHINIX_STRING_H

#include <phinix/types.h>

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
size_t strlen(const char *src);
int strcmp(const char *lhs, const char *rhs);
char *strchr(const char *src, int ch);
char *strrchr(const char *src, int ch);

int memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(const void *dest, int ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memchr(const void *src, int ch, size_t count);

#endif