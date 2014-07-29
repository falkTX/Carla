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

// -----------------------------------------------------------------------
// Define list_entry and list_entry_const

#ifndef offsetof
# define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#if (defined(__GNUC__) || defined(__clang__)) && ! defined(__STRICT_ANSI__)
# define container_of(ptr, type, member) ({                  \
    typeof( ((type *)0)->member ) *__mptr = (ptr);          \
    (type *)( (char *)__mptr - offsetof(type, member) );})
# define container_of_const(ptr, type, member) ({            \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (const type *)( (const char *)__mptr - offsetof(type, member) );})
# define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
# define list_entry_const(ptr, type, member) \
    container_of_const(ptr, type, member)
#else
# define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-offsetof(type, member)))
# define list_entry_const(ptr, type, member) \
    ((const type *)((const char *)(ptr)-offsetof(type, member)))
#endif

// -----------------------------------------------------------------------
// Abstract Linked List class
// _allocate() and _deallocate are virtual calls provided by subclasses

// NOTE: data-type classes are not allowed to throw

template<typename T>
class AbstractLinkedList
{
protected:
    struct ListHead {
        ListHead* next;
        ListHead* prev;
    };

    struct Data {
        T value;
        ListHead siblings;
    };

    AbstractLinkedList(const bool isClass) noexcept
        : kIsClass(isClass),
          kDataSize(sizeof(Data)),
          fCount(0),
          fQueue({&fQueue, &fQueue}) {}

public:
    virtual ~AbstractLinkedList() noexcept
    {
        CARLA_SAFE_ASSERT(fCount == 0);
    }

    class Itenerator {
    public:
        Itenerator(const ListHead& queue) noexcept
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
            return (fEntry != nullptr && fEntry != &kQueue);
        }

        void next() noexcept
        {
            fEntry  = fEntry2;
            fEntry2 = (fEntry != nullptr) ? fEntry->next : nullptr;
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
        ListHead* fEntry;
        ListHead* fEntry2;
        const ListHead& kQueue;

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

        ListHead* entry;
        ListHead* entry2;

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
        ListHead* entry;
        ListHead* entry2;

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
        ListHead* entry;
        ListHead* entry2;

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
        ListHead* entry;
        ListHead* entry2;

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
        ListHead* entry;
        ListHead* entry2;

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
        ListHead* entry;
        ListHead* entry2;

        for (entry = fQueue.next, entry2 = entry->next; entry != &fQueue; entry = entry2, entry2 = entry->next)
        {
            Data* const data = list_entry(entry, Data, siblings);
            CARLA_SAFE_ASSERT_CONTINUE(data != nullptr);

            if (data->value != value)
                continue;

            _delete(entry, data);
        }
    }

    void spliceAppendTo(AbstractLinkedList<T>& list) noexcept
    {
        if (fQueue.next == &fQueue)
            return;

        __list_splice_tail(&fQueue, &list.fQueue);
        list.fCount += fCount;

        _init();
    }

    void spliceInsertInto(AbstractLinkedList<T>& list) noexcept
    {
        if (fQueue.next == &fQueue)
            return;

        __list_splice(&fQueue, &list.fQueue);
        list.fCount += fCount;

        _init();
    }

protected:
    const bool   kIsClass;
    const size_t kDataSize;

    size_t   fCount;
    ListHead fQueue;

    virtual Data* _allocate() noexcept = 0;
    virtual void  _deallocate(Data* const dataPtr) noexcept = 0;

private:
    void _init() noexcept
    {
        fCount = 0;
        fQueue.next = &fQueue;
        fQueue.prev = &fQueue;
    }

    void _createData(Data* const data, const T& value) noexcept
    {
        if (kIsClass)
            new(data)Data({value, {nullptr, nullptr}});
        else
            data->value = value;

        ++fCount;
    }

    bool _add(const T& value, const bool inTail, ListHead* const queue) noexcept
    {
        if (Data* const data = _allocate())
        {
            _createData(data, value);

            if (inTail)
                __list_add(data->siblings, queue->prev, queue);
            else
                __list_add(data->siblings, queue, queue->next);

            return true;
        }

        return false;
    }

    void _delete(ListHead* const entry, Data* const data) noexcept
    {
        __list_del(entry->prev, entry->next);
        entry->next = nullptr;
        entry->prev = nullptr;

        --fCount;

        if (kIsClass)
            data->~Data();
        _deallocate(data);
    }

    const T& _get(ListHead* const entry, const T& fallback) const noexcept
    {
        const Data* const data = list_entry_const(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        return data->value;
    }

    T& _get(ListHead* const entry, T& fallback) const noexcept
    {
        Data* const data = list_entry(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        return data->value;
    }

    T _get(ListHead* const entry, T& fallback, const bool removeObj) noexcept
    {
        Data* const data = list_entry(entry, Data, siblings);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, fallback);

        if (! removeObj)
            return data->value;

        const T value(data->value);

        _delete(entry, data);

        return value;
    }

   /*
    * Insert a new entry between two known consecutive entries.
    */
    static void __list_add(ListHead& new_, ListHead* const prev, ListHead* const next) noexcept
    {
        next->prev = &new_;
        new_.next  = next;
        new_.prev  = prev;
        prev->next = &new_;
    }

   /*
    * Delete a list entry by making the prev/next entries
    * point to each other.
    */
    static void __list_del(ListHead* const prev, ListHead* const next) noexcept
    {
        next->prev = prev;
        prev->next = next;
    }

    static void __list_splice(ListHead* const list, ListHead* const head) noexcept
    {
        ListHead* const first = list->next;
        ListHead* const last = list->prev;
        ListHead* const at = head->next;

        first->prev = head;
        head->next = first;

        last->next = at;
        at->prev = last;
    }

    static void __list_splice_tail(ListHead* const list, ListHead* const head) noexcept
    {
        ListHead* const first = list->next;
        ListHead* const last = list->prev;
        ListHead* const at = head->prev;

        first->prev = at;
        at->next = first;

        last->next = head;
        head->prev = last;
    }

    template<typename> friend class RtLinkedList;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(AbstractLinkedList)
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

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(LinkedList)
};

// -----------------------------------------------------------------------

#endif // LINKED_LIST_HPP_INCLUDED
