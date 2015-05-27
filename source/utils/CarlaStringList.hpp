/*
 * Carla String List
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_STRING_LIST_HPP_INCLUDED
#define CARLA_STRING_LIST_HPP_INCLUDED

#include "LinkedList.hpp"

// -----------------------------------------------------------------------
// Helper class to manage the lifetime of a "char**" object

class CharStringListPtr
{
public:
    CharStringListPtr() noexcept
        : fCharList(nullptr) {}

    CharStringListPtr(const char* const* const c) noexcept
        : fCharList(c) {}

    CharStringListPtr(const CharStringListPtr& ptr) noexcept
        : fCharList(nullptr)
    {
        copy(ptr.fCharList);
    }

    CharStringListPtr(const LinkedList<const char*>& list) noexcept
        : fCharList(nullptr)
    {
        copy(list);
    }

    ~CharStringListPtr() noexcept
    {
        clear();
    }

    // -------------------------------------------------------------------

    operator const char* const*() const noexcept
    {
        return fCharList;
    }

    CharStringListPtr& operator=(const char* const* const c) noexcept
    {
        clear();
        fCharList = c;
        return *this;
    }

    CharStringListPtr& operator=(const CharStringListPtr& ptr) noexcept
    {
        clear();
        copy(ptr.fCharList);
        return *this;
    }

    CharStringListPtr& operator=(const LinkedList<const char*>& list) noexcept
    {
        clear();
        copy(list);
        return *this;
    }

    // -------------------------------------------------------------------

protected:
    void clear() noexcept
    {
        if (fCharList == nullptr)
            return;

        for (int i=0; fCharList[i] != nullptr; ++i)
            delete[] fCharList[i];

        delete[] fCharList;
        fCharList = nullptr;
    }

    void copy(const char* const* const c) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(c != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fCharList == nullptr,);

        std::size_t count = 0;
        for (; c[count] != nullptr; ++count) {}
        CARLA_SAFE_ASSERT_RETURN(count > 0,);

        const char** tmpList;

        try {
            tmpList = new const char*[count+1];
        } CARLA_SAFE_EXCEPTION_RETURN("CharStringListPtr::copy",);

        tmpList[count] = nullptr;

        for (std::size_t i=0; i<count; ++i)
        {
            tmpList[i] = carla_strdup_safe(c[i]);
            CARLA_SAFE_ASSERT_BREAK(tmpList[i] != nullptr);
        }

        fCharList = tmpList;
    }

    void copy(const LinkedList<const char*>& list) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fCharList == nullptr,);

        const std::size_t count(list.count());
        CARLA_SAFE_ASSERT_RETURN(count > 0,);

        const char** tmpList;

        try {
            tmpList = new const char*[count+1];
        } CARLA_SAFE_EXCEPTION_RETURN("CharStringListPtr::copy",);

        tmpList[count] = nullptr;

        std::size_t i=0;
        for (LinkedList<const char*>::Itenerator it = list.begin2(); it.valid(); it.next(), ++i)
        {
            tmpList[i] = carla_strdup_safe(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_BREAK(tmpList[i] != nullptr);
        }
        CARLA_SAFE_ASSERT(i == count);

        fCharList = tmpList;
    }

    // -------------------------------------------------------------------

private:
    const char* const* fCharList;
};

// -----------------------------------------------------------------------
// CarlaStringList

class CarlaStringList : public LinkedList<const char*>
{
public:
    CarlaStringList() noexcept
        : LinkedList<const char*>() {}

    CarlaStringList(const CarlaStringList& list) noexcept
        : LinkedList<const char*>()
    {
        for (Itenerator it = list.begin2(); it.valid(); it.next())
            LinkedList<const char*>::append(carla_strdup_safe(it.getValue(nullptr)));
    }

    ~CarlaStringList() noexcept override
    {
        clear();
    }

    // -------------------------------------------------------------------

    void clear() noexcept
    {
        for (Itenerator it = begin2(); it.valid(); it.next())
        {
            if (const char* const string = it.getValue(nullptr))
                delete[] string;
        }

        LinkedList<const char*>::clear();
    }

    // -------------------------------------------------------------------

    bool append(const char* const string) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, false);

        if (const char* const stringDup = carla_strdup_safe(string))
        {
            if (LinkedList<const char*>::append(stringDup))
                return true;
            delete[] stringDup;
        }

        return false;
    }

    bool appendAt(const char* const string, const Itenerator& it) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, false);

        if (const char* const stringDup = carla_strdup_safe(string))
        {
            if (LinkedList<const char*>::appendAt(stringDup, it))
                return true;
            delete[] stringDup;
        }

        return false;
    }

    bool insert(const char* const string) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, false);

        if (const char* const stringDup = carla_strdup_safe(string))
        {
            if (LinkedList<const char*>::insert(stringDup))
                return true;
            delete[] stringDup;
        }

        return false;
    }

    bool insertAt(const char* const string, const Itenerator& it) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, false);

        if (const char* const stringDup = carla_strdup_safe(string))
        {
            if (LinkedList<const char*>::insertAt(stringDup, it))
                return true;
            delete[] stringDup;
        }

        return false;
    }

    // -------------------------------------------------------------------

    void remove(Itenerator& it) noexcept
    {
        if (const char* const string = it.getValue(nullptr))
            delete[] string;

        LinkedList<const char*>::remove(it);
    }

    bool removeOne(const char* const string) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, false);

        for (Itenerator it = begin2(); it.valid(); it.next())
        {
            const char* const stringComp(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(stringComp != nullptr);

            if (std::strcmp(string, stringComp) != 0)
                continue;

            delete[] stringComp;
            LinkedList<const char*>::remove(it);
            return true;
        }

        return false;
    }

    void removeAll(const char* const string) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(string != nullptr,);

        for (Itenerator it = begin2(); it.valid(); it.next())
        {
            const char* const stringComp(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(stringComp != nullptr);

            if (std::strcmp(string, stringComp) != 0)
                continue;

            delete[] stringComp;
            LinkedList<const char*>::remove(it);
        }
    }

    // -------------------------------------------------------------------

    CharStringListPtr toCharStringListPtr() const noexcept
    {
        return CharStringListPtr(*this);
    }

    CarlaStringList& operator=(const char* const* const charStringList) noexcept
    {
        clear();

        CARLA_SAFE_ASSERT_RETURN(charStringList != nullptr, *this);

        for (int i=0; charStringList[i] != nullptr; ++i)
        {
            if (const char* const string = carla_strdup_safe(charStringList[i]))
                LinkedList<const char*>::append(string);
        }

        return *this;
    }

    CarlaStringList& operator=(const CarlaStringList& list) noexcept
    {
        clear();

        for (Itenerator it = list.begin2(); it.valid(); it.next())
        {
            if (const char* const string = carla_strdup_safe(it.getValue(nullptr)))
                LinkedList<const char*>::append(string);
        }

        return *this;
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

#endif // CARLA_STRING_LIST_HPP_INCLUDED
