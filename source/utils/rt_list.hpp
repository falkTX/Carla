/*
 * High-level, real-time safe, templated C++ doubly linked list
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __RT_LIST_HPP__
#define __RT_LIST_HPP__

extern "C" {
#include "rtmempool/list.h"
#include "rtmempool/rtmempool.h"
}

#include <cassert>
#include <cstring>

typedef struct list_head k_list_head;

template<typename T>
class RtList
{
public:
    RtList(const size_t minPreallocated, const size_t maxPreallocated)
    {
        qcount = 0;
        ::INIT_LIST_HEAD(&queue);

        ::rtsafe_memory_pool_create(&mempool, nullptr, sizeof(RtListData), minPreallocated, maxPreallocated);

        assert(mempool);
    }

    ~RtList()
    {
        clear();

        ::rtsafe_memory_pool_destroy(mempool);
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated)
    {
        clear();

        ::rtsafe_memory_pool_destroy(mempool);
        ::rtsafe_memory_pool_create(&mempool, nullptr, sizeof(RtListData), minPreallocated, maxPreallocated);

        assert(mempool);
    }

    void clear()
    {
        if (! isEmpty())
        {
            RtListData* data;
            k_list_head* entry;

            list_for_each(entry, &queue)
            {
                data = list_entry(entry, RtListData, siblings);
                ::rtsafe_memory_pool_deallocate(mempool, data);
            }
        }

        qcount = 0;
        ::INIT_LIST_HEAD(&queue);
    }

    size_t count() const
    {
        return qcount;
    }

    bool isEmpty() const
    {
        return (qcount == 0);
        //return (list_empty(&queue) != 0);
    }

    void append(const T& value, const bool sleepy = false)
    {
        RtListData* data;

        if (sleepy)
            data = (RtListData*)::rtsafe_memory_pool_allocate_sleepy(mempool);
        else
            data = (RtListData*)::rtsafe_memory_pool_allocate_atomic(mempool);

        if (data)
        {
            ::memcpy(&data->value, &value, sizeof(T));
            ::list_add_tail(&data->siblings, &queue);

            qcount++;
        }
    }

    T& getFirst()
    {
        return __get(true, false);
    }

    T& getFirstAndRemove()
    {
        return __get(true, true);
    }

    T& getLast()
    {
        return __get(false, false);
    }

    T& getLastAndRemove()
    {
        return __get(false, true);
    }

    bool removeOne(const T& value)
    {
        RtListData* data;
        k_list_head* entry;

        list_for_each(entry, &queue)
        {
            data = list_entry(entry, RtListData, siblings);

            if (data->value == value)
            {
                qcount--;
                ::list_del(entry);
                ::rtsafe_memory_pool_deallocate(mempool, data);
                return true;
            }
        }

        return false;
    }

    void removeAll(const T& value)
    {
        RtListData* data;
        k_list_head* entry;
        k_list_head* tmp;

        list_for_each_safe(entry, tmp, &queue)
        {
            data = list_entry(entry, RtListData, siblings);

            if (data->value == value)
            {
                qcount--;
                ::list_del(entry);
                ::rtsafe_memory_pool_deallocate(mempool, data);
            }
        }
    }

private:
    size_t qcount;
    k_list_head queue;
    RtMemPool_Handle mempool;

    struct RtListData {
        T value;
        k_list_head siblings;
    };

    T& __get(const bool first, const bool doDelete)
    {
        if (isEmpty())
        {
            // FIXME ?
            static T value;
            static bool reset = true;

            if (reset)
            {
                reset = false;
                ::memset(&value, 0, sizeof(T));
            }

            return value;
        }

        k_list_head* entry = first ? queue.next : queue.prev;
        RtListData* data = list_entry(entry, RtListData, siblings);

        T& ret = data->value;

        if (data && doDelete)
        {
            qcount--;
            ::list_del(entry);
            ::rtsafe_memory_pool_deallocate(mempool, data);
        }

        return ret;
    }

    // Non-copyable
    RtList(const RtList&);
    RtList& operator= (const RtList&);

    // Prevent heap allocation
    static void* operator new (size_t);
    static void operator delete (void*);
};

#endif // __RT_LIST_HPP__
