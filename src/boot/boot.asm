[org 0x7c00]

; 设置屏幕模式为文本模式，清除屏幕
mov ax, 3
int 0x10

; 初始化段寄存器

mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00



mov si, booting
call print


mov edi, 0x1000 ; 读取的目标内存
mov ecx, 2 ; 起始扇区
mov bl, 4; 扇区数量

call read_disk


cmp word [0x1000], 0x55aa
jnz error

jmp 0:0x1008

; 阻塞, $表达当前行
jmp $

read_disk:
    ; 设置读写扇区的数量
    mov dx, 0x1f2
    mov al, bl
    out dx, al ; 只可以用dx和ax寄存器

    inc dx ; 0x1f3
    mov al, cl ; 起始扇区0~7位
    out dx, al

    inc dx ; 0x1f4
    shr ecx, 8 ; 右移8位
    mov al, cl ; 起始扇区8~15位
    out dx, al

    inc dx ; 0x1f5
    shr ecx, 8 ; 右移8位
    mov al, cl ; 起始扇区16~23位
    out dx, al

    inc dx ; 0x1f6
    shr ecx, 8 ; 右移8位
    and cl, 0xf ; 取低4位，高4位清零，起始扇区24~27位
    mov al, 0b1110_0000; 主盘， LBA模式
    or al, cl 
    out dx, al

    inc dx ; 0x1f7
    mov al, 0x20 ; 读取硬盘命令
    out dx, al

    xor ecx, ecx
    mov cl, bl ; 读取扇区的数量

    .read:
        push cx ; 里面函数修改了cx
        call .waits ; 等待一个扇区数据准备完毕
        call .reads ; 读取一个扇区
        pop cx
        loop .read
    
    ret

    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx ; 读取数据到al寄存器
            jmp $+2 ; nop 直接跳到下一行， 一点延迟
            jmp $+2
            jmp $+2
            and al, 0b1000_1000 ; 只检查数据准备完或者硬盘是否繁忙，不处理错误，默认没有错
            cmp al, 0b0000_1000 ; 0时，硬盘准备完毕，否则磁盘繁忙
            jnz .check
        ret
    .reads:
        mov dx, 0x1f0
        mov cx, 256 ; 一个扇区256个字
        .readw:     ; 读取一个字
            in ax, dx
            jmp $+2 ; nop 直接跳到下一行， 一点延迟
            jmp $+2
            jmp $+2
            mov [edi], ax
            add edi, 2
            loop .readw
        ret

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

booting:
    db "Booting Phinix...", 10, 13, 0 ; \n\r

error:
    mov si, .msg
    call print
    hlt ; 停机
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0 ; \n\r

; 用0填充剩余区域, 512 减去末尾2字节，$$表示开始位置
times 510 - ($- $$) db 0

; 主引导扇区的最后两个字节必须是,0x55 0xaa 或者dw 0xaa55，小端存储
db 0x55, 0xaa