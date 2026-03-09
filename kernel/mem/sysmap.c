#include "sysmap.h"
pgd_t* kernel_PGD = (pgd_t*)KERNEL_PHYS;
pud_t* kernel_PUD = (pud_t*)(KERNEL_PHYS + 4*KB);
pud_t* phys_PUD0 = (pud_t*)(KERNEL_PHYS + 8*KB);
pud_t* phys_PUD1 = (pud_t*)(KERNEL_PHYS + 12*KB);
pmd_t* kernel_stack_PMD = (pmd_t*)(KERNEL_PHYS + 16*KB);
pud_t* temp_PUD = (pud_t*)(KERNEL_PHYS + 20*KB);
u64 unsuable_mems=0;//记录不可用内存的大小，单位是字节，主要用于后续的内存管理，暂时没有用到，先保留这个变量，以后可能会用到
/*
KERNEL PUD:
511: modules, currently unused
510: map 1GB for the kernel image, but currently we only map the first 512MB for the kernel image, because the kernel image is less than 512MB, and we want to reserve the rest of the 1GB for future use, e.g. loadable kernel modules
509: stack 1GB enough
*/
/*
    PHY:
    0-2M BL+GDT(Initialized by bl there is no need to rebuild it, just not break it)
    2M-2M+KSIZE[]: kernel image and kernel heap, currently we only map the kernel image, and the kernel heap is currently unused, but we can use it for dynamic memory allocation in the future
    2M+KSIZE[]-2M+KSIZE[]+24KB: kernel pts for all process 
    2M+KSIZE[]+2M-2M+KSIZE[]+2M+64MB heap
*/
u64  get_phys_size(){
    return 16*GB;
}
static void clear_pt_page(void* table) {
    u64* entries = (u64*)table;
    for (int i = 0; i < 512; i++) {
        entries[i] = 0;
    }
}
void k_init_kernel_pages(int kernel_size) {
    u64 old_cr3 = 0;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(old_cr3));
    pgd_t* old_pgd = (pgd_t*)phys_to_ptr(old_cr3 & PAGE_ADDR_4K_MASK);
    u64 kernel_size_aligned = ((u64)kernel_size + (2*MB - 1)) & ~((u64)(2*MB - 1));
    kernel_PGD = (pgd_t*)((u64)kernel_PGD + kernel_size_aligned);
    kernel_PUD = (pud_t*)((u64)kernel_PUD + kernel_size_aligned);
    phys_PUD0 = (pud_t*)((u64)phys_PUD0 + kernel_size_aligned);
    phys_PUD1 = (pud_t*)((u64)phys_PUD1 + kernel_size_aligned);
    kernel_stack_PMD = (pmd_t*)((u64)kernel_stack_PMD + kernel_size_aligned);
    clear_pt_page(kernel_PGD);
    clear_pt_page(kernel_PUD);
    clear_pt_page(phys_PUD0);
    clear_pt_page(phys_PUD1);
    clear_pt_page(kernel_stack_PMD);
    //unimplemented
    unsuable_mems+=2*MB+kernel_size_aligned+2*MB+64*MB;//记录不可用内存的大小，单位是字节，主要用于后续的内存管理，暂时没有用到，先保留这个变量，以后可能会用到
    //othervise the kernel image will overlap with the page tables, and the kernel will crash when it tries to access the page tables
    // K PMD map 0xFFFFFFFF80000000-0xFFFFFFFFFFFFFFFF, 512GB
    link_ptable(kernel_PGD, kernel_PUD, 511, PAGE_PRESENT | PAGE_WRITEABLE);
    // K PUD map 1GB size for the kernel image, but currently we only map the first 512MB for the kernel image, because the kernel image is less than 512MB, and we want to reserve the rest of the 1GB for future use, e.g. loadable kernel modules
    if (map_memory_1G(kernel_PGD, 0, PAGE_PRESENT | PAGE_WRITEABLE, 511, 510) != 0) {
        for(;;){ __asm__ __volatile__("hlt"); }
    }
    link_ptable(kernel_PUD, kernel_stack_PMD, 509, PAGE_PRESENT | PAGE_WRITEABLE);
    //build heap map
    for(int i=0;i<32;i++){
        if (map_memory_2m(kernel_PGD, 2*MB+kernel_size_aligned+2*MB+i*2*MB, PAGE_PRESENT | PAGE_WRITEABLE, 511, 509, 511-i) != 0) {
            for(;;){ __asm__ __volatile__("hlt"); }
        }
    }
    kernel_PGD->entries[0] = old_pgd->entries[0];
    // keep current low-half mapping from old CR3 to guarantee continuity after CR3 switch.

    link_ptable(kernel_PGD, phys_PUD0, 509, PAGE_PRESENT | PAGE_WRITEABLE);
    link_ptable(kernel_PGD, phys_PUD1, 510, PAGE_PRESENT | PAGE_WRITEABLE);

    for(int i=0;i<get_phys_size()/GB;i++){
        if (map_memory_1G(kernel_PGD, i*GB, PAGE_PRESENT | PAGE_WRITEABLE, 509+i/512, i) != 0) {
            for(;;){ __asm__ __volatile__("hlt"); }
        }
    }

    load_page_directory(kernel_PGD);
    //int set_leaf_page_flags(pgd_t* pgd, void* virtual_address, page_flags_t flags, u64 pgd_index, u64 pud_index, u64 pmd_index, u64 pte_index) 
    
    //set PAGE_GLOBAL
    //Linar map
    /** /
    for(int i=0;i<get_phys_size()/GB;i++){
        if (set_leaf_page_flags(kernel_PGD, PAGE_GLOBAL | (1ULL<<63), 509+i/512, i%512, 999, 999) != 0) {
            for(;;){ __asm__ __volatile__("hlt"); }
        }
    }

    //kernel image
    if(set_leaf_page_flags(kernel_PGD, PAGE_GLOBAL, 511, 510, 999, 999) != 0) {
        for(;;){ __asm__ __volatile__("hlt"); }
    }

    //stack
    for(int i=0;i<32;i++){
        if (set_leaf_page_flags(kernel_PGD,  PAGE_GLOBAL | (1ULL<<63), 511, 509, 511-i,999) != 0) {
            for(;;){ __asm__ __volatile__("hlt"); }
        }
    }
/**/

    finalize_phymap(PHYSICAL_VIRT);//page509/510 is the start of the physical memory mapping, so we set phymap to 0xFFFFFFFF80000000ULL-0x0000fe8000000000ULL=0xFFFFFFFF80000000ULL-0x0000fe8000000000ULL=0xFFFFFFFF80000000ULL-0x₀₀₀₀fe8₀₀₀₀₀₀₀₀=512GB
    //later we will update the pointers to the page tables to the virtual address, but currently we can still use the physical address to access the page tables, because we have identity mapped the first 1GB of physical memory in the temp page tables, and we have mapped the first 1GB of physical memory to the kernel virtual address in the kernel page tables, so we can access the page tables through the kernel virtual address after we jump to the kernel virtual address to execute the kernel, so we don't need to update the pointers to the page tables to the virtual address now, we can do it later when we jump to the kernel virtual address to execute the kernel.
    // 
    
        //update the pointers to the page tables to the virtual address, but currently we can still use the physical address to access the page tables, because we have identity mapped the first 1GB of physical memory in the temp page tables, and we have mapped the first 1GB of physical memory to the kernel virtual address in the kernel page tables, so we can access the page tables through the kernel virtual address after we jump to the kernel virtual address to execute the kernel, so we don't need to update the pointers to the page tables to the virtual address now, we can do it later when we jump to the kernel virtual address to execute the kernel.

    
    kernel_PGD = (pgd_t*)((u64)kernel_PGD + PHYSICAL_VIRT);//不是恒等映射，所以需要更新指针
    kernel_PUD = (pud_t*)((u64)kernel_PUD + PHYSICAL_VIRT);
    phys_PUD0 = (pud_t*)((u64)phys_PUD0 + PHYSICAL_VIRT);
    phys_PUD1 = (pud_t*)((u64)phys_PUD1 + PHYSICAL_VIRT);
    kernel_stack_PMD = (pmd_t*)((u64)kernel_stack_PMD + PHYSICAL_VIRT);
}
void k_unmap_low_memory() {
    unlink_ptable(kernel_PGD, 0);
    // Flush TLB by reloading CR3, otherwise stale TLB entries
    // will allow access to low memory without a page fault.
    flush_tlb();
}