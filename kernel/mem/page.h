typedef unsigned long long u64;
typedef unsigned int u32;
typedef struct pgd_t {
    u64 entries[512];
} __attribute__((aligned(4096))) pgd_t;
typedef struct pud_t {
    u64 entries[512];
} __attribute__((aligned(4096))) pud_t;
typedef struct pmd_t {
    u64 entries[512];
} __attribute__((aligned(4096))) pmd_t;
typedef struct pte_t {
    u64 entries[512];
} __attribute__((aligned(4096))) pte_t;
typedef enum{
    PAGE_PRESENT = 1ULL << 0,
    PAGE_WRITEABLE = 1ULL << 1,
    PAGE_USER = 1ULL << 2,
    PAGE_WRITE_THROUGH = 1ULL << 3,
    PAGE_CACHE_DISABLE = 1ULL << 4,
    PAGE_ACCESSED = 1ULL << 5,
    PAGE_DIRTY = 1ULL << 6,
    PAGE_HUGE = 1ULL << 7,
    PAGE_GLOBAL = 1ULL << 8,
    PAGE_UN_EXECUTE = 1ULL << 63
} page_flags_t;
#define KB (1<<10)
#define PAGE_SIZE 4*KB
#define VIRTUAL_PGT_RANGE_START(index) ((u64)(index) << 39)
#define VIRTUAL_PGT_RANGE_END(index) (((u64)(index) + 1) << 39) - 1
#define VIRTUAL_PUD_RANGE_START(index) ((u64)(index) << 30)
#define VIRTUAL_PUD_RANGE_END(index) (((u64)(index) + 1) << 30) - 1
#define VIRTUAL_PMD_RANGE_START(index) ((u64)(index) << 21)
#define VIRTUAL_PMD_RANGE_END(index) (((u64)(index) + 1) << 21) - 1
#define VIRTUAL_PTE_RANGE_START(index) ((u64)(index) << 12)
#define VIRTUAL_PTE_RANGE_END(index) (((u64)(index) + 1) << 12) - 1
void get_page_index(void* virtual_address, u64* pgd_index, u64* pud_index, u64* pmd_index, u64* pte_index);
void link_ptable(void* parent_table, void* child_table, u64 index, page_flags_t flags);
void unlink_ptable(void* parent_table, u64 index);
int unmap_memory_4k( pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index,u64 pmd_index,u64 pte_index);
int map_memory_4k(pgd_t* pgd, u64 physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index,u64 pmd_index,u64 pte_index);
int unmap_memory_2m(pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index,u64 pmd_index);
int map_memory_2m(pgd_t* pgd, u64    physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index,u64 pmd_index);
int unmap_memory_1G(pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index);
int map_memory_1G(pgd_t* pgd, u64 physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index);
int lookup_physical_address(pgd_t* pgd, void* virtual_address, u64* physical_address);
void finalize_phymap(u64 map_addr);
void* phys_to_ptr(u64 phys_addr);
u64 ptr_to_phys(void* ptr);
void load_page_directory(pgd_t* pgd);
#define PAGE_ADDR_4K_MASK 0xFFFFFFFFFFFFF000ULL
#define PAGE_ADDR_2M_MASK 0xFFFFFFFFFFE00000ULL
#define PAGE_ADDR_1G_MASK 0xFFFFFFFFC0000000ULL