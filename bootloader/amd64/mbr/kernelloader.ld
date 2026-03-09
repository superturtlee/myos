ENTRY(_start)

SECTIONS
{
    . = 0x7E00;

    .header :
    {
        KEEP(*(.header))
    }

    .text :
    {
        *(.text*)
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    __load_end = .;

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    __image_end = .;
    __image_size = __load_end - 0x7E00;
}
