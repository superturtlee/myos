typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int u32;
#include "vgascreen.h"
#include "block/ahci/ahci.h"
#include "fs/bootfs.h"
#define KB (1<<10)
#define MB (1<<20)
#define GDT_PHYS 1*MB
#define KERNEL_PHYS 2*MB
char bootfs_magic[] = "BOOTFS!";
int line = 2;
typedef void (*func_ptr_t)(int, int, char**);
void AMD64_entry(void) {//must be the first function in the file, and must be called AMD64_entry, because the entry point is set to AMD64_entry in the linker script
	volatile u16* const vram = (volatile u16*)0xB8000;
	detect_csm();
	puts("\n\nAMD64 Entry Point Reached\n");
	puts("Searching for AHCI Controller...\n");
	u8 pci_bus;
	u8 pci_dev;
	u8 pci_func;
	if (find_ahci_pci(&pci_bus, &pci_dev, &pci_func) != 0) {
		puts("AHCI Controller Not Found\n");
		for (;;) {
			__asm__ __volatile__("hlt");
		}
	}
	//init the ahci block device
	struct block_device ahci_dev;
	struct ahci_block_device ahci_dev_specific;
	if (init_ahci_block_device(&ahci_dev, &ahci_dev_specific, pci_bus, pci_dev, pci_func, 100ULL * 512) != 0) {//前100个扇区，足够存放bootfs了
		puts("Failed to initialize AHCI block device\n");
		for (;;) {
			__asm__ __volatile__("hlt");
		}
	}
	char buffer[512];
	for (int i = 0; i < 100; i++) {
		int rc = 0;
		rc = ahci_dev.read(&ahci_dev, buffer, 512);
		if (rc != 0) {
			puts("AHCI rc=");
			puthex32((u32)rc);
			putc('\n');
			continue;
		}
	//check if the buffer contains the bootfs magic
		int j;
		char found_magic = 1;
		for (j = 0; j < 7; j++) {
			if (buffer[j] != bootfs_magic[j]){
				found_magic = 0; break;}
		}
		if (found_magic) {
			puts("BOOTFS Found at LBA ");
			puthex32((u32)i);
			putc('\n');
			struct bootfs_control ctrl;
			bootfs_init(&ctrl, &ahci_dev, (u64)i * 512);
			int file_size = bootfs_file_size(&ctrl, "kernel.bin");
			if (file_size < 0) {
				puts("Failed to get file size for kernel, rc=");
				puthex32((u32)file_size);
				putc('\n');
			} else {
				puts("File kernel size:");
				puthex32((u32)file_size);
				putc('\n');
				//char file_buffer[512];//this is only a demo later we will load the kernel elf and bootargs.txt
				if (bootfs_read_file(&ctrl, "kernel.bin", (void*)(2*MB), file_size) != 0) {
					puts("Failed to read kernel\n");
				} else {
					func_ptr_t kernel_entry = (func_ptr_t)(2*MB);
					kernel_entry(file_size ,0, 0);
				}
			}

			break;
		}
	}
	for (;;) {
		__asm__ __volatile__("hlt");
	}
}