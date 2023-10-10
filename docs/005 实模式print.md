# 实模式print

打印字符中断调用：

> ah : 0x0e
> al : 字符
> int 0x10

## bochs魔术断点

使用方法：

```s
xchg bx, bx
```

配置：

```ini
magic_break: enabled=1
```

## 打印实例

```s
xchg bx, bx

mov si, booting
call print

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
```