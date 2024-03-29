BUILD:=../build
SRC:=.

MULTIBOOT2:=0x10000
ENTRYPOINT:=$(shell python -c "print(f'0x{$(MULTIBOOT2) + 64:x}')")

CFLAGS:= -m32
CFLAGS+= -march=pentium # pentium 处理器
CFLAGS+= -fno-builtin # 不需要 gcc 内置函数
CFLAGS+= -nostdinc   # 不需要  标准头文件
CFLAGS+= -fno-pic     # 不需要位置无关的代码 position independent code
CFLAGS+= -fno-pie     # 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib   # 不需要 gcc 标准库
CFLAGS+= -fno-stack-protector   # 不需要栈保护
CFLAGS+= -DPHINIX # 定义PHINIX
CFLAGS+= -DPHINIX_DEBUG # 定义PHINIX_DEBUG
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g

INCLUDE:=-I$(SRC)/include

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/boot/loader.bin: $(SRC)/boot/loader.asm $(BUILD)/system.bin
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@ -DKERNEL_SIZE=$$(stat -c%s "$(BUILD)/system.bin")

$(BUILD)/%.o: $(SRC)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $(DEBUG) $< -o $@

$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD)/lib/libc.o: $(BUILD)/lib/crt.o \
	$(BUILD)/lib/crt1.o \
	$(BUILD)/lib/string.o \
	$(BUILD)/lib/vsprintf.o \
	$(BUILD)/lib/stdlib.o \
	$(BUILD)/lib/syscall.o \
	$(BUILD)/lib/printf.o \
	$(BUILD)/lib/assert.o \
	$(BUILD)/lib/time.o \
	$(BUILD)/lib/restorer.o \
	$(BUILD)/lib/math.o \

	ld -m elf_i386 -r $^ -o $@

BUILTIN_APPS := \
	$(BUILD)/builtin/init.out \
	$(BUILD)/builtin/osh.out \
	$(BUILD)/builtin/env.out \
	$(BUILD)/builtin/echo.out \
	$(BUILD)/builtin/cat.out \
	$(BUILD)/builtin/ls.out \
	$(BUILD)/builtin/kill.out \
	$(BUILD)/builtin/dup.out \
	$(BUILD)/builtin/err.out \
	$(BUILD)/builtin/count.out \
	$(BUILD)/builtin/alarm.out \
	$(BUILD)/builtin/float.out \
	$(BUILD)/builtin/player.out \

$(BUILD)/builtin/%.out: $(BUILD)/builtin/%.o \
	$(BUILD)/lib/libc.o \
	
	ld -m elf_i386 -static $^ -o $@ -Ttext 0x1001000

LDFLAGS:= -m elf_i386 \
		-static  \
		-Ttext $(ENTRYPOINT)\
		--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})

$(BUILD)/kernel.bin: \
	$(BUILD)/kernel/start.o \
	$(BUILD)/kernel/main.o \
	$(BUILD)/kernel/io.o  \
	$(BUILD)/kernel/mio.o  \
	$(BUILD)/kernel/device.o  \
	$(BUILD)/kernel/console.o  \
	$(BUILD)/kernel/printk.o  \
	$(BUILD)/kernel/assert.o  \
	$(BUILD)/kernel/debug.o  \
	$(BUILD)/kernel/gdt.o  \
	$(BUILD)/kernel/gate.o  \
	$(BUILD)/kernel/schedule.o  \
	$(BUILD)/kernel/interrupt.o  \
	$(BUILD)/kernel/handler.o  \
	$(BUILD)/kernel/task.o  \
	$(BUILD)/kernel/init.o  \
	$(BUILD)/kernel/idle.o  \
	$(BUILD)/kernel/mutex.o  \
	$(BUILD)/kernel/clock.o  \
	$(BUILD)/kernel/timer.o  \
	$(BUILD)/kernel/time.o  \
	$(BUILD)/kernel/rtc.o  \
	$(BUILD)/kernel/ramdisk.o \
	$(BUILD)/kernel/ide.o  \
	$(BUILD)/kernel/serial.o  \
	$(BUILD)/kernel/memory.o \
	$(BUILD)/kernel/arena.o \
	$(BUILD)/kernel/keyboard.o \
	$(BUILD)/kernel/tty.o \
	$(BUILD)/kernel/buffer.o \
	$(BUILD)/kernel/system.o \
	$(BUILD)/kernel/execve.o \
	$(BUILD)/kernel/signal.o \
	$(BUILD)/kernel/alarm.o \
	$(BUILD)/kernel/cpu.o \
	$(BUILD)/kernel/fpu.o \
	$(BUILD)/kernel/test.o \
	$(BUILD)/kernel/isa.o \
	$(BUILD)/kernel/sb16.o \
	$(BUILD)/kernel/floppy.o \
	$(BUILD)/kernel/pci.o \
	$(BUILD)/kernel/e1000.o \
	$(BUILD)/net/pbuf.o \
	$(BUILD)/net/chksum.o \
	$(BUILD)/net/netif.o \
	$(BUILD)/net/addr.o \
	$(BUILD)/net/eth.o \
	$(BUILD)/net/arp.o \
	$(BUILD)/net/ip.o \
	$(BUILD)/net/icmp.o \
	$(BUILD)/fs/super.o \
	$(BUILD)/fs/bmap.o \
	$(BUILD)/fs/inode.o \
	$(BUILD)/fs/namei.o \
	$(BUILD)/fs/file.o \
	$(BUILD)/fs/stat.o \
	$(BUILD)/fs/dev.o \
	$(BUILD)/fs/pipe.o \
	$(BUILD)/fs/ioctl.o \
	$(BUILD)/lib/bitmap.o \
	$(BUILD)/lib/string.o  \
	$(BUILD)/lib/vsprintf.o  \
	$(BUILD)/lib/stdlib.o  \
	$(BUILD)/lib/syscall.o  \
	$(BUILD)/lib/list.o  \
	$(BUILD)/lib/fifo.o  \
	$(BUILD)/lib/printf.o  \
	$(BUILD)/lib/math.o  \
	$(BUILD)/lib/restorer.o  \
	$(BUILD)/builtin/osh.o  \

	$(shell mkdir -p $(dir $@))
	ld ${LDFLAGS} $^ -o $@ 

$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

include utils/image.mk
include utils/cdrom.mk
include utils/net.mk
include utils/cmd.mk

.PHONY: clean
clean:
	rm -rf $(BUILD)
