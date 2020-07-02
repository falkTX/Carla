/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

// NB: this must come first, *before* the header-guard.
#ifdef JUCE_VSTINTERFACE_H_INCLUDED

namespace juce
{

//==============================================================================
/** Holds a set of VSTMidiEvent objects and makes it easy to add
    events to the list.

    This is used by both the VST hosting code and the plugin wrapper.

    @tags{Audio}
*/
class VSTMidiEventList
{
public:
    //==============================================================================
    VSTMidiEventList()
        : numEventsUsed (0), numEventsAllocated (0)
    {
    }

    ~VSTMidiEventList()
    {
        freeEvents();
    }

    //==============================================================================
    void clear()
    {
        numEventsUsed = 0;

        if (events != nullptr)
            events->numberOfEvents = 0;
    }

    void addEvent (const void* const midiData, int numBytes, int frameOffset)
    {
        ensureSize (numEventsUsed + 1);

        void* const ptr = (Vst2::VstMidiEvent*) (events->events [numEventsUsed]);
        auto* const e = (Vst2::VstMidiEvent*) ptr;
        events->numberOfEvents = ++numEventsUsed;

        if (numBytes <= 4)
        {
            if (e->type == Vst2::vstSysExEventType)
            {
                delete[] (((Vst2::VstSysExEvent*) ptr)->sysExDump);
                e->type = Vst2::vstMidiEventType;
                e->size = sizeof (Vst2::VstMidiEvent);
                e->noteSampleLength = 0;
                e->noteSampleOffset = 0;
                e->tuning = 0;
                e->noteVelocityOff = 0;
            }

            e->sampleOffset = frameOffset;
            memcpy (e->midiData, midiData, (size_t) numBytes);
        }
        else
        {
            auto* const se = (Vst2::VstSysExEvent*) ptr;

            if (se->type == Vst2::vstSysExEventType)
                delete[] se->sysExDump;

            se->sysExDump = new char [(size_t) numBytes];
            memcpy (se->sysExDump, midiData, (size_t) numBytes);

            se->type = Vst2::vstSysExEventType;
            se->size = sizeof (Vst2::VstSysExEvent);
            se->offsetSamples = frameOffset;
            se->flags = 0;
            se->sysExDumpSize = numBytes;
            se->future1 = 0;
            se->future2 = 0;
        }
    }

    //==============================================================================
    // Handy method to pull the events out of an event buffer supplied by the host
    // or plugin.
    static void addEventsToMidiBuffer (const Vst2::VstEventBlock* events, MidiBuffer& dest)
    {
        for (int i = 0; i < events->numberOfEvents; ++i)
        {
            const Vst2::VstEvent* const e = events->events[i];

            if (e != nullptr)
            {
                const void* const ptr = events->events[i];

                if (e->type == Vst2::vstMidiEventType)
                {
                    dest.addEvent ((const juce::uint8*) ((const Vst2::VstMidiEvent*) ptr)->midiData,
                                   4, e->sampleOffset);
                }
                else if (e->type == Vst2::vstSysExEventType)
                {
                    const auto* se = (const Vst2::VstSysExEvent*) ptr;
                    dest.addEvent ((const juce::uint8*) se->sysExDump,
                                   (int) se->sysExDumpSize,
                                   e->sampleOffset);
                }
            }
        }
    }

    //==============================================================================
    void ensureSize (int numEventsNeeded)
    {
        if (numEventsNeeded > numEventsAllocated)
        {
            numEventsNeeded = (numEventsNeeded + 32) & ~31;

            const size_t size = 20 + (size_t) numEventsNeeded * sizeof (Vst2::VstEvent*);

            if (events == nullptr)
                events.calloc (size, 1);
            else
                events.realloc (size, 1);

            for (int i = numEventsAllocated; i < numEventsNeeded; ++i)
                events->events[i] = allocateVSTEvent();

            numEventsAllocated = numEventsNeeded;
        }
    }

    void freeEvents()
    {
        if (events != nullptr)
        {
            for (int i = numEventsAllocated; --i >= 0;)
                freeVSTEvent (events->events[i]);

            events.free();
            numEventsUsed = 0;
            numEventsAllocated = 0;
        }
    }

    //==============================================================================
    HeapBlock<Vst2::VstEventBlock> events;

private:
    int numEventsUsed, numEventsAllocated;

    static Vst2::VstEvent* allocateVSTEvent()
    {
        auto e = (Vst2::VstEvent*) std::calloc (1, sizeof (Vst2::VstMidiEvent) > sizeof (Vst2::VstSysExEvent) ? sizeof (Vst2::VstMidiEvent)
                                                                                            : sizeof (Vst2::VstSysExEvent));
        e->type = Vst2::vstMidiEventType;
        e->size = sizeof (Vst2::VstMidiEvent);
        return e;
    }

    static void freeVSTEvent (Vst2::VstEvent* e)
    {
        if (e->type == Vst2::vstSysExEventType)
        {
            delete[] (reinterpret_cast<Vst2::VstSysExEvent*> (e)->sysExDump);
        }

        std::free (e);
    }
};

} // namespace juce

#endif // JUCE_VSTINTERFACE_H_INCLUDED
