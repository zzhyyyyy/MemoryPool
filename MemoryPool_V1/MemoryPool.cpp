// complete the header file
#include<iostream>
#include "MemoryPool.h"

MemoryPool::MemoryPool(size_t Block_size):
    Block_size(Block_size),
    Slot_size(0),
    firstblock(nullptr),
    curslot(nullptr),
    lastslot(nullptr),
    freeslot(nullptr){};

MemoryPool::~MemoryPool()
{
    slot* cur = firstblock;
    while(cur)
    {
        slot* next = cur->next;
        operator delete(static_cast<void*>(cur));
        cur = next;
    }
}
void MemoryPool::init(size_t Slot_size)
{
    assert(Slot_size>=8&&"槽基本内存大小为8 Byte");
    this->Slot_size=Slot_size;
    firstblock = nullptr;
    curslot = nullptr;
    lastslot = nullptr;
    freeslot = nullptr;
}

void* MemoryPool::allocate()
{
    slot* s = popFreelist();
    if(s) return s;
    slot* temp;
    {
        std::lock_guard<std::mutex>lock(mtx);
        if(curslot>=lastslot)
        {
            //当前内存池已满 无法再申请多余的槽
            //申请在当前槽大小的内存池中加一个内存块
            allocateBlock();
        }
        //若当前内存池可分配 直接把指向当前
        //空槽指针的curslot的值赋给temp
        temp = curslot;
        //更新curslot
        curslot += (Slot_size)/sizeof(slot);
    }
    return temp;
}

void MemoryPool::deallocate(void* p)
{
    if(!p) return ;//避免nullptr进入pushFreelist函数内
    pushFreelist(static_cast<slot*>(p));
}

void MemoryPool::allocateBlock()
{
    void* newBlock = operator new(Block_size);
    //头插法插入新的内存块
    (static_cast<slot*>(newBlock))->next = firstblock;
    firstblock=static_cast<slot*>(newBlock);
    //内存对齐
    char* body = static_cast<char*>(newBlock)+sizeof(slot*);//往后顺延sizeof(slot*)个字节
    size_t paddingSize = padPointer(body,Slot_size);
    curslot = reinterpret_cast<slot*>(body+paddingSize);
    lastslot = reinterpret_cast<slot*>(reinterpret_cast<size_t>(newBlock)+Block_size-Slot_size+1); 

    //std::cout<<"申请内存块成功\n";
}

size_t MemoryPool::padPointer(char* body,size_t align)
{
    //size_t offset = reinterpret_cast<size_t>(body) % align;
    void* temp = static_cast<void*>(body);
    size_t offset = reinterpret_cast<size_t>(temp)%align;
    return offset ? (align - offset) : 0;
}

bool MemoryPool::pushFreelist(slot* p)
{
    if(!p) return false;
    while(true)
    {
        //头插法插入被释放的空闲内存槽
        slot* oldHead = freeslot.load(std::memory_order_acquire);
        p->next.store(oldHead,std::memory_order_release);
        if(freeslot.compare_exchange_strong(oldHead,p)) return true;
    }
}

slot* MemoryPool::popFreelist()
{
    std::lock_guard<std::mutex>lock(popList_mtx);
    slot* oldHead = freeslot.load(std::memory_order_acquire);
    while(true)
    {
        if(!oldHead) return nullptr;
        slot* newHead = nullptr;
        try
        {
            newHead = oldHead->next.load(std::memory_order_acquire);
        }
        catch(...)
        {
            continue;
        }
        if(freeslot.compare_exchange_strong(oldHead,newHead)) return oldHead;
    }
}

void MemoryBucket::initMemoryPool()
{
    for(int i=0;i<MEMORYPOOL_NUM;i++)
    getMemoryPool(i).init((i+1)*BASE_SLOT_SIZE);
    //std::cout<<"初始化内存池成功\n";
    return ;
}

MemoryPool& MemoryBucket::getMemoryPool(int index)
{
    static MemoryPool memorypool[MEMORYPOOL_NUM];
    return memorypool[index];
}

void* MemoryBucket::useMemory(size_t size)
{
    assert(size>=0&&"size must greater than or equal zero!");
    if(!size) return nullptr;
    //else if(size>=MAX_SLOT_SIZE) new(size);报错
    else if(size>MAX_SLOT_SIZE) return operator new(size);
    return getMemoryPool((size+7)/BASE_SLOT_SIZE-1).allocate();
}

void MemoryBucket::freeMemory(void* ptr,size_t size)
{
    if(!ptr) return ;
    else if(size>MAX_SLOT_SIZE) 
    {
        operator delete(ptr);
        return ;
    }
    getMemoryPool((size+7)/BASE_SLOT_SIZE-1).deallocate(ptr);
}