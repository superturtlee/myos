struct charoutput{
    void *device;
    void (*putc)(void* device,int color, int bg_color, char c);
};
void puts(struct charoutput* output, const char* str);