#include "charoutput.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
struct vgascreen
{
    int cursor_x;
    int cursor_y;
};

void putc(void* device, int color, int bg_color, char c);
void init_vga_output(struct charoutput* output, struct vgascreen* screen);