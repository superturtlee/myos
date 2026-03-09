#include "page.h"
// u64永远是物理地址
//void*是当前进程的虚拟地址
//即使内核进程构建其他进程的页表，传入的也是相对于内核虚拟地址的虚拟地址，而不是相对用户空间的虚拟地址
//因此不会冲突
//用户使用u64传入时，必须check_alignment，确保地址是页对齐
//非earlyload不得在低虚拟地址构建页表，否则会发生安全问题，util不作检测，但是kernel部件必须确认
u64 phymap=0;//initially the physical memory is identity mapped, so phymap is 0, but after we enable paging, we will set phymap to the virtual address of the physical memory, which is 0xFFFFFFFF80000000ULL, so that we can access the physical memory through the virtual address.
unsigned char check_alignment(u64 address, u64 mask) {
    return (address & mask)==0;
}
//512GB page will never be used in this kernel, so we don't need to define PAGE_ADDR_512G_MASK
void finalize_phymap(u64 map_addr) {
    phymap=map_addr;
}
void* phys_to_ptr(u64 physical_address) {
    return (void*)(u64)(physical_address+phymap);
}

void get_page_index(void* virtual_address, u64* pgd_index, u64* pud_index, u64* pmd_index, u64* pte_index) {
    // x86-64: pgd_index=PML4, pud_index=PDPT, pmd_index=PD, pte_index=PT
    *pgd_index = ((u64)virtual_address >> 39) & 0x1FF;
    *pud_index = ((u64)virtual_address >> 30) & 0x1FF;
    *pmd_index = ((u64)virtual_address >> 21) & 0x1FF;
    *pte_index = ((u64)virtual_address >> 12) & 0x1FF;
}
int lookup_physical_address(pgd_t* pgd, void* virtual_address, u64* physical_address) {
    u64 pgd_index, pud_index, pmd_index, pte_index;
    get_page_index(virtual_address, &pgd_index, &pud_index, &pmd_index, &pte_index);
    pgd_t* pgd_table = (pgd_t*)pgd; 
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT)) {
        return -1; // pud entry not present
    }

    if (pud_table->entries[pud_index] & PAGE_HUGE) {
        *physical_address = (pud_table->entries[pud_index] & PAGE_ADDR_1G_MASK) | ((u64)virtual_address & 0x3FFFFFFFULL);
        return 0;
    }

    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    if (!(pmd_table->entries[pmd_index] & PAGE_PRESENT)) {
        return -1; // pmd entry not present
    }

    if (pmd_table->entries[pmd_index] & PAGE_HUGE) {
        *physical_address = (pmd_table->entries[pmd_index] & PAGE_ADDR_2M_MASK) | ((u64)virtual_address & 0x1FFFFFULL);
        return 0;
    }

    pte_t* pte_table = (pte_t*)phys_to_ptr(pmd_table->entries[pmd_index] & PAGE_ADDR_4K_MASK);
    if (!(pte_table->entries[pte_index] & PAGE_PRESENT)) {
        return -1; // PTE entry not present
    }
    *physical_address = (pte_table->entries[pte_index] & PAGE_ADDR_4K_MASK) | ((u64)virtual_address & 0xFFFULL);
    return 0; // Success
}
u64 ptr_to_phys(void* ptr) {// 解析物理地址时
    //read cr3 to get the current page table base address, and then use the page tables to translate the virtual address to physical address, if the virtual address is not mapped, return the original virtual address, which is useful for accessing the physical memory before we enable paging.
    u64 pgt=0;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(pgt));
    pgt=(u64)phys_to_ptr(pgt);
    u64 rtn=0;
    if(lookup_physical_address((pgd_t*)pgt, ptr, &rtn) == 0) {
        return rtn;
    } else {
        if(phymap) return 0;//映射了还返回虚拟地址就不对了，说明这个地址没有映射到物理内存，所以返回0，表示这个地址不可用
        else return (u64)ptr;//没有映射，说明物理内存是恒等映射的，所以直接返回虚拟地址就是物理地址
    }
}
void link_ptable(void* parent_table, void* child_table, u64 index, page_flags_t flags) {
    //checl alignment of child_table
    u64 entry = ptr_to_phys(child_table) | flags;
    ((u64*)parent_table)[index] = entry;
}
void unlink_ptable(void* parent_table, u64 index) {
    ((u64*)parent_table)[index] = 0;
}
int map_memory_4k(pgd_t* pgd, u64 physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index,u64 pmd_index,u64 pte_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {//这里不可能有 hugepage 512GB hugepage 它太大了，影响内存管理，我们也没有接口来映射512GB的页，所以我们不需要考虑 hugepage 的情况
        return -1; // PGD entry not present, cannot map
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT) || (pud_table->entries[pud_index] & PAGE_HUGE)) {
        return -1; // pud entry not present, cannot map 
    }
    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    if (!(pmd_table->entries[pmd_index] & PAGE_PRESENT) || (pmd_table->entries[pmd_index] & PAGE_HUGE)) {
        return -1; // pmd entry not present, cannot map
    }
    pte_t* pte_table = (pte_t*)phys_to_ptr(pmd_table->entries[pmd_index] & PAGE_ADDR_4K_MASK);
    if (pte_table->entries[pte_index] & PAGE_PRESENT) {
        return -2; // PTE already mapped
    }
    link_ptable(pte_table, phys_to_ptr(physical_address & PAGE_ADDR_4K_MASK), pte_index, flags | PAGE_PRESENT);
    return 0; // Success
}
int unmap_memory_4k(pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index,u64 pmd_index,u64 pte_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present, cannot unmap
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT) || (pud_table->entries[pud_index] & PAGE_HUGE)) {
        return -1; // pud entry not present, cannot unmap
    }
    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    if (!(pmd_table->entries[pmd_index] & PAGE_PRESENT) || (pmd_table->entries[pmd_index] & PAGE_HUGE)) {
        return -1; // pmd entry not present, cannot unmap
    }
    pte_t* pte_table = (pte_t*)phys_to_ptr(pmd_table->entries[pmd_index] & PAGE_ADDR_4K_MASK);
    pte_table->entries[pte_index] = 0; // Clear the PTE entry to unmap
    return 0; // Success
}
int map_memory_2m(pgd_t* pgd, u64 physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index,u64 pmd_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present, cannot map
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT)) {
        return -1; // pud entry not present, cannot map
    }
    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    if (pmd_table->entries[pmd_index] & PAGE_PRESENT) {
        return -2; // entry already mapped
    }
    link_ptable(pmd_table, phys_to_ptr(physical_address & PAGE_ADDR_2M_MASK), pmd_index, flags | PAGE_PRESENT | PAGE_HUGE);
    return 0; // Success
}
int unmap_memory_2m(pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index,u64 pmd_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present, cannot unmap
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT)) {
        return -1; // pud entry not present, cannot unmap
    }
    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    pmd_table->entries[pmd_index] = 0; // Clear the pmd entry to unmap
    return 0; // Success
}
int map_memory_1G(pgd_t* pgd, u64 physical_address, page_flags_t flags,u64 pgd_index,u64 pud_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present, cannot map
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if (pud_table->entries[pud_index] & PAGE_PRESENT) {
        return -2; // entry already mapped
    }
    link_ptable(pud_table, phys_to_ptr(physical_address & PAGE_ADDR_1G_MASK), pud_index, flags | PAGE_PRESENT | PAGE_HUGE);
    return 0; // Success
}
int set_leaf_page_flags(pgd_t* pgd, page_flags_t flags, u64 pgd_index, u64 pud_index, u64 pmd_index, u64 pte_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    if(pud_index>511){
        return -1;//pud_index out of range
    }
    if (!(pud_table->entries[pud_index] & PAGE_PRESENT)) {
        return -1; // pud entry not present
    }
    pmd_t* pmd_table = (pmd_t*)phys_to_ptr(pud_table->entries[pud_index] & PAGE_ADDR_4K_MASK);
    if(pmd_index>511){
        //pmd_index out of range set leaf page flags on a 1G huge page
        if (!(pud_table->entries[pud_index] & PAGE_HUGE)) {
            return -1; // not a huge page, cannot set leaf page flags
        }
        pud_table->entries[pud_index] |= flags; // Set the specified flags
        return 0; // Success
    }
    if (!(pmd_table->entries[pmd_index] & PAGE_PRESENT)) {
        return -1; // pmd entry not present
    }
    pte_t* pte_table = (pte_t*)phys_to_ptr(pmd_table->entries[pmd_index] & PAGE_ADDR_4K_MASK);
    if(pte_index>511){
        //pte_index out of range set leaf page flags on a 2M huge page
        if (!(pmd_table->entries[pmd_index] & PAGE_HUGE)) {
            return -1; // not a huge page, cannot set leaf page flags
        }
        pmd_table->entries[pmd_index] |= flags; // Set the specified flags
        return 0; // Success
    }
    if (!(pte_table->entries[pte_index] & PAGE_PRESENT)) {
        return -1; // PTE entry not present
    }
    pte_table->entries[pte_index] |= flags; // Set the specified flags
    return 0; // Success
}

int unmap_memory_1G(pgd_t* pgd, void* virtual_address,u64 pgd_index,u64 pud_index) {
    pgd_t* pgd_table = (pgd_t*)pgd;
    if (!(pgd_table->entries[pgd_index] & PAGE_PRESENT)) {
        return -1; // PGD entry not present, cannot unmap
    }
    pud_t* pud_table = (pud_t*)phys_to_ptr(pgd_table->entries[pgd_index] & PAGE_ADDR_4K_MASK);
    pud_table->entries[pud_index] = 0; // Clear the pud entry to unmap
    return 0; // Success
}
void load_page_directory(pgd_t* pgd) {
    u64 pgd_phys = ((u64)pgd) & PAGE_ADDR_4K_MASK;
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(pgd_phys) : "memory");
}
void flush_tlb() {
    __asm__ __volatile__(
        "mov %%cr3, %%rax\n"
        "mov %%rax, %%cr3\n"
        ::: "rax", "memory"
    );
}