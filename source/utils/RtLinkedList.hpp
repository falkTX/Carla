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
        Pool(const std::size_t minPreallocated, const std::size_t maxPreallocated) noexcept
            : kDataSize(sizeof(typename AbstractLinkedList<T>::Data)),
              fHandle(nullptr)
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
            CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

            rtsafe_memory_pool_deallocate(fHandle, dataPtr);
        }

        void resize(const std::size_t minPreallocated, const std::size_t maxPreallocated) noexcept
        {
            if (fHandle != nullptr)
            {
                rtsafe_memory_pool_destroy(fHandle);
                fHandle = nullptr;
            }

            rtsafe_memory_pool_create(&fHandle, nullptr, kDataSize, minPreallocated, maxPreallocated);
            CARLA_SAFE_ASSERT(fHandle != nullptr);
        }

        bool operator==(const Pool& pool) const noexcept
        {
            return (fHandle == pool.fHandle && kDataSize == pool.kDataSize);
        }

        bool operator!=(const Pool& pool) const noexcept
        {
            return (fHandle != pool.fHandle || kDataSize != pool.kDataSize);
        }

    private:
        const std::size_t kDataSize;

        mutable RtMemPool_Handle fHandle;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(Pool)
    };

    // -------------------------------------------------------------------
    // Now the actual rt-linkedlist code

    RtLinkedList(Pool& memPool) noexcept
        : fMemPool(memPool) {}

    bool append_sleepy(const T& value) noexcept
    {
        return _add_sleepy(value, true);
    }

    bool insert_sleepy(const T& value) noexcept
    {
        return _add_sleepy(value, false);
    }

    void resize(const std::size_t minPreallocated, const std::size_t maxPreallocated) noexcept
    {
        CARLA_SAFE_ASSERT(this->fCount == 0);
        this->clear();

        fMemPool.resize(minPreallocated, maxPreallocated);
    }

    bool moveTo(RtLinkedList<T>& list, const bool inTail) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fMemPool == list.fMemPool, false);

        return AbstractLinkedList<T>::moveTo(list, inTail);
    }

protected:
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
        fMemPool.deallocate(dataPtr);
    }

private:
    Pool& fMemPool;

    bool _add_sleepy(const T& value, const bool inTail) noexcept
    {
        return this->_add_internal(_allocate_sleepy(), value, inTail, &this->fQueue);
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(RtLinkedList)
};

// -----------------------------------------------------------------------

#endif // RT_LINKED_LIST_HPP_INCLUDED
