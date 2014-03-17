/*
 * High-level, real-time safe, templated, C++ doubly-linked list
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef RT_LINKED_LIST_HPP_INCLUDED
#define RT_LINKED_LIST_HPP_INCLUDED

#include "LinkedList.hpp"

extern "C" {
#include "rtmempool/rtmempool.h"
}

// -----------------------------------------------------------------------
// Realtime safe linkedlist

template<typename T>
class RtLinkedList : public AbstractLinkedList<T>
{
public:
    // -------------------------------------------------------------------
    // RtMemPool C++ class

    class Pool
    {
    public:
        Pool(const size_t minPreallocated, const size_t maxPreallocated) noexcept
            : fHandle(nullptr),
              fDataSize(sizeof(typename AbstractLinkedList<T>::Data))
        {
            resize(minPreallocated, maxPreallocated);
        }

        ~Pool() noexcept
        {
            if (fHandle != nullptr)
            {
                rtsafe_memory_pool_destroy(fHandle);
                fHandle = nullptr;
            }
        }

        void* allocate_atomic() const noexcept
        {
            return rtsafe_memory_pool_allocate_atomic(fHandle);
        }

        void* allocate_sleepy() const noexcept
        {
            return rtsafe_memory_pool_allocate_sleepy(fHandle);
        }

        void deallocate(void* const dataPtr) const noexcept
        {
            rtsafe_memory_pool_deallocate(fHandle, dataPtr);
        }

        void resize(const size_t minPreallocated, const size_t maxPreallocated) noexcept
        {
            if (fHandle != nullptr)
            {
                rtsafe_memory_pool_destroy(fHandle);
                fHandle = nullptr;
            }

            rtsafe_memory_pool_create(&fHandle, nullptr, fDataSize, minPreallocated, maxPreallocated);
            CARLA_SAFE_ASSERT(fHandle != nullptr);
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
        mutable RtMemPool_Handle fHandle;
        const size_t             fDataSize;
    };

    // -------------------------------------------------------------------
    // Now the actual rt-linkedlist code

    RtLinkedList(Pool& memPool) noexcept
        : fMemPool(memPool) {}

    bool append_sleepy(const T& value) noexcept
    {
        if (typename AbstractLinkedList<T>::Data* const data = _allocate_sleepy())
        {
            try {
                new(data)typename AbstractLinkedList<T>::Data();
            }
            catch(...) {
                _deallocate(data);
                return false;
            }

            data->value = value;
            list_add_tail(&data->siblings, &this->fQueue);
            ++(this->fCount);

            return true;
        }

        return false;
    }

    bool insert_sleepy(const T& value) noexcept
    {
        if (typename AbstractLinkedList<T>::Data* const data = _allocate_sleepy())
        {
            try {
                new(data)typename AbstractLinkedList<T>::Data();
            }
            catch(...) {
                _deallocate(data);
                return false;
            }

            data->value = value;
            list_add(&data->siblings, &this->fQueue);
            ++(this->fCount);

            return true;
        }

        return false;
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated) noexcept
    {
        this->clear();

        fMemPool.resize(minPreallocated, maxPreallocated);
    }

    void spliceAppend(RtLinkedList& list, const bool init = true) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fMemPool == list.fMemPool,);

        AbstractLinkedList<T>::spliceAppend(list, init);
    }

    void spliceInsert(RtLinkedList& list, const bool init = true) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fMemPool == list.fMemPool,);

        AbstractLinkedList<T>::spliceInsert(list, init);
    }

private:
    Pool& fMemPool;

    typename AbstractLinkedList<T>::Data* _allocate() noexcept override
    {
        return (typename AbstractLinkedList<T>::Data*)fMemPool.allocate_atomic();
    }

    typename AbstractLinkedList<T>::Data* _allocate_sleepy() noexcept
    {
        return (typename AbstractLinkedList<T>::Data*)fMemPool.allocate_sleepy();
    }

    void _deallocate(typename AbstractLinkedList<T>::Data* const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

        fMemPool.deallocate(dataPtr);
    }

    LINKED_LIST_DECLARATIONS(RtLinkedList)
};

// -----------------------------------------------------------------------

#endif // RT_LINKED_LIST_HPP_INCLUDED
