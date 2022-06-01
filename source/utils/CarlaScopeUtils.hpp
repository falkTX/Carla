/*
 * Carla Scope-related classes and tools (pointer and setter taken from JUCE v4)
 * Copyright (C) 2013 Raw Material Software Ltd.
 * Copyright (c) 2016 ROLI Ltd.
 * Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SCOPE_UTILS_HPP_INCLUDED
#define CARLA_SCOPE_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <algorithm>
#include <clocale>

#if ! (defined(CARLA_OS_HAIKU) || defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
# define CARLA_USE_NEWLOCALE
#endif

#if defined(CARLA_OS_WIN) && __MINGW64_VERSION_MAJOR >= 5
# define CARLA_USE_CONFIGTHREADLOCALE
#endif

// -----------------------------------------------------------------------
// CarlaScopedEnvVar class

class CarlaScopedEnvVar {
public:
    CarlaScopedEnvVar(const char* const envVar, const char* const valueOrNull) noexcept
        : key(nullptr),
          origValue(nullptr)
    {
        CARLA_SAFE_ASSERT_RETURN(envVar != nullptr && envVar[0] != '\0',);

        key = carla_strdup_safe(envVar);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);

        if (const char* const envVarValue = std::getenv(key))
        {
            origValue = carla_strdup_safe(envVarValue);
            CARLA_SAFE_ASSERT_RETURN(origValue != nullptr,);
        }

        // change env var if requested
        if (valueOrNull != nullptr)
            carla_setenv(key, valueOrNull);
        // if null, unset. but only if there is in an active env var value
        else if (origValue != nullptr)
            carla_unsetenv(key);
    }

    ~CarlaScopedEnvVar() noexcept
    {
        bool hasOrigValue = false;

        if (origValue != nullptr)
        {
            hasOrigValue = true;

            carla_setenv(key, origValue);

            delete[] origValue;
            origValue = nullptr;
        }

        if (key != nullptr)
        {
            if (! hasOrigValue)
                carla_unsetenv(key);

            delete[] key;
            key = nullptr;
        }
    }

private:
    const char* key;
    const char* origValue;

    CARLA_DECLARE_NON_COPYABLE(CarlaScopedEnvVar)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------
// CarlaScopedLocale class

class CarlaScopedLocale {
#ifdef CARLA_USE_NEWLOCALE
    static constexpr locale_t kNullLocale = (locale_t)nullptr;
#endif

public:
    CarlaScopedLocale() noexcept
#ifdef CARLA_USE_NEWLOCALE
        : newloc(::newlocale(LC_NUMERIC_MASK, "C", kNullLocale)),
          oldloc(newloc != kNullLocale ? ::uselocale(newloc) : kNullLocale) {}
#else
# ifdef CARLA_USE_CONFIGTHREADLOCALE
        : oldthreadloc(_configthreadlocale(_ENABLE_PER_THREAD_LOCALE)),
# else
        :
# endif
          oldloc(carla_strdup_safe(::setlocale(LC_NUMERIC, nullptr)))
    {
        ::setlocale(LC_NUMERIC, "C");
    }
#endif

    ~CarlaScopedLocale() noexcept
    {
#ifdef CARLA_USE_NEWLOCALE
        if (oldloc != kNullLocale)
            ::uselocale(oldloc);
        if (newloc != kNullLocale)
            ::freelocale(newloc);
#else // CARLA_USE_NEWLOCALE
        if (oldloc != nullptr)
        {
            ::setlocale(LC_NUMERIC, oldloc);
            delete[] oldloc;
        }

# ifdef CARLA_USE_CONFIGTHREADLOCALE
        if (oldthreadloc != -1)
            _configthreadlocale(oldthreadloc);
# endif
#endif // CARLA_USE_NEWLOCALE
    }

private:
#ifdef CARLA_USE_NEWLOCALE
    locale_t newloc, oldloc;
#else
# ifdef CARLA_USE_CONFIGTHREADLOCALE
    const int oldthreadloc;
# endif
    const char* const oldloc;
#endif

    CARLA_DECLARE_NON_COPYABLE(CarlaScopedLocale)
    CARLA_PREVENT_HEAP_ALLOCATION
};

//=====================================================================================================================
/**
    This class holds a pointer which is automatically deleted when this object goes
    out of scope.

    Once a pointer has been passed to a CarlaScopedPointer, it will make sure that the pointer
    gets deleted when the CarlaScopedPointer is deleted. Using the CarlaScopedPointer on the stack or
    as member variables is a good way to use RAII to avoid accidentally leaking dynamically
    created objects.

    A CarlaScopedPointer can be used in pretty much the same way that you'd use a normal pointer
    to an object. If you use the assignment operator to assign a different object to a
    CarlaScopedPointer, the old one will be automatically deleted.

    A const CarlaScopedPointer is guaranteed not to lose ownership of its object or change the
    object to which it points during its lifetime. This means that making a copy of a const
    CarlaScopedPointer is impossible, as that would involve the new copy taking ownership from the
    old one.

    If you need to get a pointer out of a CarlaScopedPointer without it being deleted, you
    can use the release() method.

    Something to note is the main difference between this class and the std::auto_ptr class,
    which is that CarlaScopedPointer provides a cast-to-object operator, whereas std::auto_ptr
    requires that you always call get() to retrieve the pointer. The advantages of providing
    the cast is that you don't need to call get(), so can use the CarlaScopedPointer in pretty much
    exactly the same way as a raw pointer. The disadvantage is that the compiler is free to
    use the cast in unexpected and sometimes dangerous ways - in particular, it becomes difficult
    to return a CarlaScopedPointer as the result of a function. To avoid this causing errors,
    CarlaScopedPointer contains an overloaded constructor that should cause a syntax error in these
    circumstances, but it does mean that instead of returning a CarlaScopedPointer from a function,
    you'd need to return a raw pointer (or use a std::auto_ptr instead).
*/
template<class ObjectType>
class CarlaScopedPointer
{
public:
    //=================================================================================================================
    /** Creates a CarlaScopedPointer containing a null pointer. */
    CarlaScopedPointer() noexcept
        : object(nullptr) {}

    /** Creates a CarlaScopedPointer that owns the specified object. */
    CarlaScopedPointer(ObjectType* const objectToTakePossessionOf) noexcept
        : object(objectToTakePossessionOf) {}

    /** Creates a CarlaScopedPointer that takes its pointer from another CarlaScopedPointer.

        Because a pointer can only belong to one CarlaScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.
    */
    CarlaScopedPointer(CarlaScopedPointer& objectToTransferFrom) noexcept
        : object(objectToTransferFrom.object)
    {
        objectToTransferFrom.object = nullptr;
    }

    /** Destructor.
        This will delete the object that this CarlaScopedPointer currently refers to.
    */
    ~CarlaScopedPointer()
    {
        delete object;
    }

    /** Changes this CarlaScopedPointer to point to a new object.

        Because a pointer can only belong to one CarlaScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.

        If this CarlaScopedPointer already points to an object, that object
        will first be deleted.
    */
    CarlaScopedPointer& operator=(CarlaScopedPointer& objectToTransferFrom)
    {
        if (this != objectToTransferFrom.getAddress())
        {
            // Two CarlaScopedPointers should never be able to refer to the same object - if
            // this happens, you must have done something dodgy!
            CARLA_SAFE_ASSERT_RETURN(object == nullptr || object != objectToTransferFrom.object, *this);

            ObjectType* const oldObject = object;
            object = objectToTransferFrom.object;
            objectToTransferFrom.object = nullptr;
            delete oldObject;
        }

        return *this;
    }

    /** Changes this CarlaScopedPointer to point to a new object.

        If this CarlaScopedPointer already points to an object, that object
        will first be deleted.

        The pointer that you pass in may be a nullptr.
    */
    CarlaScopedPointer& operator=(ObjectType* const newObjectToTakePossessionOf)
    {
        if (object != newObjectToTakePossessionOf)
        {
            ObjectType* const oldObject = object;
            object = newObjectToTakePossessionOf;
            delete oldObject;
        }

        return *this;
    }

    //=================================================================================================================
    /** Returns the object that this CarlaScopedPointer refers to. */
    operator ObjectType*() const noexcept   { return object; }

    /** Returns the object that this CarlaScopedPointer refers to. */
    ObjectType* get() const noexcept        { return object; }

    /** Returns the object that this CarlaScopedPointer refers to. */
    ObjectType& operator*() const noexcept  { return *object; }

    /** Lets you access methods and properties of the object that this CarlaScopedPointer refers to. */
    ObjectType* operator->() const noexcept { return object; }

    //=================================================================================================================
    /** Removes the current object from this CarlaScopedPointer without deleting it.
        This will return the current object, and set the CarlaScopedPointer to a null pointer.
    */
    ObjectType* release() noexcept          { ObjectType* const o = object; object = nullptr; return o; }

    //=================================================================================================================
    /** Swaps this object with that of another CarlaScopedPointer.
        The two objects simply exchange their pointers.
    */
    void swapWith(CarlaScopedPointer<ObjectType>& other) noexcept
    {
        // Two CarlaScopedPointers should never be able to refer to the same object - if
        // this happens, you must have done something dodgy!
        CARLA_SAFE_ASSERT_RETURN(object != other.object || this == other.getAddress() || object == nullptr,);

        std::swap(object, other.object);
    }

private:
    //=================================================================================================================
    ObjectType* object;

    // (Required as an alternative to the overloaded & operator).
    const CarlaScopedPointer* getAddress() const noexcept { return this; }

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaScopedPointer(const CarlaScopedPointer&) = delete;
    CarlaScopedPointer& operator=(const CarlaScopedPointer&) = delete;
#else
    CarlaScopedPointer(const CarlaScopedPointer&);
    CarlaScopedPointer& operator=(const CarlaScopedPointer&);
#endif
};

//=====================================================================================================================
/** Compares a CarlaScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator==(const CarlaScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) == pointer2;
}

/** Compares a CarlaScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator!=(const CarlaScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) != pointer2;
}

//=====================================================================================================================
/**
    Helper class providing an RAII-based mechanism for temporarily setting and
    then re-setting a value.

    E.g. @code
    int x = 1;

    {
        CarlaScopedValueSetter setter (x, 2);

        // x is now 2
    }

    // x is now 1 again

    {
        CarlaScopedValueSetter setter (x, 3, 4);

        // x is now 3
    }

    // x is now 4
    @endcode
*/
template <typename ValueType>
class CarlaScopedValueSetter
{
public:
    /** Creates a CarlaScopedValueSetter that will immediately change the specified value to the
        given new value, and will then reset it to its original value when this object is deleted.
        Must be used only for 'noexcept' compatible types.
    */
    CarlaScopedValueSetter(ValueType& valueToSet, ValueType newValue) noexcept
        : value(valueToSet),
          originalValue(valueToSet)
    {
        valueToSet = newValue;
    }

    /** Creates a CarlaScopedValueSetter that will immediately change the specified value to the
        given new value, and will then reset it to be valueWhenDeleted when this object is deleted.
    */
    CarlaScopedValueSetter(ValueType& valueToSet, ValueType newValue, ValueType valueWhenDeleted) noexcept
        : value(valueToSet),
          originalValue(valueWhenDeleted)
    {
        valueToSet = newValue;
    }

    ~CarlaScopedValueSetter() noexcept
    {
        value = originalValue;
    }

private:
    //=================================================================================================================
    ValueType& value;
    const ValueType originalValue;

    CARLA_DECLARE_NON_COPYABLE(CarlaScopedValueSetter)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

#endif // CARLA_SCOPE_UTILS_HPP_INCLUDED
