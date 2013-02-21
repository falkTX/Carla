/*
 * High-level, real-time safe, templated, C++ doubly-linked list
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __RT_LIST_HPP__
#define __RT_LIST_HPP__

#include "CarlaUtils.hpp"

extern "C" {
#include "rtmempool/list.h"
#include "rtmempool/rtmempool.h"
}

// list_entry C++11 version (using nullptr instead of 0)
#undef list_entry
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)nullptr)->member)))

// Declare non copyable and prevent heap allocation
#define LIST_DECLARATIONS(className) \
    className(const className&); \
    className& operator= (const className&); \
    static void* operator new (size_t) { return nullptr; } \
    static void operator delete (void*) {}

typedef struct list_head k_list_head;

// -----------------------------------------------------------------------
// Abstract List class
// _allocate() and _deallocate are virtual calls provided by subclasses

template<typename T>
class List
{
protected:
    List()
        : kDataSize(sizeof(Data)),
          fCount(0)
    {
        _init();
    }

public:
    virtual ~List()
    {
        CARLA_ASSERT(fCount == 0);
    }

    void clear()
    {
        if (fCount != 0)
        {
            k_list_head* entry;

            list_for_each(entry, &fQueue)
            {
                if (Data* data = list_entry(entry, Data, siblings))
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

    bool insert(const T& value)
    {
        if (Data* const data = _allocate())
        {
            std::memcpy(&data->value, &value, sizeof(T));
            list_add(&data->siblings, &fQueue);
            fCount++;
            return true;
        }

        return false;
    }

    T& getAt(const size_t index, const bool remove = false)
    {
        if (fCount == 0 || index >= fCount)
            return _retEmpty();

        size_t i = 0;
        Data* data = nullptr;
        k_list_head* entry;

        list_for_each(entry, &fQueue)
        {
            if (index != i++)
                continue;

            data = list_entry(entry, Data, siblings);

            if (remove)
            {
                fCount--;
                list_del(entry);

                if (data != nullptr)
                    _deallocate(data);
            }

            break;
        }

        CARLA_ASSERT(data != nullptr);

        return (data != nullptr) ? data->value : _retEmpty();
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

            CARLA_ASSERT(data != nullptr);

            if (data != nullptr && data->value == value)
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

            CARLA_ASSERT(data != nullptr);

            if (data != nullptr && data->value == value)
            {
                fCount--;
                list_del(entry);
                _deallocate(data);
            }
        }
    }

    virtual void splice(List& list, const bool init = false)
    {
        if (init)
        {
            list_splice_init(&fQueue, &list.fQueue);
            list.fCount += fCount;
            fCount = 0;
        }
        else
        {
            list_splice(&fQueue, &list.fQueue);
            list.fCount += fCount;
        }
    }

protected:
    const size_t kDataSize;
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

        k_list_head* const entry = first ? fQueue.prev : fQueue.next;
        Data*        const data  = list_entry(entry, Data, siblings);

        CARLA_ASSERT(data != nullptr);

        if (data == nullptr)
            return _retEmpty();

        T& ret = data->value;

        if (data != nullptr && remove)
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
            carla_zeroMem(&value, sizeof(T));
        }

        return value;
    }

    LIST_DECLARATIONS(List)
};

// -----------------------------------------------------------------------
// Realtime safe list

template<typename T>
class RtList : public List<T>
{
public:
    // -------------------------------------------------------------------
    // RtMemPool C++ class

    class Pool
    {
    public:
        Pool(const size_t minPreallocated, const size_t maxPreallocated)
            : fHandle(nullptr),
              kDataSize(sizeof(typename List<T>::Data))
        {
            rtsafe_memory_pool_create(&fHandle, nullptr, kDataSize, minPreallocated, maxPreallocated);
            CARLA_ASSERT(fHandle != nullptr);
        }

        ~Pool()
        {
            if (fHandle != nullptr)
                rtsafe_memory_pool_destroy(fHandle);
        }

        void* allocate_atomic()
        {
            return rtsafe_memory_pool_allocate_atomic(fHandle);
        }

        void* allocate_sleepy()
        {
            return rtsafe_memory_pool_allocate_sleepy(fHandle);
        }

        void deallocate(void* const dataPtr)
        {
            rtsafe_memory_pool_deallocate(fHandle, dataPtr);
        }

        void resize(const size_t minPreallocated, const size_t maxPreallocated)
        {
            if (fHandle != nullptr)
            {
                rtsafe_memory_pool_destroy(fHandle);
                fHandle = nullptr;
            }

            rtsafe_memory_pool_create(&fHandle, nullptr, kDataSize, minPreallocated, maxPreallocated);
            CARLA_ASSERT(fHandle != nullptr);
        }

    private:
        RtMemPool_Handle fHandle;
        const size_t     kDataSize;
    };

    // -------------------------------------------------------------------
    // Now the actual list code

    RtList(Pool* const memPool)
        : kMemPool(memPool)
    {
        CARLA_ASSERT(kMemPool != nullptr);
    }

    ~RtList()
    {
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

    void insert_sleepy(const T& value)
    {
        if (typename List<T>::Data* const data = _allocate_sleepy())
        {
            std::memcpy(&data->value, &value, sizeof(T));
            list_add(&data->siblings, &this->fQueue);
            this->fCount++;
        }
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated)
    {
        this->clear();

        kMemPool->resize(minPreallocated, maxPreallocated);
    }

    void splice(RtList& list, const bool init = false)
    {
        CARLA_ASSERT(kMemPool == list.kMemPool);

        List<T>::splice(list, init);
    }

private:
    Pool* const kMemPool;

    typename List<T>::Data* _allocate()
    {
        return _allocate_atomic();
    }

    typename List<T>::Data* _allocate_atomic()
    {
        return (typename List<T>::Data*)kMemPool->allocate_atomic();
    }

    typename List<T>::Data* _allocate_sleepy()
    {
        return (typename List<T>::Data*)kMemPool->allocate_sleepy();
    }

    void _deallocate(typename List<T>::Data* const dataPtr)
    {
        kMemPool->deallocate(dataPtr);
    }

    LIST_DECLARATIONS(RtList)
};

// -----------------------------------------------------------------------
// Non-Realtime list, using malloc/free methods

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
        return (typename List<T>::Data*)malloc(this->kDataSize);
    }

    void _deallocate(typename List<T>::Data* const dataPtr)
    {
        free(dataPtr);
    }

    LIST_DECLARATIONS(NonRtList)
};

// -----------------------------------------------------------------------
// Non-Realtime list, using new/delete methods

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

    void _deallocate(typename List<T>::Data* const dataPtr)
    {
        delete dataPtr;
    }

    LIST_DECLARATIONS(NonRtListNew)
};

// -----------------------------------------------------------------------

#endif // __RT_LIST_HPP__
