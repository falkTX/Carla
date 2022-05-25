/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2022 Filipe Coelho <falktx@falktx.com>

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

//==============================================================================
MidiMessageSequence::MidiEventHolder::MidiEventHolder (const MidiMessage& mm)
   : message (mm), noteOffObject (nullptr)
{
}

MidiMessageSequence::MidiEventHolder::~MidiEventHolder()
{
}

}
