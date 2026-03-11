typedef int lock_t;
void lock(lock_t* lock_) {
    while(1){
        int old=1;
        __asm__ __volatile__(
            "lock xchg %0, %1"
            : "+m"(*lock_), "+r"(old)
            :
            : "memory"
        );
        if(old==0)
            break;
    }
}
void unlock(lock_t* lock_) {
    __asm__ __volatile__(
        "mov $0, %0"
        : "+m"(*lock_)
        :
        : "memory"
    );
}
void init_lock(lock_t* lock_) {
    *lock_=0;
}

void lock_int(lock_t* lock_) {
    // 关闭中断，保护临界区，防止死锁
    __asm__ __volatile__("cli");
    lock(lock_);
}
void unlock_int(lock_t* lock_) {
    // 开启中断
    unlock(lock_);


    __asm__ __volatile__("sti");
}
void init_lock_int(lock_t* lock_) {
    init_lock(lock_);
}