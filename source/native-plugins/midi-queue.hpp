/*
 * MIDI Event Queue
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef MIDI_QUEUE_HPP_INCLUDED
#define MIDI_QUEUE_HPP_INCLUDED

#include "CarlaMutex.hpp"

template<uint16_t MAX_SIZE>
class MIDIEventQueue
{
public:
    MIDIEventQueue() noexcept
        : index(0),
          empty(true),
          full(false),
          mutex() {}

    bool isEmpty() const noexcept
    {
        return empty;
    }

    bool isNotEmpty() const noexcept
    {
        return !empty;
    }

    bool isFull() const noexcept
    {
        return full;
    }

    CarlaMutex& getMutex() noexcept
    {
        return mutex;
    }

    bool put(unsigned char d1, unsigned char d2, unsigned char d3)
    {
        CARLA_SAFE_ASSERT_RETURN(d1 != 0, false);

        if (full)
            return false;

        for (unsigned short i=0; i < MAX_SIZE; i++)
        {
            if (data[i].d1 == 0)
            {
                data[i].d1 = d1;
                data[i].d2 = d2;
                data[i].d3 = d3;
                empty = false;
                full  = (i == MAX_SIZE-1);
                break;
            }
        }

        return true;
    }

    bool get(uint8_t& d1, uint8_t& d2, uint8_t& d3)
    {
        if (empty)
            return false;

        full = false;

        if (data[index].d1 == 0)
        {
            index = 0;
            empty = true;

            return false;
        }

        d1 = data[index].d1;
        d2 = data[index].d2;
        d3 = data[index].d3;

        data[index].d1 = data[index].d2 = data[index].d3 = 0;
        empty = false;
        ++index;

        return true;
    }

    bool tryToCopyDataFrom(MIDIEventQueue& queue) noexcept
    {
        const CarlaMutexTryLocker cml(queue.mutex);

        if (cml.wasNotLocked())
            return false;

        // copy data from queue
        carla_copyStruct(data, queue.data);
        index = queue.index;
        empty = queue.empty;
        full  = queue.full;

        // reset queque
        carla_zeroStruct(queue.data);
        queue.index = 0;
        queue.empty = true;
        queue.full  = false;

        return true;
    }

private:
    struct MIDIEvent {
        uint8_t d1, d2, d3;

        MIDIEvent() noexcept
            : d1(0), d2(0), d3(0) {}
    };

    MIDIEvent data[MAX_SIZE];
    uint16_t index;
    volatile bool empty, full;

    CarlaMutex mutex;
};

#endif // MIDI_QUEUE_HPP_INCLUDED
