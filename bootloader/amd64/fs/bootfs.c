// bootloader/amd64/fs/bootfs.c
// a very simple fs only single threaded, only for loading the kernel in the bootloader stage
//the bl will only support a simple fs called bootfs
//more fs? never
#include "bootfs.h"
#include "../block/blockdevice.h"
void bootfs_init(struct bootfs_control* ctrl,struct block_device* block_dev,u64 start_cursor) {
    ctrl->block_dev = *block_dev;
    ctrl->start_cursor = start_cursor;
    ctrl->block_dev.set_cursor(&ctrl->block_dev, start_cursor);
    ctrl->file_found = 0;
    ctrl->byte_readed = 0;
}
u8 str_cmp(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}
int bootfs_file_size(struct bootfs_control* ctrl, const char* filename) {
    ctrl->block_dev.set_cursor(&ctrl->block_dev, ctrl->start_cursor);
    char header[8];
    if (ctrl->block_dev.read(&ctrl->block_dev, header, 7) != 0) {
        return -1; // Invalid bootfs header
    }
    header[7] = '\0';
    if (!str_cmp(header, "BOOTFS!")) {
        return -1; // Invalid bootfs header
    }
    
    int num_files;
    ctrl->file_found = 0;
    if (ctrl->block_dev.read(&ctrl->block_dev, &num_files, sizeof(int)) != 0) {
        return -1; // Failed to read number of files
    }
    while (num_files-- > 0) {
        int i = 0;
        while (i < sizeof(ctrl->last_size)) {
            int c = ctrl->block_dev.getc(&ctrl->block_dev);
            if (c < 0) {
                return -1; // Failed to read filename
            }
            ctrl->last_size[i] = (char)c;
            if (ctrl->last_size[i] == '\0') {
                break; // Null terminator found
            }
            i++;
        }
        long size;
        if (ctrl->block_dev.read(&ctrl->block_dev, &size, sizeof(size)) != 0) {
            return -1; // Failed to read file size
        }
        if (str_cmp(ctrl->last_size, filename)) {
            ctrl->last_size_value = size;
            ctrl->file_found = 1;
            ctrl->byte_readed = 0;
            return size; // Found the file
        }
        // Skip the file data
        for (long j = 0; j < size; j++) {
            if (ctrl->block_dev.getc(&ctrl->block_dev) < 0) {
                return -1; // Failed to skip file data
            }
        }
    }
    return -1; // File not found
}
int bootfs_read_file(struct bootfs_control* ctrl, const char* filename, void* buffer, u64 size) {
    if(!ctrl->file_found || !str_cmp(ctrl->last_size, filename)){
        return -1; // File not found or buffer too small
    }
    if(size>=ctrl->last_size_value-ctrl->byte_readed) {// 剩余文件大小小于等于buffer大小，可以一次性读完
        //read remaining
        size = ctrl->last_size_value - ctrl->byte_readed;
        ctrl->block_dev.read(&ctrl->block_dev, buffer, size);
        ctrl->byte_readed = 0;
        ctrl->file_found = 0; // reset file found status
        return 0;
    }
    ctrl->block_dev.read(&ctrl->block_dev, buffer, size);//部分读取
    ctrl->byte_readed += size;
    return 0;
}
/*
below are userspace extract
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
*/