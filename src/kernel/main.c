#include <phinix/types.h>
#include <phinix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

extern void interrupt_init();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void memory_map_init();
extern void mapping_init();
// extern void memory_test();
extern void hang();

void intr_test()
{
    bool intr = interrupt_disable();
    
    // do something

    set_interrupt_state(intr);
}

void kernel_init()
{
    memory_map_init();
    mapping_init();
    interrupt_init();
    // clock_init();
    // time_init();
    // rtc_init();
    // task_init();

    // bitmap_tests();

    // char *ptr = (char*) (0x100000 * 20);
    // ptr[0] = 'a';

    // memory_test();
    // asm volatile("sti");

    bool intr = interrupt_disable();
    set_interrupt_state(true);

    LOGK("%d\n", intr);
    LOGK("%d\n", get_interrupt_state());

    BOCHS_MAGIC_BP;

    intr = interrupt_disable();

    BOCHS_MAGIC_BP;

    set_interrupt_state(true);

    LOGK("%d\n", intr);
    LOGK("%d\n", get_interrupt_state());

    hang();
    return;
}