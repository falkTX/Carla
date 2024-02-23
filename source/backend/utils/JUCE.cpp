// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
