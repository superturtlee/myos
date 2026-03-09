#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char* argv[]) {
    //open the bootfs
    FILE* bootfs = fopen(argv[1], "rb");
    if (!bootfs) {
        fprintf(stderr, "Failed to open bootfs: %s\n", argv[1]);
        return 1;
    }
    //read the files from the bootfs
    //read the header
    char header[8];
    if (fread(header, 1, 7, bootfs) != 7 || strncmp(header, "BOOTFS!", 7) != 0) {
        fprintf(stderr, "Invalid bootfs header\n");
        fclose(bootfs);
        return 1;
    }
    //read the number of files
    int num_files;
    if (fread(&num_files, sizeof(int), 1, bootfs) !=
        1) {
        fprintf(stderr, "Failed to read number of files from bootfs\n");
        fclose(bootfs);
        return 1;
    }
    while (!feof(bootfs)&& num_files-- > 0) {
        char filename[256];
        //fread
        for (size_t i = 0; i < sizeof(filename); i++) {
            if (fread(&filename[i], 1, 1, bootfs) != 1) {
                if (feof(bootfs)) {
                    break; // End of file reached
                }
                fprintf(stderr, "Failed to read filename from bootfs\n");
                fclose(bootfs);
                return 1;
            }
            if (filename[i] == '\0') {
                break; // Null terminator found, end of filename
            }
        }
        size_t len = strlen(filename);
        if (len > 0 && filename[len - 1] == '\n') {
            filename[len - 1] = '\0'; // Remove newline character
        }
        long size;
        if (fread(&size, sizeof(size), 1, bootfs) != 1) {
            fprintf(stderr, "Failed to read file size for: %s\n", filename);
            break;
        }
        printf("Extracting file: %s (size: %ld bytes)\n", filename, size);
        unsigned char* buffer = malloc(size);
        if (fread(buffer, 1, size, bootfs) != (size_t)size) {
            fprintf(stderr, "Failed to read file data for: %s\n", filename);
            free(buffer);
            break;
        }
        //write the buffer to a file
        FILE* f = fopen(filename, "wb");
        if (!f) {
            fprintf(stderr, "Failed to create file: %s\n", filename);
            free(buffer);
            continue;
        }
        fwrite(buffer, 1, size, f);
        fclose(f);
        free(buffer);
    }
    fclose(bootfs);
    return 0;  
}