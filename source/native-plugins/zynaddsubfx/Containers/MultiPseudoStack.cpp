#include "MultiPseudoStack.h"
#include <cassert>
#include <cstdio>

#define INVALID ((int32_t)0xffffffff)
#define MAX     ((int32_t)0x7fffffff)
QueueListItem::QueueListItem(void)
    :memory(0), size(0)
{
}

LockFreeQueue::LockFreeQueue(qli_t *data_, int n)
    :data(data_), elms(n), next_r(0), next_w(0), avail(0)
{
    tag  = new std::atomic<uint32_t>[n];
    for(int i=0; i<n; ++i)
        tag[i] = INVALID;
}


qli_t *LockFreeQueue::read(void) {
retry:
    int8_t free_elms = avail.load();
    if(free_elms <= 0)
        return 0;

    int32_t next_tag      = next_r.load();
    int32_t next_next_tag = (next_tag+1)&MAX;

    assert(next_tag != INVALID);

    for(int i=0; i<elms; ++i) {
        uint32_t elm_tag = tag[i].load();

        //attempt to remove tagged element
        //if and only if it's next
        if(((uint32_t)next_tag) == elm_tag) {
            if(!tag[i].compare_exchange_strong(elm_tag, INVALID))
                goto retry;

            //Ok, now there is no element that can be removed from the list
            //Effectively there's mutual exclusion over other readers here

            //Set the next element
            int sane_read = next_r.compare_exchange_strong(next_tag, next_next_tag);
            assert(sane_read && "No double read on a single tag");

            //Decrement available elements
            int32_t free_elms_next = avail.load();
            while(!avail.compare_exchange_strong(free_elms_next, free_elms_next-1));

            //printf("r%d ", free_elms_next-1);

            return &data[i];
        }
    }
    goto retry;
}

//Insert Node Q
void LockFreeQueue::write(qli_t *Q) {
retry:
    if(!Q)
        return;
    int32_t write_tag = next_w.load();
    int32_t next_write_tag = (write_tag+1)&MAX;
    if(!next_w.compare_exchange_strong(write_tag, next_write_tag))
        goto retry;

    uint32_t invalid_tag = INVALID;

    //Update tag
    int sane_write = tag[Q-data].compare_exchange_strong(invalid_tag, write_tag);
    assert(sane_write);

    //Increment available elements
    int32_t free_elms = avail.load();
    while(!avail.compare_exchange_strong(free_elms, free_elms+1))
        assert(free_elms <= 32);
    //printf("w%d ", free_elms+1);
}

MultiQueue::MultiQueue(void)
    :pool(new qli_t[32]), m_free(pool, 32), m_msgs(pool, 32)
{
    //32 instances of 2kBi memory chunks
    for(int i=0; i<32; ++i) {
        qli_t &ptr  = pool[i];
        ptr.size   = 2048;
        ptr.memory = new char[2048];
        free(&ptr);
    }
}

MultiQueue::~MultiQueue(void)
{
    for(int i=0; i<32; ++i)
        delete [] pool[i].memory;
    delete [] pool;
}
