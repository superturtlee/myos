#include "sysmap.h"
typedef unsigned char u8;
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef void (*func_ptr_t)(int, int, char**);
void putc(u16 x,u16 y,u8 c);
void kmain_high(int kernel_size, int unused, char** unused2);
void halt();
void kmain(int kernel_size, int unused, char** unused2){
    putc(0,0,'K');
    //halt();
    k_init_kernel_pages(kernel_size);//这里把内核重新映射到0xFFFFFFFF80000000ULL处了，所以我们需要在kmain_high中把kmain_high的地址转换为虚拟地址，然后跳转到kmain_high执行
    //convert kmain_high ptr
    func_ptr_t kmain_high_ptr = (func_ptr_t)((u64)kmain_high - (u64)kmain + KERNEL_VIRT +MB*2);
    //在bootloader 我们加载内核到2MB处，在kernel中我们将地址低位从0映射到0xFFFFFFFF80000000ULL，所以我们需要将kmain_high的地址转换为虚拟地址
    //新的内核映像在0xFFFFFFFF80000000ULL+2MB处，所以我们需要将kmain_high的地址转换为虚拟地址，即kmain_high的物理地址加上0xFFFFFFFF80000000ULL-2MB
    __asm__ __volatile__(
        "mov %0, %%rsp\n"
        "jmp *%1\n"
        :
        : "r"((u64)initial_stack_top), "r"(kmain_high_ptr), "D"(kernel_size), "S"(unused), "d"(unused2)
        : "memory"
    );
    __builtin_unreachable();
    for(;;);
}
void kmain_high(int kernel_size, int unused, char** unused2){
    volatile u16* const vram = (volatile u16*)0xB8000;
    k_unmap_low_memory();
    putc(1,0,'H');
    for(;;);
}
void putc(u16 x,u16 y,u8 c){
    u16* vram = (volatile u16*)phys_to_ptr(0xB8000);
    vram[y * 80 + x] = (u16)c | 0x0700; 
}
void halt(){
    for(;;){
        __asm__ __volatile__("hlt");
    }
}