# bootloader/amd64/mbr/i386/IA32Start.S
# MBR second stage: init 32-bit runtime and jump to C
.intel_syntax noprefix

.section .header, "ax"
.extern __image_size
.word __image_size

.section .text
.code16
.global _start
.extern kmain
.extern __bss_start
.extern __bss_end

.equ CODE_SEL, 0x08
.equ DATA_SEL, 0x10
.equ STACK_TOP, 0x9FC00
.equ PAGE_ADDR, 0x40000
_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax

    mov bp, offset message
    mov cx, 20
    call success

    cli
    mov al, 0xdd
    out 0x64, al
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEL:protected_mode_entry

.code32
protected_mode_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, STACK_TOP

    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb

    call kload

hang:
    hlt
    jmp hang

.code64


.code16
success:
    mov ax, 0x1301
    mov bx, 0x000A
    mov dl, 0
    int 0x10
    mov ah, 0x03
    int 0x10
    inc dh
    mov dl, 0
    mov ah, 0x02
    int 0x10
    ret

gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt



message:
    .ascii "BootLoader KLD Stage"