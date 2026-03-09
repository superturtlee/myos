// bootloader/amd64/mbr/i386/IA32Entry.c
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct GDT64 {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed));

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) GDTR;

enum {
    PAGE_PRESENT = 1ULL << 0,
    PAGE_RW = 1ULL << 1,
    PAGE_PS = 1ULL << 7,
    CR4_PAE = 1U << 5,
    CR0_PG = 1U << 31,
    EFER_MSR = 0xC0000080U,
    EFER_LME = 1ULL << 8
};

extern unsigned char _binary_amd64entry_bin_start[];
extern void enter_long_mode(u32 entry_addr);

__asm__(
".intel_syntax noprefix\n"
".global enter_long_mode\n"
"enter_long_mode:\n"
"    mov eax, [esp + 4]\n"
"    jmp 0x08:amd64_entry64\n"
"\n"
".code64\n"
".global amd64_entry64\n"
"amd64_entry64:\n"
"    mov rsp, 0x9FC00\n"
"    mov eax, eax\n"
"    call rax\n"
"1:\n"
"    hlt\n"
"    jmp 1b\n"
".code32\n"
".att_syntax prefix\n"
);

static struct GDT64 gdt[3];
static GDTR gdtr;

static u64 pml4[512] __attribute__((aligned(4096)));// 512 entries for 4-level paging
static u64 pdpt[512] __attribute__((aligned(4096)));// 512 entries for 4-level paging
static u64 pd0[512] __attribute__((aligned(4096)));// map   0GB-1GB
static u64 pd1[512] __attribute__((aligned(4096)));// map   1GB-2GB
static u64 pd2[512] __attribute__((aligned(4096)));// map   2GB-3GB
static u64 pd3[512] __attribute__((aligned(4096)));// map   3GB-4GB

static void GDT_entry(struct GDT64* entry, u32 base, u32 limit, u8 is_code, u8 is_long_mode) {
    entry->limit_low = (u16)(limit & 0xFFFFU);
    entry->base_low = (u16)(base & 0xFFFFU);
    entry->base_middle = (u8)((base >> 16) & 0xFFU);
    entry->access = is_code ? 0x9AU : 0x92U;
    entry->granularity = (u8)((limit >> 16) & 0x0FU);
    entry->granularity |= 0x80U;
    if (is_code && is_long_mode) {
        entry->granularity |= 0x20U;
    } else {
        entry->granularity |= 0x40U;
    }
    entry->base_high = (u8)((base >> 24) & 0xFFU);
}

static void setup_gdt(void) {
    GDT_entry(&gdt[0], 0, 0, 0, 0);
    GDT_entry(&gdt[1], 0, 0xFFFFFU, 1, 1);
    GDT_entry(&gdt[2], 0, 0xFFFFFU, 0, 0);
    gdtr.limit = (u16)(sizeof(gdt) - 1U);
    gdtr.base = (u32)gdt;
}

static void load_gdt(void) {
    __asm__ __volatile__("lgdt %0" : : "m"(gdtr));
}

static u64 rdmsr(u32 msr) {
    u32 lo;
    u32 hi;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | (u64)lo;
}

static void wrmsr(u32 msr, u64 value) {
    u32 lo = (u32)(value & 0xFFFFFFFFULL);
    u32 hi = (u32)(value >> 32);
    __asm__ __volatile__("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

void setup_paging(void) {
    u32 i;

    for (i = 0; i < 512U; i++) {
        pml4[i] = 0;// Clear all entries
        pdpt[i] = 0;
        pd0[i] = 0;
        pd1[i] = 0;
        pd2[i] = 0;
        pd3[i] = 0;
    }

    pml4[0] = ((u64)(u32)pdpt) | PAGE_PRESENT | PAGE_RW;// Point PML4[0] to PDPT
    pdpt[0] = ((u64)(u32)pd0) | PAGE_PRESENT | PAGE_RW;// 0GB-1GB
    pdpt[1] = ((u64)(u32)pd1) | PAGE_PRESENT | PAGE_RW;// 1GB-2GB
    pdpt[2] = ((u64)(u32)pd2) | PAGE_PRESENT | PAGE_RW;// 2GB-3GB
    pdpt[3] = ((u64)(u32)pd3) | PAGE_PRESENT | PAGE_RW;// 3GB-4GB

    for (i = 0; i < 512U; i++) {
        pd0[i] = ((u64)i << 21) | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        pd1[i] = (((u64)i + 512ULL) << 21) | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        pd2[i] = (((u64)i + 1024ULL) << 21) | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        pd3[i] = (((u64)i + 1536ULL) << 21) | PAGE_PRESENT | PAGE_RW | PAGE_PS;
    }

    __asm__ __volatile__("mov %0, %%cr3" : : "r"((u32)pml4) : "memory");

    {
        u32 cr4;
        __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= CR4_PAE;
        __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4) : "memory");
    }

    {
        u64 efer = rdmsr(EFER_MSR);
        efer |= EFER_LME;
   
        wrmsr(EFER_MSR, efer);
    }

    {
        u32 cr0;
        __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= CR0_PG;
        __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0) : "memory");
    }
}

void long_mode_entry(void) {
    setup_gdt();
    load_gdt();
    setup_paging();
    // After this point, we are in long mode and can use 64-bit code
    
}

void kload(void) {
    u32 amd64_entry_addr = (u32)_binary_amd64entry_bin_start;

    long_mode_entry();
    enter_long_mode(amd64_entry_addr);
}
