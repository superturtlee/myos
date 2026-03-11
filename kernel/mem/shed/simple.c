#include "simple.h"
#include "../page.h"
//#include "page.h" we don't need page.h because we only shed physical memory
struct linked_list_node {
    u64 addr_start;// the start address of the memory block
    u64 addr_end;// the end address of the memory block[start_addr, end_addr)
    struct linked_list_node* next;
};
// simple memory shed implementation
struct simple_shed_internal {
    struct linked_list_node* occupy_list;
//    struct linked_list_node* free_list; no need free list because we can directly add the freed memory block to the occupy list
    u64 start_addr;// the start address of the memory shed
    u64 end_addr;// the end address of the memory shed
    void* lock;// lock for the memory shed
};

void allign_4k(u64* addr) {
    if (*addr % 4096 != 0) {
        *addr = (*addr + 4096) >> 12 << 12;// allign to 4k
    }
}
void allign_4k_lower(u64* addr) {
    if (*addr % 4096 != 0) {
        *addr = (*addr) >> 12 << 12;// allign to 4k
    }
}
u64 simple_malloc(struct memory_shed* shed, u64 size) {
   struct simple_shed_internal* internal = (struct simple_shed_internal*)shed->shed_internal;
    lock(internal->lock);
    struct linked_list_node* node = internal->occupy_list;
    struct linked_list_node* prev = 0;
    u64 total_size = size+4096;// we need to allocate a linked list node to record the occupied memory block, so the total size we need to allocate is the requested size plus the size of the linked list node
    //浪费空间 但是可以让返回的物理地址对齐到4k，方便后续的内存管理
    allign_4k(&total_size);
    u64 lastend = internal->start_addr;
    while (node != 0) {
        lastend = prev == 0 ? internal->start_addr : prev->addr_end;
        if (node->addr_start - lastend >= total_size) {
            // we find a free memory block
            struct linked_list_node* newnode = phys_to_ptr(lastend);
            newnode->addr_start = lastend;
            newnode->addr_end = lastend + total_size;
            newnode->next = node;
            if (prev == 0) {
                internal->occupy_list = newnode;
            } else {
                prev->next = newnode;
            }
            unlock(internal->lock);
            return newnode->addr_start+4096;// return the start address of the allocated memory block, we need to add 4096 to skip the linked list node
        }
        prev = node;
        node = node->next;
    }
    // check the memory block after the last node
    // no need as we ser a block with size 0 at the end of the occupy list
    unlock(internal->lock);
    return 0;// no free memory block found
}
int simple_free(struct memory_shed* shed, u64 addr) {
    // no need allign the addr because we can directly add the freed memory block to the occupy list when we allocate memory, so the addr must be alligned to 4k
    struct simple_shed_internal* internal = (struct simple_shed_internal*)shed->shed_internal;
    lock(internal->lock);
    struct linked_list_node* node = internal->occupy_list;
    struct linked_list_node* prev = 0;
    while (node != 0) {
        if (node->addr_start == addr-4096) {
            // we find the memory block to be freed
            if (prev == 0) {
                internal->occupy_list = node->next;
            } else {
                prev->next = node->next;
            }
            // we don't need to add the freed memory block to the free list because we can directly add it to the occupy list when we allocate memory

            //do nor merge
            //will break cause fail to free


            unlock(internal->lock);
            return 0;
        }
        prev = node;
        node = node->next;
    }
    unlock(internal->lock);
    return -1;
}
int init_simple_shed(struct memory_shed* shed, u64 start_addr, u64 end_addr) {
    //allign the start_addr and end_addr to 4k
    allign_4k(&start_addr);
    allign_4k_lower(&end_addr);
    //
    start_addr+=4096;// we need to allocate a mem for internal data of the memory shed, so we need to add 4096 to the start_addr to skip the memory block for the internal data
    if (end_addr <= start_addr) {
        // the memory shed is too small to be initialized
        shed->shed_internal = 0;
        shed->malloc = 0;
        shed->free = 0;
        return -1;
    }

    struct simple_shed_internal* internal = phys_to_ptr(start_addr-4096+sizeof(void*));//ABI SIZE lock
    internal->lock =phys_to_ptr(start_addr-4096);// lock for the memory shed, we can use the memory block before the internal data to store the lock
    struct linked_list_node* node = phys_to_ptr(start_addr-4096+sizeof(struct simple_shed_internal));
    node->addr_start = end_addr+1;//[)
    node->addr_end = end_addr+1;
    node->next = 0;
    internal->start_addr = start_addr;
    internal->end_addr = end_addr;
    internal->occupy_list = node;
    shed->shed_internal = internal;
    shed->malloc = simple_malloc;
    shed->free = simple_free;
    init_lock(internal->lock);
    return 0;
}