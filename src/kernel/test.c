#include <phinix/types.h>
#include <phinix/cpu.h>
#include <phinix/printk.h>
#include <phinix/debug.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

err_t sys_test()
{
    // LOGK("test syscall ...\n");

    // cpu_vendor_t vendor;

    // cpu_vendor_id(&vendor);
    // printk("CPU vendor id: %s\n", vendor.info);
    // printk("CPU max value: 0x%x\n", vendor.max_value);

    // cpu_version_t ver;
    // cpu_version(&ver);
    // printk("FPU support state : %d\n", ver.FPU);
    // printk("APIC support state : %d\n", ver.APIC);

    // 输出二进制
    LOGK("%08b\n", 0x5a);
    // 输出 mac 地址
    LOGK("%m\n", "\x11\x22\x03\x04\x5f\x5a");
    // 输出 ip 地址
    LOGK("%r\n", "\xff\x4e\x03\x04");
    return EOK;
}
