#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    //initialize the bootfs
    FILE* bootfs = fopen(argv[1], "wb+");
    //read the files into memory
    //headee: BOOTFS!
    fwrite("BOOTFS!", 1, 7, bootfs);
    //number of files
    int num_files = argc - 2;
    fwrite(&num_files, sizeof(int), 1, bootfs);
    for(int i = 2; i < argc; i++) {
        FILE* f = fopen(argv[i], "rb");
        if (!f) {
            fprintf(stderr, "Failed to open file: %s\n", argv[i]);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* buffer = malloc(size);
        fread(buffer, 1, size, f);

        fclose(f);
        //write the buffer to the bootfs
        fwrite(argv[i], 1, strlen(argv[i]) + 1, bootfs);//write the filename first, including the null terminator
        fwrite(&size, sizeof(size), 1, bootfs);//write the size of the file first
        fwrite(buffer, 1, size, bootfs);
        free(buffer);
    }
    fclose(bootfs);
    return 0;  
}