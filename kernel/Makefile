CFLAGS = -m64 -ffreestanding -fpie -fno-stack-protector -I./mem

SRCS = kernelmain.c mem/page.c mem/sysmap.c charoutput/vgascreen.c charoutput/charoutput.c
OBJS = $(SRCS:.c=.o)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS)
	ld -m elf_x86_64 -N -e kmain -Ttext 0 -o kernel.elf $(OBJS)

kernel.bin: clean kernel.elf
	objcopy -O binary -j .text -j .rodata -j .data kernel.elf kernel.bin

bootfs: kernel.bin
	../utils/bootfs/mkbootfs ../bootloader/amd64/mbr/boot kernel.bin

boot: bootfs
	cd ../bootloader/amd64/mbr && make boot

clean:
	rm -f $(OBJS) kernel.elf kernel.bin