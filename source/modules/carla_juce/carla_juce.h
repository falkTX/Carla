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

#ifndef CARLA_JUCE_HPP_INCLUDED
#define CARLA_JUCE_HPP_INCLUDED

#include "AppConfig.h"
#include "CarlaDefines.h"

#ifdef USING_JUCE 
# if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
#  define USE_STANDALONE_JUCE_APPLICATION
# else
#  define USE_REFCOUNTER_JUCE_MESSAGE_MANAGER
# endif
#endif

namespace juce {
class MessageManager;
}

namespace CarlaJUCE {

void initialiseJuce_GUI();
void idleJuce_GUI();
void shutdownJuce_GUI();
const char* getVersion();

struct ScopedJuceInitialiser_GUI {
    ScopedJuceInitialiser_GUI();
    ~ScopedJuceInitialiser_GUI();
};

#ifdef USE_REFCOUNTER_JUCE_MESSAGE_MANAGER
struct ReferenceCountedJuceMessageMessager {
    ReferenceCountedJuceMessageMessager();
    ~ReferenceCountedJuceMessageMessager();
    void incRef() const;
    void decRef() const;
};

void setMessageManagerForThisThread();
void dispatchMessageManagerMessages();
#endif

#ifdef USE_STANDALONE_JUCE_APPLICATION
void setupAndUseMainApplication();
#endif

} // namespace CarlaJUCE

#endif // CARLA_JUCE_HPP_INCLUDED
