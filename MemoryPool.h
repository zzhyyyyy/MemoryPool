// memory pool header file

// slot -> 内存槽
// block -> 内存块
// memory -> 内存池
// bucket -> 哈希桶
#pragma once
#include<iostream>
#include<atomic>
#include<mutex>
#include<cassert>
#include<cstdint>

constexpr size_t BASE_SLOT_SIZE = 8; 
constexpr size_t MAX_SLOT_SIZE = 512;
constexpr int MEMORYPOOL_NUM = 64;
struct slot{
    std::atomic<slot*> next;
};
class MemoryPool{
    public:
    MemoryPool(size_t Block_size=4096);
    ~MemoryPool();
    void init(size_t Slot_size);//以size_t大小的内存槽初始化每个内存块
    void* allocate();
    void  deallocate(void*);
    private:
    void allocateBlock();//申请新的内存块
    size_t padPointer(char *p,size_t align);//内存对齐
    bool pushFreelist(slot* slot);
    slot* popFreelist();
    private:
    size_t Block_size;//每一个小的内存块的大小
    size_t Slot_size;//每个内存槽的大小
    slot* firstblock;//当前slotsize所对应的内存池中的第一个Block指针
    slot* curslot;//当前block中第一个可用的内存槽
    slot* lastslot;//当前block中最后一个可用的内存槽
    // Attention:freeslot指向的是被释放的slot,对于多个线程可能会同时释放，所以需要原子操作
    std::atomic<slot*>freeslot;//当前block中使用过但已经被释放了的内存槽(可重新使用)
    std::mutex mtx;//用来控制线程使用槽的分配
};

//实现对不同size_t内存槽的映射
class MemoryBucket{
    public:
    static void initMemoryPool();
    static MemoryPool& getMemoryPool(int index);

    static void* useMemory(size_t size);
    static void freeMemory(void* ptr,size_t size);
    template<typename T,typename... Args>
    friend T* newElement(Args&&... args);
    template<typename T>
    friend void deleteElement(T* p);
};
template<typename T,typename... Args>
T* newElement(Args&& ...args) 
{
    T* p = nullptr;
    p = static_cast<T*>(MemoryBucket::useMemory(sizeof(T)));//使用内存池中的内存
    if(p!=nullptr) new(p) T(std::forward<Args>(args)...);//如果使用 p = new T(std::forward<Args>(args)...) 会重新取堆区开辟内存 完全没用上我们自己定义的内存池
    // std::forward<Args>(args)...语句等价于std::forward<Args>Args1,std::forward<Args>Args2...
    return p;
}
template<typename T>
void deleteElement(T* p)
{
    if(p)
    {
        p->~T();
        MemoryBucket::freeMemory(static_cast<void*>(p),sizeof(T));
    }
}
