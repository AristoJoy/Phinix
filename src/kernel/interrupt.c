#include <phinix/interrupt.h>
#include <phinix/gdt.h>
#include <phinix/debug.h>
#include <phinix/printk.h>
#include <phinix/stdlib.h>
#include <phinix/io.h>
#include <phinix/assert.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ENTRY_SIZE 0X30

#define PIC_M_CTRL 0x20 //  主片的控制端口
#define PIC_M_DATA 0x21 //  主片的数据端口
#define PIC_S_CTRL 0xa0 //  从片的控制端口
#define PIC_S_DATA 0xa1 //  从片的数据端口
#define PIC_EOI 0x20    //  通知中断控制器中断结束

gate_t idt[IDT_SIZE];
descriptor_ptr_t idt_ptr;

extern void interrupt_handler();

handler_t handler_table[IDT_SIZE];
extern handler_t handler_entry_table[ENTRY_SIZE];
extern void syscall_handler();
extern void page_fault();

static char *messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

/**
 * @brief 发送中断结束指令
 *
 * @param vector 中断向量号
 */
void send_eoi(int vector)
{
    if (vector >= 0x20 && vector < 0x28)
    {
        out_byte(PIC_M_CTRL, PIC_EOI);
    }
    else if (vector >= 0x28 && vector < 0x30)
    {
        out_byte(PIC_M_CTRL, PIC_EOI);
        out_byte(PIC_S_CTRL, PIC_EOI);
    }
}

void set_interrupt_handler(u32 irq, handler_t handler)
{
    assert(irq >= 0 && irq < 16);
    handler_table[IRQ_MASTER_NR + irq] = handler;
}

void set_interrupt_mask(u32 irq, bool enable)
{
    assert(irq >= 0 && irq < 16);
    u16 port;
    if (irq < 8)
    {
        port = PIC_M_DATA;
    }
    else
    {
        port = PIC_S_DATA;
        irq -= 8;
    }
    if (enable)
    {
        out_byte(port, in_byte(port) & ~(1 << irq));
    }
    else
    {
        out_byte(port, in_byte(port) | (1 << irq));
    }
}

// 清除eflags IF位，并返回设置之前的值
bool interrupt_disable()
{
    asm volatile(
        "pushfl\n"        // 将eflags压入栈
        "cli\n"           // 清除IF位 此时外中断已被屏蔽
        "popl %eax\n"     // 弹出eflags到eax
        "shrl $9, %eax\n" // 将eax右移9位，得到IF位
        "andl $1, %eax\n" // 清除其他位
    );
}

// 获得IF位
bool get_interrupt_state()
{
    asm volatile(
        "pushfl\n"        // 将eflags压入栈
        "popl %eax\n"     // 弹出eflags到eax
        "shrl $9, %eax\n" // 将eax右移9位，得到IF位
        "andl $1, %eax\n" // 清除其他位
    );
}

// 设置IF位
void set_interrupt_state(bool state)
{
    if (state)
    {
        asm volatile("sti\n");
    }
    else
    {
        asm volatile("cli\n");
    }
}

void default_handler(int vector)
{
    send_eoi(vector);
    DEBUGK("[%x] default interrupt called\n", vector);
}

void 
exception_handler(
    int vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    char *msg = NULL;
    if (vector < 32)
    {
        msg = messages[vector];
    }
    else
    {
        msg = messages[15];
    }

    printk("\nEXCEPTION : %s \n", msg);
    printk("   VECTOR : 0x%02X\n", vector);
    printk("    ERROR : 0x%08X\n", error);
    printk("   EFLAGS : 0x%08X\n", eflags);
    printk("       CS : 0x%02X\n", cs);
    printk("      EIP : 0x%08X\n", eip);
    printk("      ESP : 0x%08X\n", esp);

    bool hanging = true;

    // 阻塞
    while (hanging)
        ;

    // 通过 EIP 的值应该可以找到出错的位置
    // 也可以在出错时，可以将 hanging 在调试器中手动设置为 0
    // 然后在下面 return 打断点，单步调试，找到出错的位置
    return;
}

/**
 * @brief 初始化中断控制器
 *
 */
void pic_init()
{
    out_byte(PIC_M_CTRL, 0b00010001); // ICW1: 边缘触发， 级联8259，需要ICW4
    out_byte(PIC_M_DATA, 0X20);       // ICW2: 起始中断向量号,0x20
    out_byte(PIC_M_DATA, 0b00000100); // ICW3: IR2接从片
    out_byte(PIC_M_DATA, 0b00000001); // ICW4: 8086模式，正常EOI

    out_byte(PIC_S_CTRL, 0b00010001); // ICW1: 边缘触发， 级联8259，需要ICW4
    out_byte(PIC_S_DATA, 0X28);       // ICW2: 起始中断向量号,0x20
    out_byte(PIC_S_DATA, 2);          // ICW3: 设置从片连接到主片的IR2引脚
    out_byte(PIC_S_DATA, 0b00000001); // ICW4: 8086模式，正常EOI

    out_byte(PIC_M_DATA, 0b11111111); // 关闭所有中断
    out_byte(PIC_S_DATA, 0b11111111); // 关闭所有中断
}

/**
 * @brief 初始化中断描述符
 *
 */
void idt_init()
{
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        gate_t *gate = &idt[i];

        handler_t handler = handler_entry_table[i];

        gate->offset_low = (u32)handler & 0xffff;
        gate->offset_high = ((u32)handler >> 16) & 0xffff;
        gate->selector = 1 << 3; // gdt中代码段
        gate->reserved = 0;      // 保留不用
        gate->type = 0b1110;     // 中断门、
        gate->segment = 0;       // 系统段
        gate->DPL = 0;           // 内核态
        gate->present = 1;       // 内存中
    }

    for (size_t i = 0; i < 0x20; i++)
    {
        handler_table[i] = exception_handler;
    }
    
    // 初始化缺页异常处理程序
    handler_table[0xe] = page_fault;

    for (size_t i = 0x20; i < ENTRY_SIZE; i++)
    {
        handler_table[i] = default_handler;
    }

    // 初始化系统调用
    gate_t *gate = &idt[0x80];
    gate->offset_low = (u32)syscall_handler &0xffff;
    gate->offset_high = ((u32)syscall_handler >> 16) &0xffff;
    gate->selector = 1 << 3; // 代码段
    gate->reserved = 0; // 保留不用
    gate->type = 0b1110; // 中断门
    gate->segment = 0; // 系统段
    gate->DPL = 3; // 用户态
    gate->present = 1;// 有效

    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;

    asm volatile("lidt idt_ptr\n");
}

void interrupt_init()
{
    pic_init();
    idt_init();
}