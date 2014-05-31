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

// NOTE: data-type classes are not allowed to throw

template<typename T>
class AbstractLinkedList
{
protected:
    struct Data {
        T value;
        k_list_head siblings;
    };

    AbstractLinkedList(const bool isClass) noexcept
        : kIsClass(isClass),
          kDataSize(sizeof(Data))
    {
        _init();
    }

public:
    virtual ~AbstractLinkedList() noexcept
    {
        CARLA_SAFE_ASSERT(fCount == 0);
    }

    class Itenerator {
    public:
        Itenerator(const k_list_head& queue) noexcept
            : fData(nullptr),
              fEntry(queue.next),
              fEntry2(fEntry->next),
              kQueue(queue)
        {
            CARLA_SAFE_ASSERT(fEntry != nullptr);
            CARLA_SAFE_ASSERT(fEntry2 != nullptr);
        }

        bool valid() const noexcept
        {
            return (fEntry != &kQueue);
        }

        void next() noexcept
        {
            fEntry  = fEntry2;
            fEntry2 = fEntry->next;
        }

        T& getValue() noexcept
        {
            fData = list_entry(fEntry, Data, siblings);
            return fData->value;
        }

        void setValue(const T& value) noexcept
        {
            fData = list_entry(fEntry, Data, siblings);
            fData->value = value;
        }

    private:
        Data* fData;
        k_list_head* fEntry;
        k_list_head* fEntry2;
        const k_list_head& kQueue;

        friend class AbstractLinkedList;
    };

    Itenerator begin() const noexcept
    {
        return Itenerator(fQueue);
    }

    void clear() noexcept
    {
        if (fCount == 0)
            return;

        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            Data* const data = list_entry(entry, Data, siblings);
            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (kIsClass)
                data->~Data();
            _deallocate(data);
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

    bool append(const T& value) noexcept
    {
        return _add(value, true, &fQueue);
    }

    bool appendAt(const T& value, const Itenerator& it) noexcept
    {
        return _add(value, true, it.fEntry->next);
    }

    bool insert(const T& value) noexcept
    {
        return _add(value, false, &fQueue);
    }

    bool insertAt(const T& value, const Itenerator& it) noexcept
    {
        return _add(value, false, it.fEntry->prev);
    }

    const T& getAt(const size_t index, const T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0 && index < fCount, fallback);

        size_t i = 0;
        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            if (index != i++)
                continue;

            return _get(entry, fallback);
        }

        return fallback;
    }

    T& getAt(const size_t index, T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0 && index < fCount, fallback);

        size_t i = 0;
        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            if (index != i++)
                continue;

            return _get(entry, fallback);
        }

        return fallback;
    }

    T getAt(const size_t index, T& fallback, const bool removeObj) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0 && index < fCount, fallback);

        size_t i = 0;
        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            if (index != i++)
                continue;

            return _get(entry, fallback, removeObj);
        }

        return fallback;
    }

    const T& getFirst(const T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.next, fallback);
    }

    T& getFirst(T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.next, fallback);
    }

    T getFirst(T& fallback, const bool removeObj) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.next, fallback, removeObj);
    }

    const T& getLast(const T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.prev, fallback);
    }

    T& getLast(T& fallback) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.prev, fallback);
    }

    T getLast(T& fallback, const bool removeObj) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCount > 0, fallback);

        return _get(fQueue.prev, fallback, removeObj);
    }

    void remove(Itenerator& it) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(it.fEntry != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(it.fData != nullptr,);

        _delete(it.fEntry, it.fData);
    }

    bool removeOne(const T& value) noexcept
    {
        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            Data* const data = list_entry(entry, Data, siblings);
            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (data->value != value)
                continue;

            _delete(entry, data);

            return true;
        }

        return false;
    }

    void removeAll(const T& value) noexcept
    {
        k_list_head* entry;
        k_list_head* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            Data* const data = list_entry(entry, Data, siblings);
            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (data->value != value)
                continue;

            _delete(entry, data);
        }
    }

    void spliceAppend(AbstractLinkedList<T>& list) noexcept
    {
        list_splice_tail_init(&fQueue, &list.fQueue);
        list.fCount += fCount;
        fCount = 0;
    }

    void spliceInsert(AbstractLinkedList<T>& list) noexcept
    {
        list_splice_init(&fQueue, &list.fQueue);
        list.fCount += fCount;
        fCount = 0;
    }

protected:
    const bool   kIsClass;
    const size_t kDataSize;
          size_t fCount;
    k_list_head  fQueue;

    virtual Data* _allocate() noexcept = 0;
    virtual void  _deallocate(Data* const dataPtr) noexcept = 0;

    void _createData(Data* const data, const T& value) noexcept
    {
        if (kIsClass)
            new(data)Data();

        data->value = value;
        ++fCount;
    }

private:
    void _init() noexcept
    {
        fCount = 0;
        fQueue.next = &fQueue;
        fQueue.prev = &fQueue;
    }

    bool _add(const T& value, const bool inTail, k_list_head* const queue) noexcept
    {
        if (Data* const data = _allocate())
        {
            _createData(data, value);

            if (inTail)
                list_add_tail(&data->siblings, queue);
            else
                list_add(&data->siblings, queue);

            return true;
        }

        return false;
    }

    void _delete(k_list_head* const entry, Data* const data) noexcept
    {
        --fCount;
        list_del(entry);

        if (kIsClass)
            data->~Data();
        _deallocate(data);
    }

    const T& _get(k_list_head* const entry, const T& fallback) const noexcept
    {
        const Data* const data = list_entry(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        return data->value;
    }

    T& _get(k_list_head* const entry, T& fallback) const noexcept
    {
        Data* const data = list_entry(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        return data->value;
    }

    T _get(k_list_head* const entry, T& fallback, const bool removeObj) noexcept
    {
        Data* const data = list_entry(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        if (! removeObj)
            return data->value;

        const T value(data->value);

        _delete(entry, data);

        return value;
    }

    LINKED_LIST_DECLARATIONS(AbstractLinkedList)
};

// -----------------------------------------------------------------------
// LinkedList

template<typename T>
class LinkedList : public AbstractLinkedList<T>
{
public:
    LinkedList(const bool isClass = false) noexcept
        : AbstractLinkedList<T>(isClass) {}

protected:
    typename AbstractLinkedList<T>::Data* _allocate() noexcept override
    {
        return (typename AbstractLinkedList<T>::Data*)std::malloc(this->kDataSize);
    }

    void _deallocate(typename AbstractLinkedList<T>::Data* const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

        std::free(dataPtr);
    }

    LINKED_LIST_DECLARATIONS(LinkedList)
};

// -----------------------------------------------------------------------

#endif // LINKED_LIST_HPP_INCLUDED
