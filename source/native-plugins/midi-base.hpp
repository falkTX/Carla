/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaJuceUtils.hpp"
#include "CarlaMathUtils.hpp"

// -----------------------------------------------------------------------

#define MAX_EVENT_DATA_SIZE          4
#define MIN_PREALLOCATED_EVENT_COUNT 100
#define MAX_PREALLOCATED_EVENT_COUNT 1000

// -----------------------------------------------------------------------

struct RawMidiEvent {
    uint64_t time;
    uint8_t  size;
    uint8_t  data[MAX_EVENT_DATA_SIZE];
};

// -----------------------------------------------------------------------

class AbstractMidiPlayer
{
public:
    virtual ~AbstractMidiPlayer() {}
    virtual void writeMidiEvent(const uint8_t port, const long double timePosFrame, const RawMidiEvent* const event) = 0;
};

// -----------------------------------------------------------------------

class MidiPattern
{
public:
    MidiPattern(AbstractMidiPlayer* const player) noexcept
        : kPlayer(player),
          fMidiPort(0),
          fStartTime(0),
          fMutex(),
          fData()
    {
        CARLA_SAFE_ASSERT(kPlayer != nullptr);
    }

    ~MidiPattern() noexcept
    {
        clear();
    }

    // -------------------------------------------------------------------
    // add data, time always counts from 0

    void addControl(const uint64_t time, const uint8_t channel, const uint8_t control, const uint8_t value)
    {
        RawMidiEvent* const ctrlEvent(new RawMidiEvent());
        ctrlEvent->time    = time;
        ctrlEvent->size    = 3;
        ctrlEvent->data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        ctrlEvent->data[1] = control;
        ctrlEvent->data[2] = value;

        appendSorted(ctrlEvent);
    }

    void addChannelPressure(const uint64_t time, const uint8_t channel, const uint8_t pressure)
    {
        RawMidiEvent* const pressureEvent(new RawMidiEvent());
        pressureEvent->time    = time;
        pressureEvent->size    = 2;
        pressureEvent->data[0] = uint8_t(MIDI_STATUS_CHANNEL_PRESSURE | (channel & MIDI_CHANNEL_BIT));
        pressureEvent->data[1] = pressure;

        appendSorted(pressureEvent);
    }

    void addNote(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity, const uint32_t duration)
    {
        addNoteOn(time, channel, pitch, velocity);
        addNoteOff(time+duration, channel, pitch, velocity);
    }

    void addNoteOn(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
    {
        RawMidiEvent* const noteOnEvent(new RawMidiEvent());
        noteOnEvent->time    = time;
        noteOnEvent->size    = 3;
        noteOnEvent->data[0] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT));
        noteOnEvent->data[1] = pitch;
        noteOnEvent->data[2] = velocity;

        appendSorted(noteOnEvent);
    }

    void addNoteOff(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity = 0)
    {
        RawMidiEvent* const noteOffEvent(new RawMidiEvent());
        noteOffEvent->time    = time;
        noteOffEvent->size    = 3;
        noteOffEvent->data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (channel & MIDI_CHANNEL_BIT));
        noteOffEvent->data[1] = pitch;
        noteOffEvent->data[2] = velocity;

        appendSorted(noteOffEvent);
    }

    void addNoteAftertouch(const uint64_t time, const uint8_t channel, const uint8_t pitch, const uint8_t pressure)
    {
        RawMidiEvent* const noteAfterEvent(new RawMidiEvent());
        noteAfterEvent->time    = time;
        noteAfterEvent->size    = 3;
        noteAfterEvent->data[0] = uint8_t(MIDI_STATUS_POLYPHONIC_AFTERTOUCH | (channel & MIDI_CHANNEL_BIT));
        noteAfterEvent->data[1] = pitch;
        noteAfterEvent->data[2] = pressure;

        appendSorted(noteAfterEvent);
    }

    void addProgram(const uint64_t time, const uint8_t channel, const uint8_t bank, const uint8_t program)
    {
        RawMidiEvent* const bankEvent(new RawMidiEvent());
        bankEvent->time    = time;
        bankEvent->size    = 3;
        bankEvent->data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        bankEvent->data[1] = MIDI_CONTROL_BANK_SELECT;
        bankEvent->data[2] = bank;

        RawMidiEvent* const programEvent(new RawMidiEvent());
        programEvent->time    = time;
        programEvent->size    = 2;
        programEvent->data[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (channel & MIDI_CHANNEL_BIT));
        programEvent->data[1] = program;

        appendSorted(bankEvent);
        appendSorted(programEvent);
    }

    void addPitchbend(const uint64_t time, const uint8_t channel, const uint8_t lsb, const uint8_t msb)
    {
        RawMidiEvent* const pressureEvent(new RawMidiEvent());
        pressureEvent->time    = time;
        pressureEvent->size    = 3;
        pressureEvent->data[0] = uint8_t(MIDI_STATUS_PITCH_WHEEL_CONTROL | (channel & MIDI_CHANNEL_BIT));
        pressureEvent->data[1] = lsb;
        pressureEvent->data[2] = msb;

        appendSorted(pressureEvent);
    }

    void addRaw(const uint64_t time, const uint8_t* const data, const uint8_t size)
    {
        RawMidiEvent* const rawEvent(new RawMidiEvent());
        rawEvent->time = time;
        rawEvent->size = size;

        carla_copy<uint8_t>(rawEvent->data, data, size);

        appendSorted(rawEvent);
    }

    // -------------------------------------------------------------------
    // remove data

    void removeRaw(const uint64_t time, const uint8_t* const data, const uint8_t size)
    {
        const CarlaMutexLocker sl(fMutex);

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin2(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(rawMidiEvent != nullptr);

            if (rawMidiEvent->time != time)
                continue;
            if (rawMidiEvent->size != size)
                continue;
            if (std::memcmp(rawMidiEvent->data, data, size) != 0)
                continue;

            delete rawMidiEvent;
            fData.remove(it);

            return;
        }

        carla_stderr("MidiPattern::removeRaw(" P_INT64 ", %p, %i) - unable to find event to remove", time, data, size);
    }

    // -------------------------------------------------------------------
    // clear

    void clear() noexcept
    {
        const CarlaMutexLocker sl(fMutex);

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin2(); it.valid(); it.next())
            delete it.getValue(nullptr);

        fData.clear();
    }

    // -------------------------------------------------------------------
    // play on time

    void play(const uint64_t timePosFrame, const uint32_t frames)
    {
        play(static_cast<long double>(timePosFrame), static_cast<double>(frames));
    }

    void play(long double timePosFrame, const double frames)
    {
        if (! fMutex.tryLock())
            return;

        if (fStartTime != 0)
            timePosFrame += static_cast<long double>(fStartTime);

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin2(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(rawMidiEvent != nullptr);

            if (timePosFrame > rawMidiEvent->time)
                continue;
            if (timePosFrame + frames <= rawMidiEvent->time)
                continue;

            kPlayer->writeMidiEvent(fMidiPort, static_cast<long double>(rawMidiEvent->time)-timePosFrame, rawMidiEvent);
        }

        fMutex.unlock();
    }

    // -------------------------------------------------------------------
    // configure

    void setMidiPort(const uint8_t port) noexcept
    {
        fMidiPort = port;
    }

    void setStartTime(const uint64_t time) noexcept
    {
        fStartTime = time;
    }

    // -------------------------------------------------------------------
    // special

    const CarlaMutex& getLock() const noexcept
    {
        return fMutex;
    }

    LinkedList<const RawMidiEvent*>::Itenerator iteneratorBegin() const noexcept
    {
        return fData.begin2();
    }

    // -------------------------------------------------------------------
    // state

    char* getState() const
    {
        static const std::size_t maxTimeSize = 20;                        // std::strlen("18446744073709551615");
        static const std::size_t maxDataSize = 4 + 4*MAX_EVENT_DATA_SIZE; // std::strlen("0xFF:127:127:127");
        static const std::size_t maxMsgSize  = maxTimeSize + 3 /* sep + size + sep */ + maxDataSize + 1 /* newline */;

        const CarlaMutexLocker sl(fMutex);

        if (fData.count() == 0)
            return nullptr;

        char* const data((char*)std::calloc(1, fData.count()*maxMsgSize));
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, nullptr);

        char* dataWrtn = data;
        int wrtn;

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin2(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(rawMidiEvent != nullptr);

            wrtn = std::snprintf(dataWrtn, maxTimeSize+4, P_INT64 ":%i:", rawMidiEvent->time, rawMidiEvent->size);
            CARLA_SAFE_ASSERT_BREAK(wrtn > 0);
            dataWrtn += wrtn;

            wrtn = std::snprintf(dataWrtn, 5, "0x%02X", rawMidiEvent->data[0]);
            CARLA_SAFE_ASSERT_BREAK(wrtn > 0);
            dataWrtn += wrtn;

            for (uint8_t i=1, size=rawMidiEvent->size; i<size; ++i)
            {
                wrtn = std::snprintf(dataWrtn, 5, ":%03u", rawMidiEvent->data[i]);
                CARLA_SAFE_ASSERT_BREAK(wrtn > 0);
                dataWrtn += wrtn;
            }

            *dataWrtn++ = '\n';
        }

        *dataWrtn = '\0';

        return data;
    }

    void setState(const char* const data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        const char* dataRead = data;
        const char* needle;
        RawMidiEvent midiEvent;
        char    tmpBuf[24];
        ssize_t tmpSize;

        clear();

        const CarlaMutexLocker sl(fMutex);

        for (; *dataRead != '\0';)
        {
            // get time
            needle = std::strchr(dataRead, ':');

            if (needle == nullptr)
                return;

            carla_zeroStruct(midiEvent);

            tmpSize = needle - dataRead;
            CARLA_SAFE_ASSERT_RETURN(tmpSize > 0 && tmpSize < 24,);

            std::strncpy(tmpBuf, dataRead, static_cast<size_t>(tmpSize));
            tmpBuf[tmpSize] = '\0';
            dataRead += tmpSize+1;

            const long long time = std::atoll(tmpBuf);
            CARLA_SAFE_ASSERT_RETURN(time >= 0,);

            midiEvent.time = static_cast<uint64_t>(time);

            // get size
            needle = std::strchr(dataRead, ':');
            CARLA_SAFE_ASSERT_RETURN(needle != nullptr,);

            tmpSize = needle - dataRead;
            CARLA_SAFE_ASSERT_RETURN(tmpSize > 0 && tmpSize < 24,);

            std::strncpy(tmpBuf, dataRead, static_cast<size_t>(tmpSize));
            tmpBuf[tmpSize] = '\0';
            dataRead += tmpSize+1;

            const int size = std::atoi(tmpBuf);
            CARLA_SAFE_ASSERT_RETURN(size > 0 && size <= MAX_EVENT_DATA_SIZE,);

            midiEvent.size = static_cast<uint8_t>(size);

            // get events
            for (int i=0; i<size; ++i)
            {
                CARLA_SAFE_ASSERT_RETURN(dataRead-data >= 4,);

                tmpSize = i==0 ? 4 : 3;

                std::strncpy(tmpBuf, dataRead, static_cast<size_t>(tmpSize));
                tmpBuf[tmpSize] = '\0';
                dataRead += tmpSize+1;

                long mdata;

                if (i == 0)
                {
                    mdata = std::strtol(tmpBuf, nullptr, 16);
                    CARLA_SAFE_ASSERT_RETURN(mdata >= 0x80 && mdata <= 0xFF,);
                }
                else
                {
                    mdata = std::atoi(tmpBuf);
                    CARLA_SAFE_ASSERT_RETURN(mdata >= 0 && mdata < MAX_MIDI_VALUE,);
                }

                midiEvent.data[i] = static_cast<uint8_t>(mdata);
            }

            for (int i=size; i<MAX_EVENT_DATA_SIZE; ++i)
                midiEvent.data[i] = 0;

            RawMidiEvent* const event(new RawMidiEvent());
            carla_copyStruct(*event, midiEvent);
            fData.append(event);
        }
    }

    // -------------------------------------------------------------------

private:
    AbstractMidiPlayer* const kPlayer;

    uint8_t  fMidiPort;
    uint64_t fStartTime;

    CarlaMutex fMutex;
    LinkedList<const RawMidiEvent*> fData;

    void appendSorted(const RawMidiEvent* const event)
    {
        const CarlaMutexLocker sl(fMutex);

        if (fData.isEmpty())
        {
            fData.append(event);
            return;
        }

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fData.begin2(); it.valid(); it.next())
        {
            const RawMidiEvent* const oldEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(oldEvent != nullptr);

            if (event->time >= oldEvent->time)
                continue;

            fData.insertAt(event, it);
            return;
        }

        fData.append(event);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPattern)
};

// -----------------------------------------------------------------------

#endif // MIDI_BASE_HPP_INCLUDED
