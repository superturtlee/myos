// bootloader/amd64/block/blockdevice.h
#include "../types.h"
struct block_device{
    void* provider;//提供者，指向具体的设备结构体，比如ahci设备结构体
    u64 cursor;//以byte为单位的读写位置
    u64 size;//以byte为单位的设备大小
    int (*set_cursor)(struct block_device* dev, u64 cursor);
    int (*read)(struct block_device* dev, void* buffer, u64 size);
    int (*getc)(struct block_device* dev);
    //write函数暂时不需要，因为我们只需要读取文件来加载内核
};
