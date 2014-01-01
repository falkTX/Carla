/*
 * High-level, templated, C++ doubly-linked list
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

#ifndef LIST_HPP_INCLUDED
#define LIST_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <new>

extern "C" {
#include "rtmempool/list.h"
}

// Declare non copyable and prevent heap allocation
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define LIST_DECLARATIONS(ClassName)                \
    ClassName(ClassName&) = delete;                  \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete; \
    static void* operator new(size_t) = delete;
#else
# define LIST_DECLARATIONS(ClassName) \
    ClassName(ClassName&);            \
    ClassName(const ClassName&);      \
    ClassName& operator=(const ClassName&);
#endif

typedef struct list_head k_list_head;

// -----------------------------------------------------------------------
// Abstract List class
// _allocate() and _deallocate are virtual calls provided by subclasses

template<typename T>
class AbstractList
{
protected:
    struct Data {
        T value;
        k_list_head siblings;
    };

    AbstractList()
        : fDataSize(sizeof(Data)),
          fCount(0)
    {
        _init();
    }

    virtual ~AbstractList()
    {
        CARLA_ASSERT(fCount == 0);
    }

public:
    class Itenerator {
    public:
        Itenerator(const k_list_head* queue)
            : fEntry(queue->next),
              fEntry2(fEntry->next),
              fQueue(queue),
              fData(nullptr)
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

        T& getValue()
        {
            fData = list_entry(fEntry, Data, siblings);
            CARLA_ASSERT(fData != nullptr);
            return fData->value;
        }

        const T& getConstValue()
        {
            fConstData = list_entry_const(fEntry, Data, siblings);
            CARLA_ASSERT(fConstData != nullptr);
            return fConstData->value;
        }

#if 0
        T& operator*() const
        {
            return getValue();
        }
#endif

    private:
        k_list_head* fEntry;
        k_list_head* fEntry2;
        const k_list_head* const fQueue;

        union {
            Data* fData;
            const Data* fConstData;
        };

        friend class AbstractList;
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

    // temporary fix for some const issue in midi-base.hpp
    void clear_const()
    {
        if (fCount != 0)
        {
            k_list_head* entry;
            k_list_head* entry2;

            list_for_each_safe(entry, entry2, &fQueue)
            {
                if (const Data* data = list_entry_const(entry, Data, siblings))
                {
                    data->~Data();

                    union CData {
                        const Data* cdata;
                        Data* data;
                    };

                    CData d;
                    d.cdata = data;
                    _deallocate(d.data);
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

    void spliceAppend(AbstractList& list, const bool init = true)
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

    void spliceInsert(AbstractList& list, const bool init = true)
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

    LIST_DECLARATIONS(AbstractList)
};

// -----------------------------------------------------------------------
// List

template<typename T>
class List : public AbstractList<T>
{
public:
    List()
    {
    }

private:
    typename AbstractList<T>::Data* _allocate() override
    {
        return (typename AbstractList<T>::Data*)std::malloc(this->fDataSize);
    }

    void _deallocate(typename AbstractList<T>::Data*& dataPtr) override
    {
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr,);

        std::free(dataPtr);
        dataPtr = nullptr;
    }

    LIST_DECLARATIONS(List)
};

// -----------------------------------------------------------------------

#endif // LIST_HPP_INCLUDED
