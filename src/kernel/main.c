#include <phinix/debug.h>

extern void interrupt_init();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void memory_map_init();
extern void mapping_init();
extern void memory_test();
extern void hang();

void kernel_init()
{
    memory_map_init();
    mapping_init();
    interrupt_init();
    // clock_init();
    // time_init();
    // rtc_init();
    // task_init();

    BOCHS_MAGIC_BP;

    char *ptr = (char*) (0x100000 * 20);
    ptr[0] = 'a';

    // memory_test();
    // asm volatile("sti");
    hang();
    return;
}