/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

MidiMessageSequence::MidiEventHolder::MidiEventHolder (const MidiMessage& mm) : message (mm) {}
MidiMessageSequence::MidiEventHolder::MidiEventHolder (MidiMessage&& mm) : message (static_cast<MidiMessage&&> (mm)) {}
MidiMessageSequence::MidiEventHolder::~MidiEventHolder() {}

//==============================================================================
MidiMessageSequence::MidiMessageSequence()
{
}

MidiMessageSequence::MidiMessageSequence (const MidiMessageSequence& other)
{
    list.addCopiesOf (other.list);
    updateMatchedPairs();
}

MidiMessageSequence& MidiMessageSequence::operator= (const MidiMessageSequence& other)
{
    MidiMessageSequence otherCopy (other);
    swapWith (otherCopy);
    return *this;
}

MidiMessageSequence::MidiMessageSequence (MidiMessageSequence&& other) noexcept
    : list (static_cast<OwnedArray<MidiEventHolder>&&> (other.list))
{}

MidiMessageSequence& MidiMessageSequence::operator= (MidiMessageSequence&& other) noexcept
{
    list = static_cast<OwnedArray<MidiEventHolder>&&> (other.list);
    return *this;
}

MidiMessageSequence::~MidiMessageSequence()
{
}

void MidiMessageSequence::swapWith (MidiMessageSequence& other) noexcept
{
    list.swapWith (other.list);
}

void MidiMessageSequence::clear()
{
    list.clear();
}

int MidiMessageSequence::getNumEvents() const noexcept
{
    return list.size();
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::getEventPointer (int index) const noexcept
{
    return list[index];
}

MidiMessageSequence::MidiEventHolder** MidiMessageSequence::begin() const noexcept     { return list.begin(); }
MidiMessageSequence::MidiEventHolder** MidiMessageSequence::end() const noexcept       { return list.end(); }

double MidiMessageSequence::getTimeOfMatchingKeyUp (int index) const noexcept
{
    if (auto* meh = list[index])
        if (meh->noteOffObject != nullptr)
            return meh->noteOffObject->message.getTimeStamp();

    return 0.0;
}

int MidiMessageSequence::getIndexOfMatchingKeyUp (int index) const noexcept
{
    if (auto* meh = list [index])
        return list.indexOf (meh->noteOffObject);

    return -1;
}

int MidiMessageSequence::getIndexOf (const MidiEventHolder* event) const noexcept
{
    return list.indexOf (event);
}

int MidiMessageSequence::getNextIndexAtTime (double timeStamp) const noexcept
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
    if (auto* meh = list [index])
        return meh->message.getTimeStamp();

    return 0.0;
}

//==============================================================================
MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (MidiEventHolder* newEvent, double timeAdjustment)
{
    newEvent->message.addToTimeStamp (timeAdjustment);
    auto time = newEvent->message.getTimeStamp();

    int i;
    for (i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.getTimeStamp() <= time)
            break;

    list.insert (i + 1, newEvent);
    return newEvent;
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (const MidiMessage& newMessage, double timeAdjustment)
{
    return addEvent (new MidiEventHolder (newMessage), timeAdjustment);
}

MidiMessageSequence::MidiEventHolder* MidiMessageSequence::addEvent (MidiMessage&& newMessage, double timeAdjustment)
{
    return addEvent (new MidiEventHolder (static_cast<MidiMessage&&> (newMessage)), timeAdjustment);
}

void MidiMessageSequence::deleteEvent (int index, bool deleteMatchingNoteUp)
{
    if (isPositiveAndBelow (index, list.size()))
    {
        if (deleteMatchingNoteUp)
            deleteEvent (getIndexOfMatchingKeyUp (index), false);

        list.remove (index);
    }
}

void MidiMessageSequence::addSequence (const MidiMessageSequence& other, double timeAdjustment)
{
    for (auto* m : other)
    {
        auto newOne = new MidiEventHolder (m->message);
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
    for (auto* m : other)
    {
        auto t = m->message.getTimeStamp() + timeAdjustment;

        if (t >= firstAllowableTime && t < endOfAllowableDestTimes)
        {
            auto newOne = new MidiEventHolder (m->message);
            newOne->message.setTimeStamp (t);
            list.add (newOne);
        }
    }

    sort();
}

struct MidiMessageSequenceSorter
{
    static int compareElements (const MidiMessageSequence::MidiEventHolder* first,
                                const MidiMessageSequence::MidiEventHolder* second) noexcept
    {
        auto diff = first->message.getTimeStamp() - second->message.getTimeStamp();
        return (diff > 0) - (diff < 0);
    }
};

void MidiMessageSequence::sort() noexcept
{
    MidiMessageSequenceSorter sorter;
    list.sort (sorter, true);
}

void MidiMessageSequence::updateMatchedPairs() noexcept
{
    for (int i = 0; i < list.size(); ++i)
    {
        auto* meh = list.getUnchecked(i);
        auto& m1 = meh->message;

        if (m1.isNoteOn())
        {
            meh->noteOffObject = nullptr;
            auto note = m1.getNoteNumber();
            auto chan = m1.getChannel();
            auto len = list.size();

            for (int j = i + 1; j < len; ++j)
            {
                auto* meh2 = list.getUnchecked(j);
                auto& m = meh2->message;

                if (m.getNoteNumber() == note && m.getChannel() == chan)
                {
                    if (m.isNoteOff())
                    {
                        meh->noteOffObject = meh2;
                        break;
                    }

                    if (m.isNoteOn())
                    {
                        auto newEvent = new MidiEventHolder (MidiMessage::noteOff (chan, note));
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

void MidiMessageSequence::addTimeToMessages (double delta) noexcept
{
    if (delta != 0)
        for (auto* m : list)
            m->message.addToTimeStamp (delta);
}

//==============================================================================
void MidiMessageSequence::extractMidiChannelMessages (const int channelNumberToExtract,
                                                      MidiMessageSequence& destSequence,
                                                      const bool alsoIncludeMetaEvents) const
{
    for (auto* meh : list)
        if (meh->message.isForChannel (channelNumberToExtract)
             || (alsoIncludeMetaEvents && meh->message.isMetaEvent()))
            destSequence.addEvent (meh->message);
}

void MidiMessageSequence::extractSysExMessages (MidiMessageSequence& destSequence) const
{
    for (auto* meh : list)
        if (meh->message.isSysEx())
            destSequence.addEvent (meh->message);
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
void MidiMessageSequence::createControllerUpdatesForTime (int channelNumber, double time, Array<MidiMessage>& dest)
{
    bool doneProg = false;
    bool donePitchWheel = false;
    bool doneControllers[128] = {};

    for (int i = list.size(); --i >= 0;)
    {
        auto& mm = list.getUnchecked(i)->message;

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
                jassert (isPositiveAndBelow (controllerNumber, 128));

                if (! doneControllers[controllerNumber])
                {
                    doneControllers[controllerNumber] = true;
                    dest.add (MidiMessage (mm, 0.0));
                }
            }
        }
    }
}

} // namespace juce
