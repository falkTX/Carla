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
