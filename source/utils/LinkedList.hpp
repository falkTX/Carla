/*
 * High-level, templated, C++ doubly-linked list
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

#ifndef LINKED_LIST_HPP_INCLUDED
#define LINKED_LIST_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <new>

extern "C" {
#include "rtmempool/list.h"
}

// Declare non copyable and prevent heap allocation
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define LINKED_LIST_DECLARATIONS(ClassName)         \
    ClassName(ClassName&) = delete;                  \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete; \
    static void* operator new(size_t) = delete;
#else
# define LINKED_LIST_DECLARATIONS(ClassName) \
    ClassName(ClassName&);                   \
    ClassName(const ClassName&);             \
    ClassName& operator=(const ClassName&);
#endif

typedef struct list_head k_list_head;

// -----------------------------------------------------------------------
// Abstract Linked List class
// _allocate() and _deallocate are virtual calls provided by subclasses

template<typename T>
class AbstractLinkedList
{
protected:
    struct Data {
        T value;
        k_list_head siblings;
    };

    AbstractLinkedList()
        : fDataSize(sizeof(Data)),
          fCount(0)
    {
        _init();
    }

public:
    virtual ~AbstractLinkedList()
    {
        CARLA_SAFE_ASSERT(fCount == 0);
    }

    class Itenerator {
    public:
        Itenerator(const k_list_head* queue) noexcept
            : fData(nullptr),
              fEntry(queue->next),
              fEntry2(fEntry->next),
              fQueue(queue)
        {
            CARLA_SAFE_ASSERT(fEntry != nullptr);
            CARLA_SAFE_ASSERT(fEntry2 != nullptr);
            CARLA_SAFE_ASSERT(fQueue != nullptr);
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

        T& getValue() noexcept
        {
            fData = list_entry(fEntry, Data, siblings);
            CARLA_SAFE_ASSERT(fData != nullptr);
            return fData->value;
        }

    private:
        Data* fData;
        k_list_head* fEntry;
        k_list_head* fEntry2;
        const k_list_head* const fQueue;

        friend class AbstractLinkedList;
    };

    Itenerator begin() const noexcept
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

    T& getAt(const size_t index) const noexcept
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

            break;
        }

        return fRetValue;
    }

    T& getAt(const size_t index, const bool removeObj)
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

            if (removeObj)
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

    T& getFirst(const bool removeObj = false)
    {
        return _getFirstOrLast(true, removeObj);
    }

    T& getLast(const bool removeObj = false)
    {
        return _getFirstOrLast(false, removeObj);
    }

    void remove(Itenerator& it)
    {
        CARLA_SAFE_ASSERT_RETURN(it.fEntry != nullptr,);

        --fCount;
        list_del(it.fEntry);

        CARLA_SAFE_ASSERT_RETURN(it.fData != nullptr,);

        it.fData->~Data();
        _deallocate(it.fData);
    }

    bool removeOne(const T& value)
    {
        Data* data = nullptr;
        k_list_head* entry;
        k_list_head* entry2;

        list_for_each_safe(entry, entry2, &fQueue)
        {
            data = list_entry(entry, Data, siblings);

            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (data->value == value)
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

    void removeAll(const T& value)
    {
        Data* data;
        k_list_head* entry;
        k_list_head* entry2;

        list_for_each_safe(entry, entry2, &fQueue)
        {
            data = list_entry(entry, Data, siblings);

            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (data->value == value)
            {
                --fCount;
                list_del(entry);

                data->~Data();
                _deallocate(data);
            }
        }
    }

    void spliceAppend(AbstractLinkedList& list, const bool init = true) noexcept
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

    void spliceInsert(AbstractLinkedList& list, const bool init = true) noexcept
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
    mutable T fRetValue;

    void _init() noexcept
    {
        fCount = 0;
        INIT_LIST_HEAD(&fQueue);
    }

    T& _getFirstOrLast(const bool first, const bool removeObj)
    {
        if (fCount == 0)
            return fRetValue;

        k_list_head* const entry = first ? fQueue.next : fQueue.prev;
        Data*              data  = list_entry(entry, Data, siblings);

        if (data != nullptr)
            fRetValue = data->value;

        if (removeObj)
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

    LINKED_LIST_DECLARATIONS(AbstractLinkedList)
};

// -----------------------------------------------------------------------
// LinkedList

template<typename T>
class LinkedList : public AbstractLinkedList<T>
{
public:
    LinkedList() {}

private:
    typename AbstractLinkedList<T>::Data* _allocate() override
    {
        return (typename AbstractLinkedList<T>::Data*)std::malloc(this->fDataSize);
    }

    void _deallocate(typename AbstractLinkedList<T>::Data*& dataPtr) override
    {
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

        std::free(dataPtr);
        dataPtr = nullptr;
    }

    LINKED_LIST_DECLARATIONS(LinkedList)
};

// -----------------------------------------------------------------------

#endif // LINKED_LIST_HPP_INCLUDED
