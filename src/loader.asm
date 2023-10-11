[org 0x1000]

dw 0x55aa ; 魔数用于判断错误

xchg bx, bx

mov si, loading
call print

xchg bx, bx

detecting:
    xor ebx, ebx

    ; ards缓存地址
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer

    mov ecx, 20
    mov edx, 0x534d4150

.next:
    mov eax, 0xe820
    int 0x15

    jc error ; 出错

    ; 指向下一个结构体
    add di, cx

    ; 将结构体数量加一
    inc word [ards_count]

    cmp ebx, 0
    jnz .next

    mov si, detect_success
    call print

    ; 结构体数量
    mov cx, [ards_count]
    ; 结构体指针
    mov si, 0
.show:
    mov eax, [ards_buffer + si ]
    mov ebx, [ards_buffer + si  + 8]
    mov edx, [ards_buffer + si  + 16]
    add si, 20
    xchg bx, bx
    loop .show

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

ards_count:
    dw 0
ards_buffer: