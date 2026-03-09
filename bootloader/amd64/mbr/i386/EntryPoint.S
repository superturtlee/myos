# bootloader/amd64/mbr/i386/EntryPoint.S
.intel_syntax noprefix
.code16
.section .text
.global _start
.equ SEG_BASE, 0x7C00
.equ KERNEL_LOADER_ADDR, 0x7E00
.equ KERNEL_LOADER_SECT, 1
_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov [boot_drive], dl

    call cls

    mov bp, offset message
    mov cx, 20
    call success
    mov eax,KERNEL_LOADER_SECT
    mov bx,KERNEL_LOADER_ADDR
    mov cx,1
    call readdisk
    mov ax,[KERNEL_LOADER_ADDR]
    mov cx, ax
    shr cx, 9
    test ax, 0x01ff
    jnz go
    dec cx
    go:
    jcxz load_done #cx==0
    mov eax,KERNEL_LOADER_SECT+1
    mov bx,KERNEL_LOADER_ADDR+512
    call readdisk #
    load_done:
    jmp KERNEL_LOADER_ADDR+2

hang: 
    jmp hang


success:
    mov ax, 0x1301
    mov bx, 0x00a
    mov dl, 0
    int 0x10
    mov ah, 0x03
    int 0x10
    inc dh
    mov dl, 0
    mov ah, 0x02
    int 0x10
    ret

cls:
    mov ax, 0x0600 #
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x184f
    int 0x10
    mov ax, 0x0200
    mov bx, 0
    mov dx, 0 
    int 0x10
    ret

readdisk:
#args eax:sector bx:loadaddr cx:count/size
    mov dword ptr [dap_lba_low], eax
    mov dword ptr [dap_lba_high], 0
    mov word ptr [dap_offset], bx
    mov word ptr [dap_segment], 0
    mov word ptr [sectors_left], cx

.read_next:
    mov ax, [sectors_left]
    test ax, ax
    jz .done
    cmp ax, 127
    jbe .chunk_ok
    mov ax, 127
.chunk_ok:
    mov [dap_count], ax

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, offset disk_address_packet
    int 0x13
    jc .disk_error

    mov ax, [dap_count]
    add word ptr [dap_lba_low], ax
    adc word ptr [dap_lba_low + 2], 0

    mov ax, [dap_count]
    shl ax, 9
    add word ptr [dap_offset], ax
    jnc .no_seg_carry
    add word ptr [dap_segment], 0x1000
.no_seg_carry:

    mov ax, [sectors_left]
    sub ax, [dap_count]
    mov [sectors_left], ax
    jmp .read_next

.disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp hang

.done:
    ret



message:
    .ascii "BootLoader MBR Stage"

boot_drive:
    .byte 0

sectors_left:
    .word 0

disk_address_packet:
    .byte 0x10
    .byte 0x00
dap_count:
    .word 0
dap_offset:
    .word 0
dap_segment:
    .word 0
dap_lba_low:
    .long 0
dap_lba_high:
    .long 0

    . = _start + 510
    .byte 0x55, 0xaa
