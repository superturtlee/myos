// bootloader/amd64/block/ahci/ahci.h
#include "../../types.h"

#include "../blockdevice.h"
struct ahci_block_device{
	u8 pci_bus;
	u8 pci_dev;
	u8 pci_func;
	u8 buffer[512];
	u64 current_LBA_in_buffer;//记录当前buffer中存的是什么LBA的数据，如果要读取的LBA不在buffer中，就需要重新读盘
};
int find_ahci_pci(u8* out_bus, u8* out_dev, u8* out_func) ;
int init_ahci_block_device(struct block_device* dev,struct ahci_block_device* ahci_dev, u8 pci_bus, u8 pci_dev, u8 pci_func, u64 size);