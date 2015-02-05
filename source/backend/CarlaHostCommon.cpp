/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaHost.h"
#include "CarlaUtils.hpp"

#include "juce_core.h"

namespace CB = CarlaBackend;

// -------------------------------------------------------------------------------------------------------------------
// Always return a valid string ptr

static const char* const gNullCharPtr = "";

#ifdef CARLA_COMMON_NEED_CHECKSTRINGPTR
static void checkStringPtr(const char*& charPtr) noexcept
{
    if (charPtr == nullptr)
        charPtr = gNullCharPtr;
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// Constructors

_CarlaPluginInfo::_CarlaPluginInfo() noexcept
    : type(CB::PLUGIN_NONE),
      category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0),
      optionsAvailable(0x0),
      optionsEnabled(0x0),
      filename(gNullCharPtr),
      name(gNullCharPtr),
      label(gNullCharPtr),
      maker(gNullCharPtr),
      copyright(gNullCharPtr),
      iconName(gNullCharPtr),
      uniqueId(0) {}

_CarlaPluginInfo::~_CarlaPluginInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
    if (maker != gNullCharPtr)
        delete[] maker;
    if (copyright != gNullCharPtr)
        delete[] copyright;
}

_CarlaParameterInfo::_CarlaParameterInfo() noexcept
    : name(gNullCharPtr),
      symbol(gNullCharPtr),
      unit(gNullCharPtr),
      scalePointCount(0) {}

_CarlaParameterInfo::~_CarlaParameterInfo() noexcept
{
    if (name != gNullCharPtr)
        delete[] name;
    if (symbol != gNullCharPtr)
        delete[] symbol;
    if (unit != gNullCharPtr)
        delete[] unit;
}

_CarlaScalePointInfo::_CarlaScalePointInfo() noexcept
    : value(0.0f),
      label(gNullCharPtr) {}

_CarlaScalePointInfo::~_CarlaScalePointInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
}

_CarlaTransportInfo::_CarlaTransportInfo() noexcept
    : playing(false),
      frame(0),
      bar(0),
      beat(0),
      tick(0),
      bpm(0.0) {}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_library_filename()
{
    carla_debug("carla_get_library_filename()");

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using namespace juce;
        ret = File(File::getSpecialLocation(File::currentExecutableFile)).getFullPathName().toRawUTF8();
    }

    return ret;
}

const char* carla_get_library_folder()
{
    carla_debug("carla_get_library_folder()");

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using namespace juce;
        ret = File(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory()).getFullPathName().toRawUTF8();
    }

    return ret;
}

// -------------------------------------------------------------------------------------------------------------------
