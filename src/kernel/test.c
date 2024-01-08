#include <phinix/types.h>
#include <phinix/cpu.h>
#include <phinix/printk.h>
#include <phinix/debug.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

err_t sys_test()
{
    LOGK("test syscall ...\n");

    cpu_vendor_t vendor;

    cpu_vendor_id(&vendor);
    printk("CPU vendor id: %s\n", vendor.info);
    printk("CPU max value: 0x%x\n", vendor.max_value);

    cpu_version_t ver;
    cpu_version(&ver);
    printk("FPU support state : %d\n", ver.FPU);
    printk("APIC support state : %d\n", ver.APIC);
    return EOK;
}
