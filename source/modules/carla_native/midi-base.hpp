/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef MIDI_BASE_HPP_INCLUDED
#define MIDI_BASE_HPP_INCLUDED

#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "LinkedList.hpp"

#define MAX_EVENT_DATA_SIZE          4
#define MIN_PREALLOCATED_EVENT_COUNT 100
#define MAX_PREALLOCATED_EVENT_COUNT 1000

struct RawMidiEvent {
    uint64_t time;
    uint8_t  size;
    uint8_t  data[MAX_EVENT_DATA_SIZE];

    RawMidiEvent()
        : time(0),
          size(0)
    {
        carla_fill<uint8_t>(data, MAX_EVENT_DATA_SIZE, 0);
    }
};

class AbstractMidiPlayer
{
public:
    virtual ~AbstractMidiPlayer() {}
    virtual void writeMidiEvent(const uint64_t timePosFrame, const RawMidiEvent* const event) = 0;
};

class MidiPattern
{
public:
    MidiPattern(AbstractMidiPlayer* const player)
        : kPlayer(player),
          fStartTime(0),
          fDuration(0)
    {
        CARLA_ASSERT(kPlayer != nullptr);
    }

    ~MidiPattern()
    {
        fData.clear();
    }

    void addControl(const uint64_t time, const uint8_t channel, const uint8_t control, const uint8_t value)
    {
        RawMidiEvent* ctrlEvent(new RawMidiEvent());
        ctrlEvent->time    = time;
        ctrlEvent->size    = 3;
        ctrlEvent->data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (channel & 0x0F));
        ctrlEvent->data[1] = control;
        ctrlEvent->data[2] = value;

        append(ctrlEvent);
    }

    void addChannelPressure(const uint64_t time, const uint8_t channel, const uint8_t pressure)
    {
        RawMidiEvent* pressureEvent(new RawMidiEvent());
        pressureEvent->time    = time;
        pressureEvent->size    = 2;
        pressureEvent->data[0] = uint8_t(MIDI_STATUS_CHANNEL_PRESSURE | (channel & 0x0F));
        pressureEvent->data[1] = pressure;

        append(pressureEvent);
    }

    void addNote(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity, const uint32_t duration)
    {
        addNoteOn(time, channel, pitch, velocity);
        addNoteOff(time+duration, channel, pitch, velocity);
    }

    void addNoteOn(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
    {
        RawMidiEvent* noteOnEvent(new RawMidiEvent());
        noteOnEvent->time    = time;
        noteOnEvent->size    = 3;
        noteOnEvent->data[0] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & 0x0F));
        noteOnEvent->data[1] = pitch;
        noteOnEvent->data[2] = velocity;

        append(noteOnEvent);
    }

    void addNoteOff(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity = 0)
    {
        RawMidiEvent* noteOffEvent(new RawMidiEvent());
        noteOffEvent->time    = time;
        noteOffEvent->size    = 3;
        noteOffEvent->data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (channel & 0x0F));
        noteOffEvent->data[1] = pitch;
        noteOffEvent->data[2] = velocity;

        append(noteOffEvent);
    }

    void addNoteAftertouch(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t pressure)
    {
        RawMidiEvent* noteAfterEvent(new RawMidiEvent());
        noteAfterEvent->time    = time;
        noteAfterEvent->size    = 3;
        noteAfterEvent->data[0] = uint8_t(MIDI_STATUS_POLYPHONIC_AFTERTOUCH | (channel & 0x0F));
        noteAfterEvent->data[1] = pitch;
        noteAfterEvent->data[2] = pressure;

        append(noteAfterEvent);
    }

    void addProgram(const uint64_t time, const uint8_t channel, const uint8_t bank, const uint8_t program)
    {
        RawMidiEvent* bankEvent(new RawMidiEvent());
        bankEvent->time    = time;
        bankEvent->size    = 3;
        bankEvent->data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (channel & 0x0F));
        bankEvent->data[1] = MIDI_CONTROL_BANK_SELECT;
        bankEvent->data[2] = bank;

        RawMidiEvent* programEvent(new RawMidiEvent());
        programEvent->time    = time;
        programEvent->size    = 2;
        programEvent->data[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (channel & 0x0F));
        programEvent->data[1] = program;

        append(bankEvent);
        append(programEvent);
    }

    void addPitchbend(const uint64_t time, const uint8_t channel, const uint8_t lsb, const uint8_t msb)
    {
        RawMidiEvent* pressureEvent(new RawMidiEvent());
        pressureEvent->time    = time;
        pressureEvent->size    = 3;
        pressureEvent->data[0] = uint8_t(MIDI_STATUS_PITCH_WHEEL_CONTROL | (channel & 0x0F));
        pressureEvent->data[1] = lsb;
        pressureEvent->data[2] = msb;

        append(pressureEvent);
    }

    void addRaw(const uint64_t time, const uint8_t* data, const uint8_t size)
    {
        RawMidiEvent* rawEvent(new RawMidiEvent());
        rawEvent->time = time;
        rawEvent->size = size;

        carla_copy<uint8_t>(rawEvent->data, data, size);

        append(rawEvent);
    }

    void play(uint64_t timePosFrame, uint32_t frames)
    {
        if (! fMutex.tryLock())
            return;

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue());

            if (timePosFrame > rawMidiEvent->time)
                continue;
            if (timePosFrame + frames <= rawMidiEvent->time)
                continue;

            kPlayer->writeMidiEvent(timePosFrame, rawMidiEvent);
        }

        fMutex.unlock();
    }

    void clear()
    {
        const CarlaMutex::ScopedLocker sl(fMutex);
        fData.clear();
    }

private:
    AbstractMidiPlayer* const kPlayer;

    uint32_t fStartTime; // unused
    uint32_t fDuration;  // unused

    CarlaMutex fMutex;
    LinkedList<const RawMidiEvent*> fData;

    void append(const RawMidiEvent* const event)
    {
        if (fData.isEmpty())
        {
            const CarlaMutex::ScopedLocker sl(fMutex);
            fData.append(event);
            return;
        }

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin(); it.valid(); it.next())
        {
            const RawMidiEvent* const oldEvent(it.getValue());

            if (event->time >= oldEvent->time)
                continue;

            const CarlaMutex::ScopedLocker sl(fMutex);
            fData.insertAt(event, it);
            return;
        }

        const CarlaMutex::ScopedLocker sl(fMutex);
        fData.append(event);
    }
};

#endif // MIDI_BASE_HPP_INCLUDED
