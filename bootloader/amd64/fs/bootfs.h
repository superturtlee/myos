// bootloader/amd64/fs/bootfs.h
#include "../types.h"
#include "../block/blockdevice.h"
struct bootfs_control{
    struct block_device block_dev;
    u64 start_cursor;//bootfs在磁盘上的起始位置，以byte为单位
    char last_size[256];//上次查询文件大小时的文件名
    u64 last_size_value;
    u8 file_found;//上次查询文件大小时是否找到文件
    int byte_readed;//在读取文件时已经读取的字节数 typo but not important
};
int bootfs_file_size(struct bootfs_control* ctrl, const char* filename);
void bootfs_init(struct bootfs_control* ctrl,struct block_device* block_dev,u64 start_cursor);
int bootfs_read_file(struct bootfs_control* ctrl, const char* filename, void* buffer, u64 size);