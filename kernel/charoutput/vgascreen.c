// only run for csm
#include "vgascreen.h"
#include "../mem/page.h"

void outb(u16 port, u8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}
void update_cursor(struct vgascreen* screen) {
    // Cursor position is stored in the VGA hardware, we need to update it whenever we print a character
    u16 pos = screen->cursor_y * 80 + screen->cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}
void roll_screen(struct vgascreen* screen) {
    
    volatile u16* const vram = phys_to_ptr(0xB8000);
    for (int y = 1; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            vram[(y - 1) * 80 + x] = vram[y * 80 + x];
        }
    }
    // clear the last line
    for (int x = 0; x < 80; x++) {
        vram[24 * 80 + x] = ' ' | 0x0700; // white on black
    }
    screen->cursor_y--;
    update_cursor(screen);
}

void putc(void* device, int color, int bg_color, char c) {
    struct vgascreen* screen = (struct vgascreen*)device;
    volatile u16* const vram = phys_to_ptr(0xB8000);

    if (c == '\n') {
        screen->cursor_x = 0;
        screen->cursor_y++;
        if (screen->cursor_y >= 25) {
            roll_screen(screen);
        }
    } else {
        vram[screen->cursor_y * 80 + screen->cursor_x] = (u16)c | 0x0700; 
        screen->cursor_x++;
        if (screen->cursor_x >= 80) {
            screen->cursor_x = 0;
            screen->cursor_y++;
            if (screen->cursor_y >= 25) {
                roll_screen(screen);
            }
        }
    }
    update_cursor(screen);
}
u8 inb(u16 port) {
    u8 value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
void read_cursor_position(int* x,int* y) {
    volatile u16* const vram = phys_to_ptr(0xB8000);
    u16 pos = 0;
    outb(0x3D4, 0x0F);
    pos |= (u16)inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((u16)inb(0x3D5)) << 8;
    *x = pos % 80;
    *y = pos / 80;
}
void init_vga_output(struct charoutput* output, struct vgascreen* screen) {
    read_cursor_position(&screen->cursor_x, &screen->cursor_y);
    output->device = screen;
    output->putc = putc;
}