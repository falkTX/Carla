/*
 * Fake juce event thread needed for juce_audio_processors
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaJuceUtils.hpp"

// -------------------------------------------------------------------------------------------------------------------

#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))

#undef KeyPress
#include "juce_audio_processors.h"
#include "juce_events.h"

namespace juce {

#include "juce_events/broadcasters/juce_ActionBroadcaster.cpp"
#include "juce_events/broadcasters/juce_AsyncUpdater.cpp"
#include "juce_events/messages/juce_DeletedAtShutdown.cpp"
#include "juce_events/messages/juce_MessageManager.cpp"
#include "juce_audio_processors/processors/juce_AudioProcessor.cpp"
#include "juce_audio_processors/processors/juce_AudioProcessorGraph.cpp"

class JuceEventsThread : public Thread
{
public:
    JuceEventsThread()
        : Thread("JuceEventsThread"),
          fInitializing(false),
          fLock(),
          fQueue() {}

    ~JuceEventsThread()
    {
        signalThreadShouldExit();
        stopThread(2000);

        const ScopedLock sl(fLock);
        fQueue.clear();
    }

    bool postMessage(MessageManager::MessageBase* const msg)
    {
        const ScopedLock sl(fLock);
        fQueue.add(msg);
        return true;
    }

    bool isInitializing() const noexcept
    {
        return fInitializing;
    }

protected:
    void run() override
    {
        /*
         * We need to know when we're initializing because MessageManager::setCurrentThreadAsMessageThread()
         * calls doPlatformSpecificInitialisation/Shutdown, in which we started this thread previously.
         * To avoid a deadlock we do not call start/stopThread if still initializing.
         */
        fInitializing = true;

        if (MessageManager* const msgMgr = MessageManager::getInstance())
            msgMgr->setCurrentThreadAsMessageThread();

        fInitializing = false;

        for (; ! threadShouldExit();)
        {
            // dispatch messages until no more present, then sleep
            for (; dispatchNextInternalMessage();) {}

            sleep(25);
        }
    }

private:
    volatile bool fInitializing;
    CriticalSection fLock;
    ReferenceCountedArray<MessageManager::MessageBase> fQueue;

    MessageManager::MessageBase::Ptr popNextMessage()
    {
        const ScopedLock sl(fLock);
        return fQueue.removeAndReturn(0);
    }

    bool dispatchNextInternalMessage()
    {
        if (const MessageManager::MessageBase::Ptr msg = popNextMessage())
        {
            JUCE_TRY
            {
                msg->messageCallback();
                return true;
            }
            JUCE_CATCH_EXCEPTION
        }

        return false;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceEventsThread)
};

static JuceEventsThread& getJuceEventsThreadInstance()
{
    static JuceEventsThread sJuceEventsThread;
    return sJuceEventsThread;
}

JUCEApplicationBase::CreateInstanceFunction JUCEApplicationBase::createInstance = nullptr;

void MessageManager::doPlatformSpecificInitialisation()
{
    JuceEventsThread& juceEventsThread(getJuceEventsThreadInstance());

    if (! juceEventsThread.isInitializing())
        juceEventsThread.startThread();
}

void MessageManager::doPlatformSpecificShutdown()
{
    JuceEventsThread& juceEventsThread(getJuceEventsThreadInstance());

    if (! juceEventsThread.isInitializing())
        juceEventsThread.stopThread(-1);
}

bool MessageManager::postMessageToSystemQueue(MessageManager::MessageBase* const message)
{
    JuceEventsThread& juceEventsThread(getJuceEventsThreadInstance());
    return juceEventsThread.postMessage(message);
}

bool MessageManager::dispatchNextMessageOnSystemQueue(bool)
{
    carla_stderr2("MessageManager::dispatchNextMessageOnSystemQueue() unsupported");
    return false;
}

} // namespace juce

#endif // ! CARLA_OS_MAC || CARLA_OS_WIN
