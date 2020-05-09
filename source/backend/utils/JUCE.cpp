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

#include "CarlaUtils.h"
#include "CarlaUtils.hpp"

#ifdef USING_JUCE

// -------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
# pragma GCC diagnostic ignored "-Wundef"
#endif

#include "AppConfig.h"
#include "juce_events/juce_events.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_juce_init()
{
    juce::initialiseJuce_GUI();
#if !(defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
#endif
}

void carla_juce_idle()
{
#if !(defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
    const juce::MessageManager* const msgMgr(juce::MessageManager::getInstanceWithoutCreating());
    CARLA_SAFE_ASSERT_RETURN(msgMgr != nullptr,);

    for (; msgMgr->dispatchNextMessageOnSystemQueue(true);) {}
#endif
}

void carla_juce_cleanup()
{
    juce::shutdownJuce_GUI();
}

// -------------------------------------------------------------------------------------------------------------------

#else // USING_JUCE

// -------------------------------------------------------------------------------------------------------------------

void carla_juce_init() {}
void carla_juce_idle() {}
void carla_juce_cleanup() {}

// -------------------------------------------------------------------------------------------------------------------

#endif // USING_JUCE
