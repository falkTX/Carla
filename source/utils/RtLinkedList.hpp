/*
 * High-level, real-time safe, templated, C++ doubly-linked list
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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

#ifdef STOAT_TEST_BUILD
    // overridden for stoat
    bool append(const T& value) noexcept
    {
        if (typename AbstractLinkedList<T>::Data* const data = _allocate())
            return this->_add_internal(data, value, true, &this->fQueue);
        return false;
    }

    void clear() noexcept
    {
        if (this->fCount == 0)
            return;

        for (typename AbstractLinkedList<T>::ListHead *entry = this->fQueue.next, *entry2 = entry->next;
             entry != &this->fQueue; entry = entry2, entry2 = entry->next)
        {
            typename AbstractLinkedList<T>::Data* const data = list_entry(entry, typename AbstractLinkedList<T>::Data, siblings);
            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            this->_deallocate(data);
        }

        this->_init();
    }

    T getFirst(T& fallback, const bool removeObj) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(this->fCount > 0, fallback);

        typename AbstractLinkedList<T>::ListHead* const entry = this->fQueue.next;

        typename AbstractLinkedList<T>::Data* const data = list_entry(entry, typename AbstractLinkedList<T>::Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        if (! removeObj)
            return data->value;

        const T value(data->value);

        _deleteRT(entry, data);

        return value;
    }

    void _deleteRT(typename AbstractLinkedList<T>::ListHead* const entry, typename AbstractLinkedList<T>::Data* const data) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(entry       != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(entry->prev != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(entry->next != nullptr,);

        --this->fCount;

        entry->next->prev = entry->prev;
        entry->prev->next = entry->next;

        entry->next = nullptr;
        entry->prev = nullptr;

        _deallocate(data);
    }
#endif

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
        if (typename AbstractLinkedList<T>::Data* const data = _allocate_sleepy())
            return this->_add_internal(data, value, inTail, &this->fQueue);
        return false;
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(RtLinkedList)
};

// -----------------------------------------------------------------------

#endif // RT_LINKED_LIST_HPP_INCLUDED
