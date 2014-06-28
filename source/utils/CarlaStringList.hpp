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

#include "CarlaString.hpp"
#include "LinkedList.hpp"

// -----------------------------------------------------------------------
// Helper class to manage the lifetime of a "char**" object

class CharStringListPtr
{
public:
    CharStringListPtr() noexcept
        : charList(nullptr) {}

    CharStringListPtr(const char* const* const c) noexcept
        : charList(c) {}

    CharStringListPtr(const CharStringListPtr& ptr) noexcept
        : charList(nullptr)
    {
        copy(ptr.charList);
    }

    CharStringListPtr(const LinkedList<CarlaString>& list) noexcept
        : charList(nullptr)
    {
        copy(list);
    }

    ~CharStringListPtr() noexcept
    {
        clear();
    }

    operator const char* const*() const noexcept
    {
        return charList;
    }

    CharStringListPtr& operator=(const char* const* const c) noexcept
    {
        clear();
        charList = c;
        return *this;
    }

    CharStringListPtr& operator=(const CharStringListPtr& ptr) noexcept
    {
        clear();
        copy(ptr.charList);
        return *this;
    }

    CharStringListPtr& operator=(const LinkedList<CarlaString>& list) noexcept
    {
        clear();
        copy(list);
        return *this;
    }

protected:
    void clear() noexcept
    {
        if (charList == nullptr)
            return;

        for (int i=0; charList[i] != nullptr; ++i)
            delete[] charList[i];

        delete[] charList;
        charList = nullptr;
    }

    void copy(const char* const* const c) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(c != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(charList == nullptr,);

        size_t count = 0;
        for (; c[count] != nullptr; ++count) {}
        CARLA_SAFE_ASSERT_RETURN(count > 0,);

        const char** tmpList;

        try {
            tmpList = new const char*[count+1];
        } CARLA_SAFE_EXCEPTION_RETURN("CharStringListPtr::copy",);

        for (size_t i=0; i<count; ++i)
        {
            try {
                tmpList[i] = carla_strdup(c[i]);
            }
            catch(...) {
                tmpList[i] = nullptr;
                break;
            }
        }

        tmpList[count] = nullptr;
        charList = tmpList;
    }

    void copy(const LinkedList<CarlaString>& list) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(charList == nullptr,);

        const size_t count(list.count());
        CARLA_SAFE_ASSERT_RETURN(count > 0,);

        const char** tmpList;

        try {
            tmpList = new const char*[count+1];
        } CARLA_SAFE_EXCEPTION_RETURN("CharStringListPtr::copy",);

        size_t i=0;
        for (LinkedList<CarlaString>::Itenerator it = list.begin(); it.valid(); it.next(), ++i)
        {
            const CarlaString& string(it.getValue());

            try {
                tmpList[i] = string.dup();
            }
            catch(...) {
                tmpList[i] = nullptr;
                break;
            }
        }

        tmpList[count] = nullptr;
        charList = tmpList;
    }

private:
    const char* const* charList;
};

// -----------------------------------------------------------------------
// CarlaStringList

class CarlaStringList : public LinkedList<CarlaString>
{
public:
    CarlaStringList() noexcept
        : LinkedList<CarlaString>(true) {}

#if 0
    CarlaStringList(const CarlaStringList& list) noexcept
        : LinkedList<CarlaString>(true)
    {
        for (Itenerator it = list.begin(); it.valid(); it.next())
            LinkedList<CarlaString>::append(it.getValue());
    }
#endif

    ~CarlaStringList() noexcept
    {
        clear();
    }

    bool append(const char* const strBuf) noexcept
    {
        const CarlaString string(strBuf);
        return LinkedList<CarlaString>::append(string);
    }

    bool appendAt(const char* const strBuf, const Itenerator& it) noexcept
    {
        const CarlaString string(strBuf);
        return LinkedList<CarlaString>::appendAt(string, it);
    }

    bool insert(const char* const strBuf) noexcept
    {
        const CarlaString string(strBuf);
        return LinkedList<CarlaString>::insert(string);
    }

    bool insertAt(const char* const strBuf, const Itenerator& it) noexcept
    {
        const CarlaString string(strBuf);
        return LinkedList<CarlaString>::insertAt(string, it);
    }

    CharStringListPtr toCharStringListPtr() const noexcept
    {
        return CharStringListPtr(*this);
    }

private:
    LinkedList<CarlaString> fList;
};

// -----------------------------------------------------------------------

#endif // CARLA_STRING_LIST_HPP_INCLUDED
