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

#ifndef WATER_MIDIMESSAGESEQUENCE_H_INCLUDED
#define WATER_MIDIMESSAGESEQUENCE_H_INCLUDED

#include "MidiMessage.h"
#include "../containers/Array.h"
#include "../containers/OwnedArray.h"

namespace water {

//==============================================================================
/**
    A sequence of timestamped midi messages.

    This allows the sequence to be manipulated, and also to be read from and
    written to a standard midi file.

    @see MidiMessage, MidiFile
*/
class MidiMessageSequence
{
public:
    //==============================================================================
    /** Creates an empty midi sequence object. */
    MidiMessageSequence();

    /** Creates a copy of another sequence. */
    MidiMessageSequence (const MidiMessageSequence&);

    /** Replaces this sequence with another one. */
    MidiMessageSequence& operator= (const MidiMessageSequence&);

    /** Destructor. */
    ~MidiMessageSequence();

    //==============================================================================
    /** Structure used to hold midi events in the sequence.

        These structures act as 'handles' on the events as they are moved about in
        the list, and make it quick to find the matching note-offs for note-on events.

        @see MidiMessageSequence::getEventPointer
    */
    class MidiEventHolder
    {
    public:
        //==============================================================================
        /** Destructor. */
        ~MidiEventHolder();

        /** The message itself, whose timestamp is used to specify the event's time. */
        MidiMessage message;

        /** The matching note-off event (if this is a note-on event).

            If this isn't a note-on, this pointer will be nullptr.

            Use the MidiMessageSequence::updateMatchedPairs() method to keep these
            note-offs up-to-date after events have been moved around in the sequence
            or deleted.
        */
        MidiEventHolder* noteOffObject;

    private:
        //==============================================================================
        friend class MidiMessageSequence;
        MidiEventHolder (const MidiMessage&);

        CARLA_DECLARE_NON_COPY_CLASS(MidiEventHolder);
    };

    //==============================================================================
    /** Clears the sequence. */
    void clear();

    /** Returns the number of events in the sequence. */
    int getNumEvents() const noexcept;

    /** Returns a pointer to one of the events. */
    MidiEventHolder* getEventPointer (int index) const noexcept;

    //==============================================================================
    /** Returns the timestamp of the first event in the sequence.
        @see getEndTime
    */
    double getStartTime() const noexcept;

    /** Returns the timestamp of the last event in the sequence.
        @see getStartTime
    */
    double getEndTime() const noexcept;

    /** Returns the timestamp of the event at a given index.
        If the index is out-of-range, this will return 0.0
    */
    double getEventTime (int index) const noexcept;

    //==============================================================================
    /** Inserts a midi message into the sequence.

        The index at which the new message gets inserted will depend on its timestamp,
        because the sequence is kept sorted.

        Remember to call updateMatchedPairs() after adding note-on events.

        @param newMessage       the new message to add (an internal copy will be made)
        @param timeAdjustment   an optional value to add to the timestamp of the message
                                that will be inserted
        @see updateMatchedPairs
    */
    MidiEventHolder* addEvent (const MidiMessage& newMessage,
                               double timeAdjustment = 0);

    /** Merges another sequence into this one.
        Remember to call updateMatchedPairs() after using this method.

        @param other                    the sequence to add from
        @param timeAdjustmentDelta      an amount to add to the timestamps of the midi events
                                        as they are read from the other sequence
    */
    void addSequence (const MidiMessageSequence& other,
                      double timeAdjustmentDelta);

    //==============================================================================
    /** Makes sure all the note-on and note-off pairs are up-to-date.

        Call this after re-ordering messages or deleting/adding messages, and it
        will scan the list and make sure all the note-offs in the MidiEventHolder
        structures are pointing at the correct ones.
    */
    void updateMatchedPairs() noexcept;

    /** Forces a sort of the sequence.
        You may need to call this if you've manually modified the timestamps of some
        events such that the overall order now needs updating.
    */
    void sort() noexcept;

    //==============================================================================
    /** Swaps this sequence with another one. */
    void swapWith (MidiMessageSequence&) noexcept;

private:
    //==============================================================================
    friend class MidiFile;
    OwnedArray<MidiEventHolder> list;
};

}

#endif // WATER_MIDIMESSAGESEQUENCE_H_INCLUDED
