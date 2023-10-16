[bits 32]

section .text

global in_byte ; 将in_byte导出
in_byte:
    push ebp
    mov ebp, esp   ; 保持栈帧

    xor eax, eax ; 将eax 清空
    mov edx, [ebp + 8] ; port
    in al, dx ; 将端口dx的8bit输入到al

    jmp $+2     ; 一点延迟
    jmp $+2
    jmp $+2

    leave   ; 恢复栈帧
    ret

global out_byte ; 将out_byte导出
out_byte:
    push ebp
    mov ebp, esp   ; 保持栈帧

    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; value
    out dx, al ; 将al的8bit输出到端口

    jmp $+2     ; 一点延迟
    jmp $+2
    jmp $+2

    leave   ; 恢复栈帧
    ret


global in_word ; 将in_word导出
in_word:
    push ebp
    mov ebp, esp   ; 保持栈帧

    xor eax, eax ; 将eax 清空
    mov edx, [ebp + 8] ; port
    in ax, dx ; 将端口dx的16bit输入到ax

    jmp $+2     ; 一点延迟
    jmp $+2
    jmp $+2

    leave   ; 恢复栈帧
    ret

global out_word ; 将out_word导出
out_word:
    push ebp
    mov ebp, esp   ; 保持栈帧

    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; value
    out dx, ax ; 将ax的16bit输出到端口

    jmp $+2     ; 一点延迟
    jmp $+2
    jmp $+2

    leave   ; 恢复栈帧
    ret