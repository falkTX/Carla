/*
  ZynAddSubFX - a software synthesizer

  MultiPseudoStack.h - Multiple-Writer Lock Free Datastructure
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <atomic>
#include <cassert>

namespace zyncarla {

//XXX rename this thing
typedef struct QueueListItem qli_t;
struct QueueListItem
{
    QueueListItem(void);
    char     *memory;
    uint32_t  size;
};


//Many reader many writer
class LockFreeQueue
{
    qli_t *const           data;
    const int              elms;
    std::atomic<uint32_t> *tag;
    std::atomic<int32_t>   next_r;
    std::atomic<int32_t>   next_w;
    std::atomic<int32_t>   avail;
    public:
    LockFreeQueue(qli_t *data_, int n);
    ~LockFreeQueue(void);
    qli_t *read(void);
    void write(qli_t *Q);
};


/*
 * Many reader Many writer capiable queue
 * - lock free
 * - allocation free (post initialization)
 */
class MultiQueue
{
    qli_t *pool;
    LockFreeQueue m_free;
    LockFreeQueue m_msgs;

    public:
    MultiQueue(void);
    ~MultiQueue(void);
    void dump(void);
    qli_t *alloc(void)   { return m_free.read();   }
    void free(qli_t  *q) {        m_free.write(q); }
    void write(qli_t *q) {        m_msgs.write(q); }
    qli_t *read(void)    { return m_msgs.read();   }
};

}
