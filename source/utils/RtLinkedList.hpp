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
              kDataSize(sizeof(typename AbstractLinkedList<T>::Data))
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
        mutable RtMemPool_Handle fHandle;
        const size_t             kDataSize;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(Pool)
    };

    // -------------------------------------------------------------------
    // Now the actual rt-linkedlist code

    RtLinkedList(Pool& memPool, const bool isClass = false) noexcept
        : AbstractLinkedList<T>(isClass),
          fMemPool(memPool) {}

    bool append_sleepy(const T& value) noexcept
    {
        return _add_sleepy(value, true);
    }

    bool insert_sleepy(const T& value) noexcept
    {
        return _add_sleepy(value, false);
    }

    void resize(const size_t minPreallocated, const size_t maxPreallocated) noexcept
    {
        this->clear();

        fMemPool.resize(minPreallocated, maxPreallocated);
    }

    void spliceAppendTo(RtLinkedList<T>& list) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fMemPool == list.fMemPool,);

        AbstractLinkedList<T>::spliceAppendTo(list);
    }

    void spliceInsertInto(RtLinkedList<T>& list) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fMemPool == list.fMemPool,);

        AbstractLinkedList<T>::spliceInsertInto(list);
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
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

        fMemPool.deallocate(dataPtr);
    }

private:
    Pool& fMemPool;

    bool _add_sleepy(const T& value, const bool inTail) noexcept
    {
        if (typename AbstractLinkedList<T>::Data* const data = _allocate_sleepy())
        {
            this->_createData(data, value);

            if (inTail)
                this->__list_add(data->siblings, this->fQueue.prev, &(this->fQueue));
            else
                this->__list_add(data->siblings, &(this->fQueue), this->fQueue.next);

            return true;
        }

        return false;
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(RtLinkedList)
};

// -----------------------------------------------------------------------

#endif // RT_LINKED_LIST_HPP_INCLUDED
