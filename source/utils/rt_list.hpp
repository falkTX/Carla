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

// Declare non copyable and prevent heap allocation
#define LIST_DECLARATIONS(className) \
    className(const className&); \
    className& operator= (const className&); \
    static void* operator new (size_t); \
    static void operator delete (void*); \

typedef struct list_head k_list_head;

// -----------------------------------------------------------------------

template<typename T>
class List
{
protected:
    List()
        : fDataSize(sizeof(Data))
    {
        _init();
    }

public:
    virtual ~List()
    {
        clear();
    }

    void clear()
    {
        if (fCount != 0)
        {
            Data* data;
            k_list_head* entry;

            list_for_each(entry, &fQueue)
            {
                data = list_entry(entry, Data, siblings);
                _deallocate(data);
            }
        }

        _init();
    }

    size_t count() const
    {
        return fCount;
    }

    bool isEmpty() const
    {
        return (fCount == 0);
    }

    bool append(const T& value)
    {
        if (Data* const data = _allocate())
        {
            std::memcpy(&data->value, &value, sizeof(T));
            list_add_tail(&data->siblings, &fQueue);
            fCount++;
            return true;
        }

        return false;
    }

    T& getAt(const size_t index, const bool remove = false)
    {
        if (fCount == 0 || index >= fCount)
            return _retEmpty();

        Data* data = nullptr;
        k_list_head* entry;
        size_t i = 0;

        list_for_each(entry, &fQueue)
        {
            if (index != i++)
                continue;

            data = list_entry(entry, Data, siblings);
            assert(data);

            if (remove)
            {
                fCount--;
                list_del(entry);
                _deallocate(data);
            }

            break;
        }

        assert(data);
        return data->value;
    }

    T& getFirst(const bool remove = false)
    {
        return _getFirstOrLast(true, remove);
    }

    T& getLast(const bool remove = false)
    {
        return _getFirstOrLast(false, remove);
    }

    bool removeOne(const T& value)
    {
        Data* data = nullptr;
        k_list_head* entry;

        list_for_each(entry, &fQueue)
        {
            data = list_entry(entry, Data, siblings);
            assert(data);

            if (data->value == value)
            {
                fCount--;
                list_del(entry);
                _deallocate(data);
                break;
            }
        }

        return (data != nullptr);
    }

    void removeAll(const T& value)
    {
        Data* data;
        k_list_head* entry;
        k_list_head* tmp;

        list_for_each_safe(entry, tmp, &fQueue)
        {
            data = list_entry(entry, Data, siblings);
            assert(data);

            if (data->value == value)
            {
                fCount--;
                list_del(entry);
                _deallocate(data);
            }
        }
    }

    void splice(List& list, const bool init = false)
    {
        if (init)
            list_splice_init(&fQueue, &list.fQueue);
        else
            list_splice(&fQueue, &list.fQueue);
    }

protected:
    const size_t fDataSize;
          size_t fCount;
    k_list_head  fQueue;

    struct Data {
        T value;
        k_list_head siblings;
    };

    virtual Data* _allocate() = 0;
    virtual void  _deallocate(Data* const dataPtr) = 0;

private:
    void _init()
    {
        fCount = 0;
        INIT_LIST_HEAD(&fQueue);
    }

    T& _getFirstOrLast(const bool first, const bool remove)
    {
        if (fCount == 0)
            return _retEmpty();

        k_list_head* const entry = first ? fQueue.next : fQueue.prev;
        Data*        const data  = list_entry(entry, Data, siblings);

        if (data == nullptr)
            return _retEmpty();

        T& ret = data->value;

        if (data && remove)
        {
            fCount--;
            list_del(entry);
            _deallocate(data);
        }

        return ret;
    }

    T& _retEmpty()
    {
        // FIXME ?
        static T value;
        static bool reset = true;

        if (reset)
        {
            reset = false;
            std::memset(&value, 0, sizeof(T));
        }

        return value;
    }

    //LIST_DECLARATIONS(List)
};

template<typename T>
class RtList : public List<T>
{
public:
    RtList(const size_t minPreallocated, const size_t maxPreallocated)
    {
        rtsafe_memory_pool_create(&fMemPool, nullptr, this->fDataSize, minPreallocated, maxPreallocated);
        assert(fMemPool);
    }

    ~RtList()
    {
        if (fMemPool != nullptr)
            rtsafe_memory_pool_destroy(fMemPool);
    }

    void append_sleepy(const T& value)
    {
        if (typename List<T>::Data* const data = _allocate_sleepy())
        {
            std::memcpy(&data->value, &value, sizeof(T));
            list_add_tail(&data->siblings, &this->fQueue);
            this->fCount++;
        }
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated)
    {
        this->clear();

        rtsafe_memory_pool_destroy(fMemPool);
        rtsafe_memory_pool_create(&fMemPool, nullptr, this->fDataSize, minPreallocated, maxPreallocated);
        assert(fMemPool);
    }

private:
    RtMemPool_Handle fMemPool;

    typename List<T>::Data* _allocate()
    {
        return _allocate_atomic();
    }

    typename List<T>::Data* _allocate_atomic()
    {
        return (typename List<T>::Data*)rtsafe_memory_pool_allocate_atomic(fMemPool);
    }

    typename List<T>::Data* _allocate_sleepy()
    {
        return (typename List<T>::Data*)rtsafe_memory_pool_allocate_sleepy(fMemPool);
    }

    void  _deallocate(typename List<T>::Data* const dataPtr)
    {
        rtsafe_memory_pool_deallocate(fMemPool, dataPtr);
    }

    //LIST_DECLARATIONS(RtList)
};

template<typename T>
class NonRtList : public List<T>
{
public:
    NonRtList()
    {
    }

    ~NonRtList()
    {
    }

private:
    typename List<T>::Data* _allocate()
    {
        return (typename List<T>::Data*)malloc(this->fDataSize);
    }

    void  _deallocate(typename List<T>::Data* const dataPtr)
    {
        free(dataPtr);
    }

    //LIST_DECLARATIONS(NonRtList)
};

template<typename T>
class NonRtListNew : public List<T>
{
public:
    NonRtListNew()
    {
    }

    ~NonRtListNew()
    {
    }

private:
    typename List<T>::Data* _allocate()
    {
        return new typename List<T>::Data;
    }

    void  _deallocate(typename List<T>::Data* const dataPtr)
    {
        delete dataPtr;
    }

    //LIST_DECLARATIONS(NonRtListNew)
};

// -----------------------------------------------------------------------

#endif // __RT_LIST_HPP__
