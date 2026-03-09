#include "charoutput.h"

void puts(struct charoutput* output, const char* str) {
    while (*str) {
        output->putc(output->device, 0, 0, *str); // default color: white on black
        str++;
    }
}