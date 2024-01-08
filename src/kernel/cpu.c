#include <phinix/cpu.h>

// cpu cpuid功能核验
bool cpu_check_cpuid()
{
    bool ret;
    asm volatile(
        "pushfl \n" // 保持eflags

        "pushfl \n"                   // 得到eflags
        "xorl $0x00200000, (%%esp)\n" // 反转ID位
        "popfl\n"                     // 写入eflags

        "pushfl\n"                  // 得到eflags
        "popl %%eax\n"              // 写入eax
        "xorl (%%esp), %%eax\n"      // 将写入的值与原值比较
        "andl $0x00200000, %%eax\n" // 得到ID位
        "shrl $21, %%eax\n"         // 右移21位，得到是否支持

        "popfl\n" // 恢复eflags
        : "=a"(ret));
    return ret;
}

// 设置cpu供应商信息
void cpu_vendor_id(cpu_vendor_t *item)
{
    asm volatile(
        "cpuid \n"
        : "=a"(*((u32 *)item + 0)),
          "=b"(*((u32 *)item + 1)),
          "=d"(*((u32 *)item + 2)),
          "=c"(*((u32 *)item + 3))
        : "a"(0));
    item->info[12] = 0;
}

// 设置cpu版本信息
void cpu_version(cpu_version_t *item)
{
    asm volatile(
        "cpuid \n"
        : "=a"(*((u32 *)item + 0)),
          "=b"(*((u32 *)item + 1)),
          "=d"(*((u32 *)item + 2)),
          "=c"(*((u32 *)item + 3))
        : "a"(1));
}