THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
I386_MAKEFILE := $(THIS_DIR)/i386/Makefile
AMD64_MAKEFILE := $(THIS_DIR)/../Makefile

I386_SOURCE_DIR := $(THIS_DIR)/i386
AMD64_SOURCE_DIR := $(THIS_DIR)/..

.PHONY: all clean boot

all: bootloader.bin

builddir:
	mkdir -p $(THIS_DIR)/build

mbr.bin IA32Start.o IA32Entry.o:
	$(MAKE) -f $(I386_MAKEFILE) SOURCE_DIR=$(I386_SOURCE_DIR) OUTPUT_DIR=$(I386_SOURCE_DIR)/build $@

amd64entry.bin:
	$(MAKE) -f $(AMD64_MAKEFILE) SOURCE_DIR=$(AMD64_SOURCE_DIR) OUTPUT_DIR=$(AMD64_SOURCE_DIR)/build $@

AMD64Blob.o: amd64entry.bin builddir
	cp $(AMD64_SOURCE_DIR)/build/amd64entry.bin $(THIS_DIR)/amd64entry.bin
	cd $(THIS_DIR) && objcopy -I binary -O elf32-i386 -B i386 amd64entry.bin build/AMD64Blob.o

kernelloader.bin: IA32Start.o IA32Entry.o AMD64Blob.o kernelloader.ld builddir
	ld -m elf_i386 -N -e _start -T kernelloader.ld --oformat binary -o $(THIS_DIR)/build/kernelloader.bin $(I386_SOURCE_DIR)/build/IA32Start.o $(I386_SOURCE_DIR)/build/IA32Entry.o $(THIS_DIR)/build/AMD64Blob.o

bootloader.bin: mbr.bin kernelloader.bin builddir
	cat $(I386_SOURCE_DIR)/build/mbr.bin $(THIS_DIR)/build/kernelloader.bin > $(THIS_DIR)/build/bootloader.bin
	@size=$$(stat -c%s $(THIS_DIR)/build/bootloader.bin); \
	pad=$$(( (512 - ($$size % 512)) % 512 )); \
	if [ $$pad -ne 0 ]; then dd if=/dev/zero bs=1 count=$$pad >> $(THIS_DIR)/build/bootloader.bin 2>/dev/null; fi
	if [ -f $(THIS_DIR)/boot ]; then cat $(THIS_DIR)/boot >> $(THIS_DIR)/build/bootloader.bin; fi

boot: clean bootloader.bin builddir
	dd if=/dev/zero of=$(THIS_DIR)/build/bootloader.img bs=1M count=128
	dd if=$(THIS_DIR)/build/bootloader.bin of=$(THIS_DIR)/build/bootloader.img conv=notrunc
	qemu-system-x86_64 -cpu host -drive file=$(THIS_DIR)/build/bootloader.img,format=raw,if=none,id=drive0 -device ich9-ahci,id=ahci -device ide-hd,bus=ahci.0,drive=drive0 -boot c -display sdl -accel kvm -smp 8 -m 16G -no-reboot -no-shutdown

clean:
	$(MAKE) -f $(I386_MAKEFILE) SOURCE_DIR=$(I386_SOURCE_DIR) OUTPUT_DIR=$(I386_SOURCE_DIR)/build clean
	$(MAKE) -f $(AMD64_MAKEFILE) SOURCE_DIR=$(AMD64_SOURCE_DIR) OUTPUT_DIR=$(AMD64_SOURCE_DIR)/build clean
	rm -f $(THIS_DIR)/amd64entry.bin
	rm -f AMD64Blob.o kernelloader.bin bootloader.bin bootloader.img
