[bits 32]

section .text
global _start

extern __libc_start_main
extern _init
extern _fini
extern main

_start:
    xor ebx, ebx; 清除栈底，表示程序开场
    pop esi; 栈顶参数为argc
    mov ecx, esp ; 其次为argv

    and esp, -16 ; 栈对齐，SSE需要16字节对齐
    push eax; 没什么用
    push esp; 用户程序栈最大地址
    push edx; 动态链接器
    push _fini; libc析构函数
    push _init; libc构造函数
    push ecx; argv
    push esi; argc
    push main; 主函数

    call __libc_start_main

    ud2; 程序不可能走到这，不然可能是其他地方有问题