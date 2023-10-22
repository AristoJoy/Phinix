#ifndef PHINIX_DEBUG_H
#define PHINIX_DEBUG_H

void debugk(char *file, int line, const char *fmt, ...);

#define BOCHS_MAGIC_BP asm volatile("xchgw %bx, %bx") // bochs magic breakpoint
#define DEBUGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args);

#endif