/*
 * Carla Utility Tests
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

#ifdef NDEBUG
# error Build this file with debug ON please
#endif

#include "CarlaBackendUtils.hpp"

using namespace CarlaBackend;

// -----------------------------------------------------------------------

#define HAVE_JUCE
#define HAVE_JUCE_LATER
#undef CARLA_BACKEND_START_NAMESPACE
#define CARLA_BACKEND_START_NAMESPACE namespace state_juce {
#include "CarlaStateUtils.cpp"

// -----------------------------------------------------------------------

#undef CARLA_STATE_UTILS_HPP_INCLUDED

// -----------------------------------------------------------------------

#undef HAVE_JUCE
#undef HAVE_JUCE_LATER
#undef CARLA_BACKEND_START_NAMESPACE
#define CARLA_BACKEND_START_NAMESPACE namespace state_qt {
#include "CarlaStateUtils.cpp"

// -----------------------------------------------------------------------
// main

int main()
{
    state_juce::StateSave jsave;
    jsave.type = carla_strdup("NONE");
    carla_stdout(jsave.toString().toRawUTF8());

    state_qt::StateSave qsave;
    qsave.type = carla_strdup("NONE");
    carla_stdout(qsave.toString().toUtf8().constData());

    return 0;
}

// -----------------------------------------------------------------------
