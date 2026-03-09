#include "page.h"
#include "defs.h"
#define initial_stack_top (KERNEL_VIRT-8)//510映射KERNEL_VIRT的1GB前面的509就是堆栈区域，初始栈顶需要8字节对齐(jmp进入函数，ABI要求RSP≡8 mod 16)
/*
KERNEL PUD in pgt 511:
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
#define initial_stack_size (64*MB)
extern pgd_t* kernel_PGD;// 在初始化页表之前，指向页表物理地址，初始化后将指向页表虚拟地址，然后内核会jump到内核虚拟地址执行
extern pud_t* kernel_PUD;// 在初始化页表之前，指向页表物理地址，初始化后将指向页表虚拟地址 进程共享
extern pud_t* phys_PUD0;
extern pud_t* phys_PUD1;
void k_unmap_low_memory();
void k_init_kernel_pages(int kernel_size);