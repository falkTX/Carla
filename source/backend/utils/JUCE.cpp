/*
 * Carla Plugin Host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#ifdef USING_JUCE
# include "carla_juce/carla_juce.h"
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_juce_init()
{
#ifdef USING_JUCE
    CarlaJUCE::initialiseJuce_GUI();
#endif
}

void carla_juce_idle()
{
#ifdef USING_JUCE
    CarlaJUCE::idleJuce_GUI();
#endif
}

void carla_juce_cleanup()
{
#ifdef USING_JUCE
    CarlaJUCE::shutdownJuce_GUI();
#endif
}

// -------------------------------------------------------------------------------------------------------------------
