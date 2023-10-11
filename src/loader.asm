[org 0x1000]

dd 0x55aa ; 魔数用于判断错误

; xchg bx, bx

mov si, loading
call print

; xchg bx, bx

detecting:
    xor ebx, ebx

    ; ards缓存地址
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer

    
    mov edx, 0x534d4150 ; 固定签名

.next:
    ; 子功能号
    mov eax, 0xe820
    ; ards 结构的大小 (字节)
    mov ecx, 20
    int 0x15

    jc error ; 出错

    ; 指向下一个结构体
    add di, cx

    ; 将结构体数量加一
    inc dword [ards_count]

    cmp ebx, 0
    jnz .next

    mov si, detect_success
    call print
    
    jmp prepare_protect_mode

prepare_protect_mode:
    xchg bx, bx

    cli ; 关闭中断

    ; 打开A20线
    in al, 0x92
    or al, 0b10
    out 0x92, al

    ; 加载gdt
    lgdt [gdt_ptr]

    ; 打开保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax

     ; 用跳转来刷新缓存，启用保护模式，必须指明是dword
    jmp dword code_selector:protect_mode

print:
    mov ah, 0x0e
.next:
    mov al, [si]
    cmp al, 0
    jz .done
    int 0x10
    inc si
    jmp .next
.done:
    ret

loading:
    db "Loading Phinix...", 10, 13, 0 ; \n\r

detect_success:
    db "Detecting Memory Success...", 10, 13, 0 ; \n\r

error:
    mov si, .msg
    call print
    hlt ; 停机
    jmp $
    .msg db "Loading Error!!!", 10, 13, 0 ; \n\r

[bits 32]
protect_mode:
    xchg bx, bx
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x10000; 修改栈顶

    mov byte [0xb8000], 'P'

    mov byte [0x200000], 0x50

jmp $

; 段选择子, TI = 0 全局描述符，RPL = 0 最高级
code_selector equ (1 << 3)
data_selector equ (2 << 3)

; 内存基地址
memory_base equ 0
; 内存界限
memory_limit equ ((1024 * 1024 * 1024 * 4) / (1024 * 4)) - 1

; gdt指针
gdt_ptr:
    dw (gdt_end - gdt_base) - 1
    dd gdt_base

gdt_base:
    dd 0, 0 ; null描述符
gdt_code:
    dw memory_limit & 0xffff ; 段界限 0 ~ 15 位
    dw memory_base & 0xffff ; 基地址 0 ~ 15 位
    db (memory_base >> 16) & 0xff ; 基地址 16 ~ 23 位
    ; 在内存 - DPL 0 - 代码段 - 代码 - 非依从代码段 - 可读- 未被访问
    db 0b_1_00_1_1_0_1_0
    ; 4KB - 32位 - 非64位 - 0 - limit_high
    db 0b_1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff ; 基地址 24 ~ 31 位
gdt_data:
    dw memory_limit & 0xffff ; 段界限 0 ~ 15 位
    dw memory_base & 0xffff ; 基地址 0 ~ 15 位
    db (memory_base >> 16) & 0xff ; 基地址 16 ~ 23 位
    ; 在内存 - DPL 0 - 代码段 - 数据 - 向上扩展 - 可写- 未被访问
    db 0b_1_00_1_0_0_1_0
    ; 4KB - 32位 - 非64位 - 0 - limit_high
    db 0b_1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff ; 基地址 24 ~ 31 位
gdt_end:

ards_count:
    dd 0
ards_buffer: