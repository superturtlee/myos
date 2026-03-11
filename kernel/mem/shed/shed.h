typedef unsigned long long u64;
struct memory_shed {
    void* shed_internal;// internal data for the memory shed
    // malloc
    u64 (*malloc)(struct memory_shed* shed, u64 size);//we shed physcal memory, so the return value is a physical address not void* which is a virtual address
    // free
    int (*free)(struct memory_shed* shed, u64 addr);//we shed physcal
};