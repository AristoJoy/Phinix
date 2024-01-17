#include <phinix/types.h>
#include <phinix/cpu.h>
#include <phinix/printk.h>
#include <phinix/debug.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

extern void test_e1000_send_packet();

err_t sys_test()
{
    test_e1000_send_packet();
}
