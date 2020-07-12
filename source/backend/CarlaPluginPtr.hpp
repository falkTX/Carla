/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_CPP_COMPAT_HPP_INCLUDED
#define CARLA_CPP_COMPAT_HPP_INCLUDED

#include "CarlaDefines.h"

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <memory>
#else
# include <algorithm>
# include "CarlaUtils.hpp"
#endif

// -----------------------------------------------------------------------

#ifndef CARLA_PROPER_CPP11_SUPPORT
namespace std {

/* This code is part of shared_ptr.hpp:
 *   minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
 * Copyright (c) 2013-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 * Distributed under the MIT License (MIT)
 */
class shared_ptr_count
{
public:
    shared_ptr_count() : pn(nullptr) {}
    shared_ptr_count(const shared_ptr_count& count) : pn(count.pn) {}
    void swap(shared_ptr_count& lhs) noexcept { std::swap(pn, lhs.pn); }

    long use_count(void) const noexcept
    {
        long count = 0;
        if (nullptr != pn)
        {
            count = *pn;
        }
        return count;
    }

    template<class U>
    void acquire(U* p)
    {
        if (nullptr != p)
        {
            if (nullptr == pn)
            {
                try
                {
                    pn = new volatile long(1);
                }
                catch (std::bad_alloc&)
                {
                    delete p;
                    throw;
                }
            }
            else
            {
                ++(*pn);
            }
        }
    }

    template<class U>
    void release(U* p) noexcept
    {
        if (nullptr != pn)
        {
            --(*pn);
            if (0 == *pn)
            {
                delete p;
                delete pn;
            }
            pn = nullptr;
        }
    }

public:
    volatile long* pn;
};

template<class T>
class shared_ptr
{
public:
    typedef T element_type;

    shared_ptr(void) noexcept
      : px(nullptr),
        pn() {}

    /*explicit*/ shared_ptr(T* p)
      : px(nullptr),
        pn()
    {
        acquire(p);
    }

    template <class U>
    shared_ptr(const shared_ptr<U>& ptr, T* p) :
       px(nullptr),
       pn(ptr.pn)
    {
       acquire(p);
    }

    template <class U>
    shared_ptr(const shared_ptr<U>& ptr) noexcept :
        px(nullptr),
        pn(ptr.pn)
    {
        CARLA_SAFE_ASSERT_RETURN(nullptr == ptr.px || 0 != ptr.pn.use_count(),);
        acquire(static_cast<typename shared_ptr<T>::element_type*>(ptr.px));
    }

    shared_ptr(const shared_ptr& ptr) noexcept :
        px(nullptr),
        pn(ptr.pn)
    {
        CARLA_SAFE_ASSERT_RETURN(nullptr == ptr.px || 0 != ptr.pn.use_count(),);
        acquire(ptr.px);
    }

    shared_ptr& operator=(shared_ptr ptr) noexcept
    {
        swap(ptr);
        return *this;
    }

    ~shared_ptr(void) noexcept
    {
        release();
    }

    void reset(void) noexcept
    {
        release();
    }

    void swap(shared_ptr& lhs) noexcept
    {
        std::swap(px, lhs.px);
        pn.swap(lhs.pn);
    }

    operator bool() const noexcept
    {
        return (0 < pn.use_count());
    }
    long use_count(void)  const noexcept
    {
        return pn.use_count();
    }

    // underlying pointer operations :
    T& operator*() const noexcept
    {
        return *px;
    }
    T* operator->() const noexcept
    {
        return px;
    }
    T* get(void) const noexcept
    {
        return px;
    }

private:
    void acquire(T* p)
    {
        pn.acquire(p);
        px = p;
    }

    void release(void) noexcept
    {
        pn.release(px);
        px = nullptr;
    }

private:
    template<class U>
    friend class shared_ptr;

private:
    T*               px;
    shared_ptr_count pn;
};

template<class T, class U> bool operator==(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() == r.get());
}
template<class T, class U> bool operator!=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() != r.get());
}
template<class T, class U> bool operator<=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() <= r.get());
}
template<class T, class U> bool operator<(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() < r.get());
}
template<class T, class U> bool operator>=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() >= r.get());
}
template<class T, class U> bool operator>(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept
{
    return (l.get() > r.get());
}
template<class T, class U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr)
{
    return shared_ptr<T>(ptr, static_cast<typename shared_ptr<T>::element_type*>(ptr.get()));
}
template<class T, class U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr)
{
    T* p = dynamic_cast<typename shared_ptr<T>::element_type*>(ptr.get());
    if (nullptr != p)
    {
        return shared_ptr<T>(ptr, p);
    }
    else
    {
        return shared_ptr<T>();
    }
}
template<class T>
bool operator==(const shared_ptr<T>& pointer1, T* const pointer2) noexcept
{
    return static_cast<T*>(pointer1) == pointer2;
}
template<class T>
bool operator!=(const shared_ptr<T>& pointer1, T* const pointer2) noexcept
{
    return static_cast<T*>(pointer1) != pointer2;
}

} // namespace std
#endif // CARLA_PROPER_CPP11_SUPPORT

CARLA_BACKEND_START_NAMESPACE

typedef std::shared_ptr<CarlaPlugin> CarlaPluginPtr;

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

#endif // CARLA_CPP_COMPAT_HPP_INCLUDED
