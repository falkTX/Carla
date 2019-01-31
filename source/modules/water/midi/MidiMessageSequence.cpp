/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2019 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#include "MidiMessageSequence.h"

namespace water {

MidiMessageSequence::MidiMessageSequence()
{
}

MidiMessageSequence::MidiMessageSequence (const MidiMessageSequence& other)
{
    addSequence(other, 0.0);
    updateMatchedPairs();
}

MidiMessageSequence& MidiMessageSequence::operator= (const MidiMessageSequence& other)
{
    MidiMessageSequence otherCopy (other);
    swapWith (otherCopy);
    return *this;
}

void MidiMessageSequence::swapWith (MidiMessageSequence& other) noexcept
{
    list.swapWith (other.list);
}

MidiMessageSequence::~MidiMessageSequence()
{
}

void MidiMessageSequence::clear()
{
    list.clear();
}

int MidiMessageSequence::getNumEvents() const noexcept
{
    return list.size();
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::getEventPointer (const int index) const noexcept
{
    return list [index];
}

double MidiMessageSequence::getTimeOfMatchingKeyUp (const int index) const noexcept
{
    if (const MidiEventHolder* const meh = list [index])
        if (meh->noteOffObject != nullptr)
            return meh->noteOffObject->message.getTimeStamp();

    return 0.0;
}

int MidiMessageSequence::getIndexOfMatchingKeyUp (const int index) const noexcept
{
    if (const MidiEventHolder* const meh = list [index])
        return list.indexOf (meh->noteOffObject);

    return -1;
}

int MidiMessageSequence::getIndexOf (const MidiEventHolder* const event) const noexcept
{
    return list.indexOf (event);
}

int MidiMessageSequence::getNextIndexAtTime (const double timeStamp) const noexcept
{
    const int numEvents = list.size();

    int i;
    for (i = 0; i < numEvents; ++i)
        if (list.getUnchecked(i)->message.getTimeStamp() >= timeStamp)
            break;

    return i;
}

//==============================================================================
double MidiMessageSequence::getStartTime() const noexcept
{
    return getEventTime (0);
}

double MidiMessageSequence::getEndTime() const noexcept
{
    return getEventTime (list.size() - 1);
}

double MidiMessageSequence::getEventTime (const int index) const noexcept
{
    if (const MidiEventHolder* const meh = list [index])
        return meh->message.getTimeStamp();

    return 0.0;
}

//==============================================================================
MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (const MidiMessage& newMessage,
                                                                     double timeAdjustment)
{
    MidiEventHolder* const newOne = new MidiEventHolder (newMessage);

    timeAdjustment += newMessage.getTimeStamp();
    newOne->message.setTimeStamp (timeAdjustment);

    int i;
    for (i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.getTimeStamp() <= timeAdjustment)
            break;

    list.insert (i + 1, newOne);
    return newOne;
}

void MidiMessageSequence::deleteEvent (const int index,
                                       const bool deleteMatchingNoteUp)
{
    if (isPositiveAndBelow (index, static_cast<int>(list.size())))
    {
        if (deleteMatchingNoteUp)
            deleteEvent (getIndexOfMatchingKeyUp (index), false);

        list.remove (index);
    }
}

struct MidiMessageSequenceSorter
{
    static int compareElements (const MidiMessageSequence::MidiEventHolder* const first,
                                const MidiMessageSequence::MidiEventHolder* const second) noexcept
    {
        const double diff = first->message.getTimeStamp() - second->message.getTimeStamp();
        return (diff > 0) - (diff < 0);
    }
};

void MidiMessageSequence::addSequence (const MidiMessageSequence& other, double timeAdjustment)
{
    for (int i = 0; i < static_cast<int>(other.list.size()); ++i)
    {
        const MidiMessage& m = other.list.getUnchecked(i)->message;

        MidiEventHolder* const newOne = new MidiEventHolder (m);
        newOne->message.addToTimeStamp (timeAdjustment);
        list.add (newOne);
    }

    sort();
}

void MidiMessageSequence::addSequence (const MidiMessageSequence& other,
                                       double timeAdjustment,
                                       double firstAllowableTime,
                                       double endOfAllowableDestTimes)
{
    for (int i = 0; i < static_cast<int>(other.list.size()); ++i)
    {
        const MidiMessage& m = other.list.getUnchecked(i)->message;
        const double t = m.getTimeStamp() + timeAdjustment;

        if (t >= firstAllowableTime && t < endOfAllowableDestTimes)
        {
            MidiEventHolder* const newOne = new MidiEventHolder (m);
            newOne->message.setTimeStamp (t);

            list.add (newOne);
        }
    }

    sort();
}

//==============================================================================
void MidiMessageSequence::sort() noexcept
{
    MidiMessageSequenceSorter sorter;
    list.sort (sorter, true);
}

void MidiMessageSequence::updateMatchedPairs() noexcept
{
    for (int i = 0; i < static_cast<int>(list.size()); ++i)
    {
        MidiEventHolder* const meh = list.getUnchecked(i);
        const MidiMessage& m1 = meh->message;

        if (m1.isNoteOn())
        {
            meh->noteOffObject = nullptr;
            const int note = m1.getNoteNumber();
            const int chan = m1.getChannel();
            const int len = list.size();

            for (int j = i + 1; j < len; ++j)
            {
                const MidiMessage& m = list.getUnchecked(j)->message;

                if (m.getNoteNumber() == note && m.getChannel() == chan)
                {
                    if (m.isNoteOff())
                    {
                        meh->noteOffObject = list[j];
                        break;
                    }
                    else if (m.isNoteOn())
                    {
                        MidiEventHolder* const newEvent = new MidiEventHolder (MidiMessage::noteOff (chan, note));
                        list.insert (j, newEvent);
                        newEvent->message.setTimeStamp (m.getTimeStamp());
                        meh->noteOffObject = newEvent;
                        break;
                    }
                }
            }
        }
    }
}

void MidiMessageSequence::addTimeToMessages (const double delta) noexcept
{
    for (int i = static_cast<int>(list.size()); --i >= 0;)
    {
        MidiMessage& mm = list.getUnchecked(i)->message;
        mm.setTimeStamp (mm.getTimeStamp() + delta);
    }
}

//==============================================================================
void MidiMessageSequence::extractMidiChannelMessages (const int channelNumberToExtract,
                                                      MidiMessageSequence& destSequence,
                                                      const bool alsoIncludeMetaEvents) const
{
    for (int i = 0; i < static_cast<int>(list.size()); ++i)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isForChannel (channelNumberToExtract) || (alsoIncludeMetaEvents && mm.isMetaEvent()))
            destSequence.addEvent (mm);
    }
}

void MidiMessageSequence::extractSysExMessages (MidiMessageSequence& destSequence) const
{
    for (int i = 0; i < static_cast<int>(list.size()); ++i)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isSysEx())
            destSequence.addEvent (mm);
    }
}

void MidiMessageSequence::deleteMidiChannelMessages (const int channelNumberToRemove)
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.isForChannel (channelNumberToRemove))
            list.remove(i);
}

void MidiMessageSequence::deleteSysExMessages()
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.isSysEx())
            list.remove(i);
}

//==============================================================================
void MidiMessageSequence::createControllerUpdatesForTime (const int channelNumber, const double time, Array<MidiMessage>& dest)
{
    bool doneProg = false;
    bool donePitchWheel = false;
    bool doneControllers[128] = { 0 };

    for (int i = list.size(); --i >= 0;)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isForChannel (channelNumber) && mm.getTimeStamp() <= time)
        {
            if (mm.isProgramChange() && ! doneProg)
            {
                doneProg = true;
                dest.add (MidiMessage (mm, 0.0));
            }
            else if (mm.isPitchWheel() && ! donePitchWheel)
            {
                donePitchWheel = true;
                dest.add (MidiMessage (mm, 0.0));
            }
            else if (mm.isController())
            {
                const int controllerNumber = mm.getControllerNumber();
                wassert (isPositiveAndBelow (controllerNumber, 128));

                if (! doneControllers[controllerNumber])
                {
                    doneControllers[controllerNumber] = true;
                    dest.add (MidiMessage (mm, 0.0));
                }
            }
        }
    }
}


//==============================================================================
MidiMessageSequence::MidiEventHolder::MidiEventHolder (const MidiMessage& mm)
   : message (mm), noteOffObject (nullptr)
{
}

MidiMessageSequence::MidiEventHolder::~MidiEventHolder()
{
}

}
