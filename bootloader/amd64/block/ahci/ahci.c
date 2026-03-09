// bootloader/amd64/block/ahci/ahci.c
//这个驱动对计算机系统这门课程没有参考意义，所以copy了代码
// 该文件实现了AHCI协议的读盘功能
#include "ahci.h"
#define PCI_CONFIG_ADDR 0xCF8U
#define PCI_CONFIG_DATA 0xCFCU

#define AHCI_PCI_BUS 0U
#define AHCI_PCI_DEV 31U
#define AHCI_PCI_FUNC 2U

#define HBA_PxCMD_ST 0x0001U
#define HBA_PxCMD_FRE 0x0010U
#define HBA_PxCMD_FR 0x4000U
#define HBA_PxCMD_CR 0x8000U
#define HBA_GHC_AE (1U << 31)

#define HBA_PxIS_TFES (1U << 30)

#define ATA_DEV_BUSY 0x80U
#define ATA_DEV_DRQ 0x08U

#define FIS_TYPE_REG_H2D 0x27U
#define ATA_CMD_READ_DMA_EX 0x25U

#define SATA_SIG_ATA 0x00000101U

#define AHCI_MEM_BASE 0x00110000ULL
#define AHCI_CMD_LIST_SIZE 0x800U
#define AHCI_FIS_SIZE 0x100U
#define AHCI_CMD_TBL_BASE 0x00130000ULL
#define AHCI_CMD_TBL_PORT_STRIDE 0x2000U
#define AHCI_CMD_TBL_SLOT_STRIDE 0x100U

typedef struct {
	u32 clb;
	u32 clbu;
	u32 fb;
	u32 fbu;
	u32 is;
	u32 ie;
	u32 cmd;
	u32 rsv0;
	u32 tfd;
	u32 sig;
	u32 ssts;
	u32 sctl;
	u32 serr;
	u32 sact;
	u32 ci;
	u32 sntf;
	u32 fbs;
	u32 rsv1[11];
	u32 vendor[4];
} __attribute__((packed)) hba_port_t;

typedef struct {
	u32 cap;
	u32 ghc;
	u32 is;
	u32 pi;
	u32 vs;
	u32 ccc_ctl;
	u32 ccc_pts;
	u32 em_loc;
	u32 em_ctl;
	u32 cap2;
	u32 bohc;
	u8 rsv[0xA0 - 0x2C];
	u8 vendor[0x100 - 0xA0];
	hba_port_t ports[32];
} __attribute__((packed)) hba_mem_t;

typedef struct {
	u8 cfl:5;
	u8 a:1;
	u8 w:1;
	u8 p:1;
	u8 r:1;
	u8 b:1;
	u8 c:1;
	u8 rsv0:1;
	u8 pmp:4;
	u16 prdtl;
	volatile u32 prdbc;
	u32 ctba;
	u32 ctbau;
	u32 rsv1[4];
} __attribute__((packed)) hba_cmd_header_t;

typedef struct {
	u32 dba;
	u32 dbau;
	u32 rsv0;
	u32 dbc:22;
	u32 rsv1:9;
	u32 i:1;
} __attribute__((packed)) hba_prdt_entry_t;

typedef struct {
	u8 cfis[64];
	u8 acmd[16];
	u8 rsv[48];
	hba_prdt_entry_t prdt_entry[1];
} __attribute__((packed)) hba_cmd_tbl_t;

typedef struct {
	u8 fis_type;
	u8 pmport:4;
	u8 rsv0:3;
	u8 c:1;
	u8 command;
	u8 featurel;
	u8 lba0;
	u8 lba1;
	u8 lba2;
	u8 device;
	u8 lba3;
	u8 lba4;
	u8 lba5;
	u8 featureh;
	u8 countl;
	u8 counth;
	u8 icc;
	u8 control;
	u8 rsv1[4];
} __attribute__((packed)) fis_reg_h2d_t;

static inline void outl(u16 port, u32 value) {
	__asm__ __volatile__("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port) {
	u32 value;
	__asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static u32 pci_read32(u8 bus, u8 dev, u8 func, u8 off) {
	u32 addr;
	addr = (1U << 31) | ((u32)bus << 16) | ((u32)dev << 11) | ((u32)func << 8) | ((u32)(off & 0xFCU));
	outl(PCI_CONFIG_ADDR, addr);
	return inl(PCI_CONFIG_DATA);
}

int find_ahci_pci(u8* out_bus, u8* out_dev, u8* out_func) {
	u32 bus;
	for (bus = 0; bus < 256U; bus++) {
		u32 dev;
		for (dev = 0; dev < 32U; dev++) {
			u32 func;
			for (func = 0; func < 8U; func++) {
				u32 vendor_device = pci_read32((u8)bus, (u8)dev, (u8)func, 0x00);
				u32 class_reg;
				u8 class_code;
				u8 subclass;
				u8 prog_if;

				if (vendor_device == 0xFFFFFFFFU) {
					if (func == 0U) {
						break;
					}
					continue;
				}

				class_reg = pci_read32((u8)bus, (u8)dev, (u8)func, 0x08);
				class_code = (u8)((class_reg >> 24) & 0xFFU);
				subclass = (u8)((class_reg >> 16) & 0xFFU);
				prog_if = (u8)((class_reg >> 8) & 0xFFU);

				if (class_code == 0x01U && subclass == 0x06U && prog_if == 0x01U) {
					*out_bus = (u8)bus;
					*out_dev = (u8)dev;
					*out_func = (u8)func;
					return 0;
				}
			}
		}
	}
	return -10;
}

static void pci_write32(u8 bus, u8 dev, u8 func, u8 off, u32 value) {
	u32 addr;
	addr = (1U << 31) | ((u32)bus << 16) | ((u32)dev << 11) | ((u32)func << 8) | ((u32)(off & 0xFCU));
	outl(PCI_CONFIG_ADDR, addr);
	outl(PCI_CONFIG_DATA, value);
}

static void memzero(void* dst, u32 bytes) {
	u8* p = (u8*)dst;
	u32 i;
	for (i = 0; i < bytes; i++) {
		p[i] = 0;
	}
}

static void stop_cmd(hba_port_t* port) {
	port->cmd &= ~HBA_PxCMD_ST;
	port->cmd &= ~HBA_PxCMD_FRE;
	while (port->cmd & (HBA_PxCMD_FR | HBA_PxCMD_CR)) {
	}
}

static void start_cmd(hba_port_t* port) {
	while (port->cmd & HBA_PxCMD_CR) {
	}
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

static int find_cmdslot(hba_port_t* port) {
	u32 slots = port->sact | port->ci;
	u32 i;
	for (i = 0; i < 32U; i++) {
		if ((slots & (1U << i)) == 0U) {
			return (int)i;
		}
	}
	return -1;
}

static int port_is_sata(hba_port_t* port) {
	u32 ssts = port->ssts;
	u8 det = (u8)(ssts & 0x0FU);
	u8 ipm = (u8)((ssts >> 8) & 0x0FU);
	if (det != 3U || ipm == 0U) {
		return 0;
	}
	return port->sig == SATA_SIG_ATA;
}

static int setup_port(hba_mem_t* abar, u32 port_no) {
	hba_port_t* port = &abar->ports[port_no];
	u64 cmd_list_base = AHCI_MEM_BASE + ((u64)port_no * AHCI_CMD_LIST_SIZE);
	u64 fis_base = AHCI_MEM_BASE + 0x10000ULL + ((u64)port_no * AHCI_FIS_SIZE);
	u64 cmd_tbl_port_base = AHCI_CMD_TBL_BASE + ((u64)port_no * AHCI_CMD_TBL_PORT_STRIDE);
	u32 i;

	stop_cmd(port);

	port->clb = (u32)(cmd_list_base & 0xFFFFFFFFULL);
	port->clbu = (u32)(cmd_list_base >> 32);
	port->fb = (u32)(fis_base & 0xFFFFFFFFULL);
	port->fbu = (u32)(fis_base >> 32);

	memzero((void*)(unsigned long)cmd_list_base, AHCI_CMD_LIST_SIZE);
	memzero((void*)(unsigned long)fis_base, AHCI_FIS_SIZE);

	for (i = 0; i < 32U; i++) {
		hba_cmd_header_t* header = &((hba_cmd_header_t*)(unsigned long)cmd_list_base)[i];
		u64 cmd_tbl_base = cmd_tbl_port_base + ((u64)i * AHCI_CMD_TBL_SLOT_STRIDE);
		header->prdtl = 1;
		header->ctba = (u32)(cmd_tbl_base & 0xFFFFFFFFULL);
		header->ctbau = (u32)(cmd_tbl_base >> 32);
		memzero((void*)(unsigned long)cmd_tbl_base, AHCI_CMD_TBL_SLOT_STRIDE);
	}

	port->is = 0xFFFFFFFFU;
	start_cmd(port);
	return 0;
}

static int read_port_lba(hba_port_t* port, u64 lba, u16 sector_count, void* buffer) {
	int slot = find_cmdslot(port);
	u64 cmd_list_base;
	hba_cmd_header_t* cmd_header;
	hba_cmd_tbl_t* cmd_tbl;
	fis_reg_h2d_t* fis;
	u32 spin;

	if (slot < 0) {
		return -3;
	}

	cmd_list_base = ((u64)port->clbu << 32) | port->clb;
	cmd_header = &((hba_cmd_header_t*)(unsigned long)cmd_list_base)[slot];
	cmd_header->cfl = (u8)(sizeof(fis_reg_h2d_t) / sizeof(u32));
	cmd_header->w = 0;
	cmd_header->c = 0;
	cmd_header->prdtl = 1;

	cmd_tbl = (hba_cmd_tbl_t*)(unsigned long)((((u64)cmd_header->ctbau) << 32) | cmd_header->ctba);
	memzero(cmd_tbl, AHCI_CMD_TBL_SLOT_STRIDE);

	cmd_tbl->prdt_entry[0].dba = (u32)((u64)(unsigned long)buffer & 0xFFFFFFFFULL);
	cmd_tbl->prdt_entry[0].dbau = (u32)(((u64)(unsigned long)buffer) >> 32);
	cmd_tbl->prdt_entry[0].dbc = (u32)(((u32)sector_count << 9) - 1U);
	cmd_tbl->prdt_entry[0].i = 1;

	fis = (fis_reg_h2d_t*)(&cmd_tbl->cfis[0]);
	fis->fis_type = FIS_TYPE_REG_H2D;
	fis->c = 1;
	fis->command = ATA_CMD_READ_DMA_EX;
	fis->device = 1U << 6;

	fis->lba0 = (u8)(lba & 0xFFULL);
	fis->lba1 = (u8)((lba >> 8) & 0xFFULL);
	fis->lba2 = (u8)((lba >> 16) & 0xFFULL);
	fis->lba3 = (u8)((lba >> 24) & 0xFFULL);
	fis->lba4 = (u8)((lba >> 32) & 0xFFULL);
	fis->lba5 = (u8)((lba >> 40) & 0xFFULL);

	fis->countl = (u8)(sector_count & 0xFFU);
	fis->counth = (u8)((sector_count >> 8) & 0xFFU);

	for (spin = 0; spin < 1000000U; spin++) {
		if ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) == 0U) {
			break;
		}
	}
	if (spin == 1000000U) {
		return -4;
	}

	port->is = 0xFFFFFFFFU;
	port->ci = 1U << (u32)slot;

	while (1) {
		if ((port->ci & (1U << (u32)slot)) == 0U) {
			break;
		}
		if (port->is & HBA_PxIS_TFES) {
			return -5;
		}
	}

	if (port->is & HBA_PxIS_TFES) {
		return -6;
	}
	return 0;
}

static int readdisk_ahci(u64 lba_start, u64 lba_end, void* buffer, u8 pci_bus, u8 pci_dev, u8 pci_func) {
	u32 device_vendor;
	u32 cmd;
	u32 abar_lo;
	u32 abar_hi;
	u64 abar_phys;
	hba_mem_t* abar;
	u32 i;
	u32 port_no = 0xFFFFFFFFU;
	u64 current_lba;
	u64 total_sectors;
	u8* dst;

	if (buffer == 0) {
		return -1;
	}
	if (lba_end < lba_start) {
		return -2;
	}

	device_vendor = pci_read32(pci_bus, pci_dev, pci_func, 0x00);
	if (device_vendor == 0xFFFFFFFFU) {
		return -7;
	}

	cmd = pci_read32(pci_bus, pci_dev, pci_func, 0x04);
	cmd |= (1U << 1) | (1U << 2);
	pci_write32(pci_bus, pci_dev, pci_func, 0x04, cmd);

	abar_lo = pci_read32(pci_bus, pci_dev, pci_func, 0x24) & 0xFFFFFFF0U;
	abar_hi = pci_read32(pci_bus, pci_dev, pci_func, 0x28);
	abar_phys = ((u64)abar_hi << 32) | abar_lo;
	if (abar_phys == 0ULL) {
		return -8;
	}

	abar = (hba_mem_t*)(unsigned long)abar_phys;
	abar->ghc |= HBA_GHC_AE;

	for (i = 0; i < 32U; i++) {
		if ((abar->pi & (1U << i)) && port_is_sata(&abar->ports[i])) {
			port_no = i;
			break;
		}
	}
	if (port_no == 0xFFFFFFFFU) {
		return -9;
	}

	setup_port(abar, port_no);

	current_lba = lba_start;
	total_sectors = lba_end - lba_start + 1ULL;
	dst = (u8*)buffer;

	while (total_sectors > 0ULL) {
		u16 chunk = (total_sectors > 255ULL) ? 255U : (u16)total_sectors;
		int rc = read_port_lba(&abar->ports[port_no], current_lba, chunk, dst);
		if (rc != 0) {
			return rc;
		}
		current_lba += (u64)chunk;
		dst += ((u32)chunk << 9);
		total_sectors -= (u64)chunk;
	}

	return 0;
}
//below are block device interface

int block_set_cursor(struct block_device* dev, u64 cursor) {
    if (cursor > dev->size) {
        return -1; // out of bounds
    }
    dev->cursor = cursor;
    return 0;
}
int block_getc(struct block_device* dev) {
    if (dev->cursor >= dev->size) {
        return -1; // EOF
    }
    u64 target_LBA = dev->cursor / 512;
    if (target_LBA != ((struct ahci_block_device*)dev->provider)->current_LBA_in_buffer) {
        int rc = readdisk_ahci(target_LBA, target_LBA, ((struct ahci_block_device*)dev->provider)->buffer, ((struct ahci_block_device*)dev->provider)->pci_bus, ((struct ahci_block_device*)dev->provider)->pci_dev, ((struct ahci_block_device*)dev->provider)->pci_func);
        if (rc != 0) {
            return -2; // read error
        }
        ((struct ahci_block_device*)dev->provider)->current_LBA_in_buffer = target_LBA;
    }
    u8 byte = ((struct ahci_block_device*)dev->provider)->buffer[dev->cursor % 512];
    dev->cursor++;
    return byte;
}
int block_read(struct block_device* dev, void* buffer, u64 size) {
    u8* dst = (u8*)buffer;
    for (u64 i = 0; i < size; i++) {
        int c = block_getc(dev);
        if (c < 0) {
            return -1; // error or EOF
        }
        dst[i] = (u8)c;
    }
    return 0;
}

int init_ahci_block_device(struct block_device* dev,struct ahci_block_device* ahci_dev, u8 pci_bus, u8 pci_dev, u8 pci_func, u64 size) {
	ahci_dev->pci_bus = pci_bus;
	ahci_dev->pci_dev = pci_dev;
	ahci_dev->pci_func = pci_func;
	dev->provider = ahci_dev;
	dev->cursor = 0;
	dev->size = size;
	dev->set_cursor = block_set_cursor;
	dev->read = block_read;
	dev->getc = block_getc;
	return 0;
}