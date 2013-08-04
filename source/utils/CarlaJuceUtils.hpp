/*
 * Carla misc utils using Juce resources
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

#ifndef CARLA_JUCE_UTILS_HPP_INCLUDED
#define CARLA_JUCE_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include "JuceHeader.h"

#define CARLA_PREVENT_HEAP_ALLOCATION   \
private:                                \
    static void* operator new(size_t);  \
    static void operator delete(void*);

#define CARLA_DECLARE_NON_COPYABLE(ClassName) \
private:                                      \
    ClassName(const ClassName&);              \
    ClassName& operator=(const ClassName&);

#define CARLA_LEAK_DETECTOR(ClassName) \
    JUCE_LEAK_DETECTOR(ClassName)

#define CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName) \
    CARLA_DECLARE_NON_COPYABLE(ClassName)                        \
    CARLA_LEAK_DETECTOR(ClassName)

#endif // CARLA_JUCE_UTILS_HPP_INCLUDED
