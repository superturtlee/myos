#include <stdio.h>
typedef unsigned long long u64;
#define VIRTUAL_PGT_RANGE_START(index) ((u64)(index) << 39)
#define VIRTUAL_PGT_RANGE_END(index) (((u64)(index) + 1) << 39) - 1
#define VIRTUAL_PUD_RANGE_START(index) ((u64)(index) << 30)
#define VIRTUAL_PUD_RANGE_END(index) (((u64)(index) + 1) << 30) - 1
#define VIRTUAL_PMD_RANGE_START(index) ((u64)(index) << 21)
#define VIRTUAL_PMD_RANGE_END(index) (((u64)(index) + 1) << 21) - 1
#define VIRTUAL_PTE_RANGE_START(index) ((u64)(index) << 12)
#define VIRTUAL_PTE_RANGE_END(index) (((u64)(index) + 1) << 12) - 1
#define KB (1<<10)
#define MB (1<<20)
#define GB (1ULL<<30)
#define INDEX_MAX 511

static int is_index_valid(int index) {
    return index >= 0 && index <= INDEX_MAX;
}

static int is_canonical_address(u64 virtual_address) {
    u64 sign_bit = (virtual_address >> 47) & 1ULL;
    u64 high_bits = virtual_address >> 48;
    if (sign_bit) {
        return high_bits == 0xFFFFULL;
    }
    return high_bits == 0;
}

static u64 canonicalize_48bit(u64 address) {
    if (address & (1ULL << 47)) {
        return address | 0xFFFF000000000000ULL;
    }
    return address & 0x0000FFFFFFFFFFFFULL;
}

void print_range(const char *name, u64 start, u64 end) {
    u64 canonical_start = canonicalize_48bit(start);
    u64 canonical_end = canonicalize_48bit(end);
    u64 start_GBS = start / GB;
    u64 start_MB = (start % GB) / MB;
    u64 start_KB = (start % MB) / KB;
    u64 end_GBS = end / GB;
    u64 end_MB = (end % GB) / MB;
    u64 end_KB = (end % MB) / KB;
    printf("%s range: 0x%016llx - 0x%016llx (%llu GB, %llu MB, %llu KB - %llu GB, %llu MB, %llu KB)\n",
           name, canonical_start, canonical_end,
           start_GBS, start_MB, start_KB,
           end_GBS, end_MB, end_KB);
}
int main() {
    int pgd_input, pud_input, pmd_input, pte_input;
    printf("Enter PGD index, PUD index, PMD index, PTE index (0-511): ");
    if (scanf("%d %d %d %d", &pgd_input, &pud_input, &pmd_input, &pte_input) != 4) {
        printf("Invalid input format.\n");
        return 1;
    }
    if (!is_index_valid(pgd_input) || !is_index_valid(pud_input) ||
        !is_index_valid(pmd_input) || !is_index_valid(pte_input)) {
        printf("Index out of range. Each index must be between 0 and 511.\n");
        return 1;
    }

    u64 pgd_range_start = VIRTUAL_PGT_RANGE_START(pgd_input);
    u64 pgd_range_end = VIRTUAL_PGT_RANGE_END(pgd_input);
    u64 pud_range_start = VIRTUAL_PUD_RANGE_START(pud_input);
    u64 pud_range_end = VIRTUAL_PUD_RANGE_END(pud_input);
    u64 pmd_range_start = VIRTUAL_PMD_RANGE_START(pmd_input);
    u64 pmd_range_end = VIRTUAL_PMD_RANGE_END(pmd_input);
    u64 pte_range_start = VIRTUAL_PTE_RANGE_START(pte_input);
    u64 pte_range_end = VIRTUAL_PTE_RANGE_END(pte_input);

    print_range("PGD", pgd_range_start, pgd_range_end);
    print_range("PUD", pud_range_start + pgd_range_start, pud_range_end + pgd_range_start);
    print_range("PMD", pmd_range_start + pud_range_start + pgd_range_start, pmd_range_end + pud_range_start + pgd_range_start);
    print_range("PTE", pte_range_start + pmd_range_start + pud_range_start + pgd_range_start, pte_range_end + pmd_range_start + pud_range_start + pgd_range_start);

    printf("input virtual address (hex): ");
    u64 virtual_address;
    if (scanf("%llx", &virtual_address) != 1) {
        printf("Invalid virtual address input.\n");
        return 1;
    }

    u64 pgd_index = (virtual_address >> 39) & 0x1FF;
    u64 pud_index = (virtual_address >> 30) & 0x1FF;
    u64 pmd_index = (virtual_address >> 21) & 0x1FF;
    u64 pte_index = (virtual_address >> 12) & 0x1FF;
    if (!is_canonical_address(virtual_address)) {
        printf("Warning: 0x%016llx is not a canonical x86-64 virtual address.\n", virtual_address);
    }
    printf("Virtual address 0x%016llx has PGD index %llu, PUD index %llu, PMD index %llu, PTE index %llu\n",
           virtual_address, pgd_index, pud_index, pmd_index, pte_index);
    return 0;
}