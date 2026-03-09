SOURCE_DIR ?= .
OUTPUT_DIR ?= ./build

.PHONY: all clean

all: mbr.bin IA32Start.o IA32Entry.o

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

mbr.bin: $(SOURCE_DIR)/EntryPoint.S | $(OUTPUT_DIR)
	gcc -m32 -c -o $(OUTPUT_DIR)/EntryPoint.o $<
	ld -m elf_i386 -N -e _start -Ttext 0x7c00 --oformat binary -o $(OUTPUT_DIR)/mbr.bin $(OUTPUT_DIR)/EntryPoint.o

IA32Start.o: $(SOURCE_DIR)/IA32Start.S | $(OUTPUT_DIR)
	gcc -m32 -c -o $(OUTPUT_DIR)/IA32Start.o $<

IA32Entry.o: $(SOURCE_DIR)/IA32Entry.c | $(OUTPUT_DIR)
	gcc -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -c -o $(OUTPUT_DIR)/IA32Entry.o $<

clean:
	rm -f $(OUTPUT_DIR)/EntryPoint.o $(OUTPUT_DIR)/IA32Start.o $(OUTPUT_DIR)/IA32Entry.o $(OUTPUT_DIR)/mbr.bin
