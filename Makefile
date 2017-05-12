ARCH = arm-none-eabi
AS = $(ARCH)-as
CC = $(ARCH)-gcc
LD = $(ARCH)-ld
CFLAGS = -W -Wall
LDFLAGS = -m armelf -no-undefined $(LIBS)
OBJCOPY = $(ARCH)-objcopy

IMG = ledtape.img
OBJS = boot.o test.o serialled.o pwmfifo.o

%.o: %.s
	$(AS) $< -o $@

%.elf: %.o
	$(LD) $(LDFLAGS) $< -o $@

%.img: %.elf
	$(OBJCOPY) $< -O binary $@

.PHONY: default install clean
default: install

ledtape.elf: $(OBJS)
	$(LD) $(LDFLAGS) $+ -o $@

install: $(IMG)
	cp -i $< /media/boot/kernel7.img
	eject /media/boot

clean:
	$(RM) *.o a.out *.pyc ledtape *.elf *.img *~
