/*
 * Carla Scoped Locale
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SCOPED_LOCALE_HPP_INCLUDED
#define CARLA_SCOPED_LOCALE_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <clocale>

// -----------------------------------------------------------------------
// CarlaScopedLocale class

class CarlaScopedLocale {
public:
    CarlaScopedLocale() noexcept
        : fLocale(carla_strdup_safe(::setlocale(LC_NUMERIC, nullptr)))
    {
        ::setlocale(LC_NUMERIC, "C");
    }

    ~CarlaScopedLocale() noexcept
    {
        if (fLocale != nullptr)
        {
            ::setlocale(LC_NUMERIC, fLocale);
            delete[] fLocale;
        }
    }

private:
    const char* const fLocale;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaScopedLocale)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

#endif // CARLA_SCOPED_LOCALE_HPP_INCLUDED
