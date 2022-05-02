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

#ifndef USING_JUCE
# error this file is not meant to be built if not using juce!
#endif

#include "carla_juce.h"

#include "CarlaUtils.hpp"

#include "juce_events/juce_events.h"
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
# include "juce_gui_basics/juce_gui_basics.h"
#endif

namespace juce {
bool dispatchNextMessageOnSystemQueue(bool returnIfNoPendingMessages);
}

namespace CarlaJUCE {

void initialiseJuce_GUI()
{
    juce::initialiseJuce_GUI();
#if !(defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
#endif
}

void idleJuce_GUI()
{
#if !(defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
    juce::dispatchNextMessageOnSystemQueue(true);
#endif
}

void shutdownJuce_GUI()
{
    juce::shutdownJuce_GUI();
}

const char* getVersion()
{
    static const juce::String version(juce::SystemStats::getJUCEVersion());
    return version.toRawUTF8();
}

static int numScopedInitInstances = 0;

ScopedJuceInitialiser_GUI::ScopedJuceInitialiser_GUI()
{
    if (numScopedInitInstances++ == 0)
        initialiseJuce_GUI();
}

ScopedJuceInitialiser_GUI::~ScopedJuceInitialiser_GUI()
{
    if (--numScopedInitInstances == 0)
        shutdownJuce_GUI();
}

#ifdef USE_REFCOUNTER_JUCE_MESSAGE_MANAGER
ReferenceCountedJuceMessageMessager::ReferenceCountedJuceMessageMessager()
{
    CARLA_SAFE_ASSERT(numScopedInitInstances == 0);
}

ReferenceCountedJuceMessageMessager::~ReferenceCountedJuceMessageMessager()
{
    CARLA_SAFE_ASSERT(numScopedInitInstances == 0);
}

void ReferenceCountedJuceMessageMessager::incRef() const
{
    if (numScopedInitInstances++ == 0)
    {
        juce::initialiseJuce_GUI();
        juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
    }
}

void ReferenceCountedJuceMessageMessager::decRef() const
{
    if (--numScopedInitInstances == 0)
    {
        juce::shutdownJuce_GUI();
    }
}

void setMessageManagerForThisThread()
{
    juce::MessageManager* const msgMgr = juce::MessageManager::getInstanceWithoutCreating();
    CARLA_SAFE_ASSERT_RETURN(msgMgr != nullptr,);

    if (! msgMgr->isThisTheMessageThread())
    {
        try {
            msgMgr->setCurrentThreadAsMessageThread();
        } CARLA_SAFE_EXCEPTION_RETURN("setCurrentThreadAsMessageThread",);
    }
}

void dispatchMessageManagerMessages()
{
    const juce::MessageManagerLock mml;
    juce::dispatchNextMessageOnSystemQueue(true);
}
#endif

#ifdef USE_STANDALONE_JUCE_APPLICATION
class CarlaJuceApp : public juce::JUCEApplication,
                     private juce::Timer
{
public:
    CarlaJuceApp()  {}
    ~CarlaJuceApp() {}

    void initialise(const juce::String&) override
    {
        startTimer(8);
    }

    void shutdown() override
    {
        gCloseNow = true;
        stopTimer();
    }

    const juce::String getApplicationName() override
    {
        return "CarlaPlugin";
    }

    const juce::String getApplicationVersion() override
    {
        return CARLA_VERSION_STRING;
    }

    void timerCallback() override
    {
        gIdle();

        if (gCloseNow)
        {
            quit();
            gCloseNow = false;
        }
    }
};

void setupAndUseMainApplication()
{
#ifndef CARLA_OS_WIN
    static const int argc = 0;
    static const char* argv[] = {};
#endif
    static juce::JUCEApplicationBase* juce_CreateApplication() { return new CarlaJuceApp(); }
    juce::JUCEApplicationBase::createInstance = &juce_CreateApplication;
    juce::JUCEApplicationBase::main(JUCE_MAIN_FUNCTION_ARGS);
}
#endif

} // namespace CarlaJUCE
