TOOL_CHAIN = arm-none-eabi-
AS = $(TOOL_CHAIN)as
CC = $(TOOL_CHAIN)gcc
LD = $(TOOL_CHAIN)ld
OBJCOPY = $(TOOL_CHAIN)objcopy
OBJDUMP = $(TOOL_CHAIN)objdump

TARGET_ARCH = -march=armv8-a
CFLAGS = -W -Wall
LDFLAGS = -m armelf -no-undefined $(LIBS)

IMG = ledtape.img
OBJS = boot.o test.o serialled.o pwmfifo.o

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.elf: %.o
	$(LD) $(LDFLAGS) $< -o $@

%.img: %.elf
	$(OBJCOPY) $< -O binary $@

.PHONY: default install clean disas
default: install

ledtape.elf: $(OBJS)
	$(LD) $(LDFLAGS) $+ -o $@

install: $(IMG)
	cp -i $< /media/boot/kernel7.img
	eject /media/boot

clean:
	$(RM) *.o a.out *.pyc ledtape *.elf *.img *~

disas: $(IMG:.img=.elf)
	$(OBJDUMP) -D $<

