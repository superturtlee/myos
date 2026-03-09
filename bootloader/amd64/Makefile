SOURCE_DIR ?= .
OUTPUT_DIR ?= ./build

AMD64_ENTRY_OBJ := $(OUTPUT_DIR)/AMD64Entry64.o
AMD64_AHCI_OBJ := $(OUTPUT_DIR)/AMD64AHCI64.o
AMD64_BOOTFS_OBJ := $(OUTPUT_DIR)/AMD64BOOTFS64.o
AMD64_VGA_OBJ := $(OUTPUT_DIR)/AMD64VGASCREEN64.o
AMD64_ELF := $(OUTPUT_DIR)/amd64entry64.elf

.PHONY: all clean

all: amd64entry.bin

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(AMD64_ENTRY_OBJ): $(SOURCE_DIR)/AMD64Entry.c | $(OUTPUT_DIR)
	gcc -m64 -ffreestanding -fpie -fno-stack-protector -c -o $@ $<

$(AMD64_AHCI_OBJ): $(SOURCE_DIR)/block/ahci/ahci.c $(SOURCE_DIR)/block/ahci/ahci.h | $(OUTPUT_DIR)
	gcc -m64 -ffreestanding -fpie -fno-stack-protector -c -o $@ $(SOURCE_DIR)/block/ahci/ahci.c

$(AMD64_BOOTFS_OBJ): $(SOURCE_DIR)/fs/bootfs.c $(SOURCE_DIR)/fs/bootfs.h | $(OUTPUT_DIR)
	gcc -m64 -ffreestanding -fpie -fno-stack-protector -c -o $@ $(SOURCE_DIR)/fs/bootfs.c

$(AMD64_VGA_OBJ): $(SOURCE_DIR)/vgascreen.c $(SOURCE_DIR)/vgascreen.h | $(OUTPUT_DIR)
	gcc -m64 -ffreestanding -fpie -fno-stack-protector -c -o $@ $(SOURCE_DIR)/vgascreen.c

$(AMD64_ELF): $(AMD64_ENTRY_OBJ) $(AMD64_AHCI_OBJ) $(AMD64_BOOTFS_OBJ) $(AMD64_VGA_OBJ)
	ld -m elf_x86_64 -N -e AMD64_entry -Ttext 0 -o $@ $(AMD64_ENTRY_OBJ) $(AMD64_AHCI_OBJ) $(AMD64_BOOTFS_OBJ) $(AMD64_VGA_OBJ)

amd64entry.bin: $(AMD64_ELF)
	objcopy -O binary -j .text -j .rodata -j .data $(AMD64_ELF) $(OUTPUT_DIR)/amd64entry.bin

clean:
	rm -f $(AMD64_ENTRY_OBJ) $(AMD64_AHCI_OBJ) $(AMD64_BOOTFS_OBJ) $(AMD64_VGA_OBJ) $(AMD64_ELF) $(OUTPUT_DIR)/amd64entry.bin
