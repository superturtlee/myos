// only run for csm
#include "types.h"
int cursor_x = 0;
int cursor_y = 0;
char is_csm = 1;
void detect_csm() {
    // check if we are running in csm mode by trying to access the vga text buffer at 0xB8000
    // but when in uefi build, the code is still there, but it should not be used, because in uefi mode we should use the uefi console output protocol instead of the vga text buffer
    //it is not graceful to make the code still there, but it is simpler than making two separate builds   it simply the makefile
    volatile u16* const vram = (volatile u16*)0xB8000;
    // try to read from the vga text buffer, if it succeeds we are in csm mode
    // if it fails we are in uefi mode and we should not use the vga text buffer
    // we can detect this by checking if the value at 0xB8000 is 0 or not, because in uefi mode it should be 0
    if (vram[0] == 0) {
        is_csm = 0;
    }
}
void outb(u16 port, u8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}
void update_cursor() {
    // Cursor position is stored in the VGA hardware, we need to update it whenever we print a character
    u16 pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}
void roll_screen() {
    volatile u16* const vram = (volatile u16*)0xB8000;
    for (int y = 1; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            vram[(y - 1) * 80 + x] = vram[y * 80 + x];
        }
    }
    // clear the last line
    for (int x = 0; x < 80; x++) {
        vram[24 * 80 + x] = ' ' | 0x0700; // white on black
    }
    cursor_y--;
    update_cursor();
}
void putc(char c) {
    if (!is_csm) {
        //uefi putc here later
        return; // do nothing if we are not in csm mode
    }
    volatile u16* const vram = (volatile u16*)0xB8000;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 25) {
            roll_screen();
        }
    } else {
        vram[cursor_y * 80 + cursor_x] = (u16)c | 0x0700; // white on black
        cursor_x++;
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= 25) {
                roll_screen();
            }
        }
    }
    update_cursor();
}
void puts(const char* str) {
    if (!is_csm) {
        return; // do nothing if we are not in csm mode
    }
    while (*str) {
        putc(*str);
        str++;
    }
    update_cursor();
}
void puthex32(u32 v) {
    if (!is_csm) {
        return; // do nothing if we are not in csm mode
    }
    char hex[] = "0123456789ABCDEF";
    puts("0x");
    for (int i = 0; i < 8; i++) {
        u8 nib = (u8)((v >> ((7 - i) * 4)) & 0xFU);
        putc(hex[nib]);
    }
    update_cursor();
}