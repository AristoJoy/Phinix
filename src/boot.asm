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

xchg bx, bx

mov si, booting
call print


; 阻塞, $表达当前行
jmp $

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

; 用0填充剩余区域, 512 减去末尾2字节，$$表示开始位置
times 510 - ($- $$) db 0

; 主引导扇区的最后两个字节必须是,0x55 0xaa 或者dw 0xaa55，小端存储
db 0x55, 0xaa