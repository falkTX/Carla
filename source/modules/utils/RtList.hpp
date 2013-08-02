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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef RT_LIST_HPP_INCLUDED
#define RT_LIST_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <new>

extern "C" {
#include "rtmempool/list.h"
#include "rtmempool/rtmempool.h"
}

// Declare non copyable and prevent heap allocation
#define LIST_DECLARATIONS(ClassName)                       \
    ClassName(ClassName&);                                 \
    ClassName(const ClassName&);                           \
    ClassName& operator=(const ClassName&);                \
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
    struct Data {
        T value;
        k_list_head siblings;
    };

    List()
        : fDataSize(sizeof(Data)),
          fCount(0)
    {
        _init();
    }

    virtual ~List()
    {
        CARLA_ASSERT(fCount == 0);
    }

public:
    class Itenerator {
    public:
        Itenerator(const k_list_head* queue)
            : fData(nullptr),
              fEntry(queue->next),
              fEntry2(fEntry->next),
              fQueue(queue)
        {
            CARLA_ASSERT(fEntry != nullptr);
            CARLA_ASSERT(fEntry2 != nullptr);
            CARLA_ASSERT(fQueue != nullptr);
        }

        bool valid() const noexcept
        {
            return (fEntry != fQueue);
        }

        void next() noexcept
        {
            fEntry  = fEntry2;
            fEntry2 = fEntry->next;
        }

        T& operator*()
        {
            fData = list_entry(fEntry, Data, siblings);
            CARLA_ASSERT(fData != nullptr);
            return fData->value;
        }

    private:
        Data* fData;
        k_list_head* fEntry;
        k_list_head* fEntry2;
        const k_list_head* const fQueue;

        friend class List;
    };

    Itenerator begin() const
    {
        return Itenerator(&fQueue);
    }

    void clear()
    {
        if (fCount != 0)
        {
            k_list_head* entry;
            k_list_head* entry2;

            list_for_each_safe(entry, entry2, &fQueue)
            {
                if (Data* data = list_entry(entry, Data, siblings))
                {
                    data->~Data();
                    _deallocate(data);
                }
            }
        }

        _init();
    }

    size_t count() const noexcept
    {
        return fCount;
    }

    bool isEmpty() const noexcept
    {
        return (fCount == 0);
    }

    bool append(const T& value)
    {
        if (Data* const data = _allocate())
        {
            new(data)Data();
            data->value = value;
            list_add_tail(&data->siblings, &fQueue);
            ++fCount;
            return true;
        }

        return false;
    }

    bool appendAt(const T& value, const Itenerator& it)
    {
        if (Data* const data = _allocate())
        {
            new(data)Data();
            data->value = value;
            list_add_tail(&data->siblings, it.fEntry->next);
            ++fCount;
            return true;
        }

        return false;
    }

    bool insert(const T& value)
    {
        if (Data* const data = _allocate())
        {
            new(data)Data();
            data->value = value;
            list_add(&data->siblings, &fQueue);
            ++fCount;
            return true;
        }

        return false;
    }

    bool insertAt(const T& value, const Itenerator& it)
    {
        if (Data* const data = _allocate())
        {
            new(data)Data();
            data->value = value;
            list_add(&data->siblings, it.fEntry->prev);
            ++fCount;
            return true;
        }

        return false;
    }

    T& getAt(const size_t index, const bool remove = false)
    {
        if (fCount == 0 || index >= fCount)
            return fRetValue;

        size_t i = 0;
        Data* data = nullptr;
        k_list_head* entry;
        k_list_head* entry2;

        list_for_each_safe(entry, entry2, &fQueue)
        {
            if (index != i++)
                continue;

            data = list_entry(entry, Data, siblings);

            if (data != nullptr)
                fRetValue = data->value;

            if (remove)
            {
                --fCount;
                list_del(entry);

                if (data != nullptr)
                {
                    data->~Data();
                    _deallocate(data);
                }
            }

            break;
        }

        return fRetValue;
    }

    T& getFirst(const bool remove = false)
    {
        return _getFirstOrLast(true, remove);
    }

    T& getLast(const bool remove = false)
    {
        return _getFirstOrLast(false, remove);
    }

    void remove(Itenerator& it)
    {
        CARLA_ASSERT(it.fEntry != nullptr);
        CARLA_ASSERT(it.fData != nullptr);

        if (it.fEntry != nullptr && it.fData != nullptr)
        {
            --fCount;
            list_del(it.fEntry);

            it.fData->~Data();
            _deallocate(it.fData);
        }
    }

    bool removeOne(const T& value)
    {
        Data* data = nullptr;
        k_list_head* entry;
        k_list_head* entry2;

        list_for_each_safe(entry, entry2, &fQueue)
        {
            data = list_entry(entry, Data, siblings);

            CARLA_ASSERT(data != nullptr);

            if (data != nullptr && data->value == value)
            {
                --fCount;
                list_del(entry);

                data->~Data();
                _deallocate(data);
                break;
            }
        }

        return (data != nullptr);
    }

    void spliceAppend(List& list, const bool init = true)
    {
        if (init)
        {
            list_splice_tail_init(&fQueue, &list.fQueue);
            list.fCount += fCount;
            fCount = 0;
        }
        else
        {
            list_splice_tail(&fQueue, &list.fQueue);
            list.fCount += fCount;
        }
    }

    void spliceInsert(List& list, const bool init = true)
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
    const size_t fDataSize;
          size_t fCount;
    k_list_head  fQueue;

    virtual Data* _allocate() = 0;
    virtual void  _deallocate(Data*& dataPtr) = 0;

private:
    T fRetValue;

    void _init() noexcept
    {
        fCount = 0;
        INIT_LIST_HEAD(&fQueue);
    }

    T& _getFirstOrLast(const bool first, const bool remove)
    {
        if (fCount == 0)
            return fRetValue;

        k_list_head* const entry = first ? fQueue.next : fQueue.prev;
        Data*              data  = list_entry(entry, Data, siblings);

        if (data != nullptr)
            fRetValue = data->value;

        if (remove)
        {
            --fCount;
            list_del(entry);

            if (data != nullptr)
            {
                data->~Data();
                _deallocate(data);
            }
        }

        return fRetValue;
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
              fDataSize(sizeof(typename List<T>::Data))
        {
            resize(minPreallocated, maxPreallocated);
        }

        ~Pool()
        {
            if (fHandle != nullptr)
            {
                rtsafe_memory_pool_destroy(fHandle);
                fHandle = nullptr;
            }
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

            rtsafe_memory_pool_create(&fHandle, nullptr, fDataSize, minPreallocated, maxPreallocated);
            CARLA_ASSERT(fHandle != nullptr);
        }

        bool operator==(const Pool& pool) const noexcept
        {
            return (fHandle == pool.fHandle && fDataSize == pool.fDataSize);
        }

        bool operator!=(const Pool& pool) const noexcept
        {
            return (fHandle != pool.fHandle || fDataSize != pool.fDataSize);
        }

    private:
        RtMemPool_Handle fHandle;
        const size_t     fDataSize;
    };

    // -------------------------------------------------------------------
    // Now the actual list code

    RtList(Pool& memPool)
        : fMemPool(memPool)
    {
    }

    ~RtList() override
    {
    }

    void append_sleepy(const T& value)
    {
        if (typename List<T>::Data* const data = _allocate_sleepy())
        {
            new(data)typename List<T>::Data();
            data->value = value;
            list_add_tail(&data->siblings, &this->fQueue);
            ++this->fCount;
        }
    }

    void insert_sleepy(const T& value)
    {
        if (typename List<T>::Data* const data = _allocate_sleepy())
        {
            new(data)typename List<T>::Data();
            data->value = value;
            list_add(&data->siblings, &this->fQueue);
            ++this->fCount;
        }
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated)
    {
        this->clear();

        fMemPool.resize(minPreallocated, maxPreallocated);
    }

    void spliceAppend(RtList& list, const bool init = true)
    {
        CARLA_ASSERT(fMemPool == list.fMemPool);

        List<T>::spliceAppend(list, init);
    }

    void spliceInsert(RtList& list, const bool init = true)
    {
        CARLA_ASSERT(fMemPool == list.fMemPool);

        List<T>::spliceInsert(list, init);
    }

private:
    Pool& fMemPool;

    typename List<T>::Data* _allocate() override
    {
        return _allocate_atomic();
    }

    typename List<T>::Data* _allocate_atomic()
    {
        return (typename List<T>::Data*)fMemPool.allocate_atomic();
    }

    typename List<T>::Data* _allocate_sleepy()
    {
        return (typename List<T>::Data*)fMemPool.allocate_sleepy();
    }

    void _deallocate(typename List<T>::Data*& dataPtr) override
    {
        CARLA_ASSERT(dataPtr != nullptr);
        fMemPool.deallocate(dataPtr);
        dataPtr = nullptr;
    }

    LIST_DECLARATIONS(RtList)
};

// -----------------------------------------------------------------------
// Non-Realtime list

template<typename T>
class NonRtList : public List<T>
{
public:
    NonRtList()
    {
    }

    ~NonRtList() override
    {
    }

private:
    typename List<T>::Data* _allocate() override
    {
        return (typename List<T>::Data*)std::malloc(this->fDataSize);
    }

    void _deallocate(typename List<T>::Data*& dataPtr) override
    {
        CARLA_ASSERT(dataPtr != nullptr);
        std::free(dataPtr);
        dataPtr = nullptr;
    }

    LIST_DECLARATIONS(NonRtList)
};

// -----------------------------------------------------------------------

#endif // RT_LIST_HPP_INCLUDED
