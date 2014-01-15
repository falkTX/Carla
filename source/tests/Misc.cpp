/*
 * Carla Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngine.hpp"
#include "CarlaMIDI.h"
#include "CarlaUtils.hpp"

CARLA_BACKEND_USE_NAMESPACE

void testControlEventDump()
{
    uint8_t size;
    uint8_t data[3];

    EngineControlEvent e;

    // test null event
    e.type = kEngineControlEventTypeNull;
    e.dumpToMidiData(0, size, data);
    assert(size == 0);

    // test regular param event (half value)
    e.type  = kEngineControlEventTypeParameter;
    e.param = MIDI_CONTROL_MODULATION_WHEEL;
    e.value = 0.5f;
    e.dumpToMidiData(0, size, data);
    assert(size == 3);
    assert(data[0] == MIDI_STATUS_CONTROL_CHANGE);
    assert(data[1] == MIDI_CONTROL_MODULATION_WHEEL);
    assert(data[2] == (MAX_MIDI_VALUE-1)/2);

    // test regular param event (out of bounds positive)
    e.value = 9.0f;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == MAX_MIDI_VALUE-1);

    // test regular param event (out of bounds negative)
    e.value = -11.0f;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == 0);

    // test bad param as bank select
    e.param = MIDI_CONTROL_BANK_SELECT;
    e.value = 100;
    e.dumpToMidiData(0, size, data);
    assert(size == 3);
    assert(data[1] == MIDI_CONTROL_BANK_SELECT);
    assert(data[2] == 100);

    // test bad param as bank select (out of bounds index positive)
    e.value = 9999;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == MAX_MIDI_VALUE-1);

    // test bad param as bank select (out of bounds index negative)
    e.value = -9999;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == 0);

    // test bad param (out of bounds index)
    e.param = 999;
    e.dumpToMidiData(0, size, data);
    assert(size == 0);

    // test regular midi-bank event
    e.type  = kEngineControlEventTypeMidiBank;
    e.param = 11;
    e.dumpToMidiData(0, size, data);
    assert(size == 3);
    assert(data[0] == MIDI_STATUS_CONTROL_CHANGE);
    assert(data[1] == MIDI_CONTROL_BANK_SELECT);
    assert(data[2] == 11);

    // test bad midi-bank event (out of bounds)
    e.param = 300;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == MAX_MIDI_VALUE-1);

    // test bad midi-bank event (out of bounds #2)
    e.param = 32867;
    e.dumpToMidiData(0, size, data);
    assert(data[2] == MAX_MIDI_VALUE-1);

    // test regular midi-program event
    e.type  = kEngineControlEventTypeMidiProgram;
    e.param = 15;
    e.dumpToMidiData(0, size, data);
    assert(size == 2);
    assert(data[0] == MIDI_STATUS_PROGRAM_CHANGE);
    assert(data[1] == 15);

    // test bad midi-program event (out of bounds)
    e.param = 299;
    e.dumpToMidiData(0, size, data);
    assert(data[1] == MAX_MIDI_VALUE-1);

    // test regular all-sound-off event
    e.type  = kEngineControlEventTypeAllSoundOff;
    e.dumpToMidiData(0, size, data);
    assert(size == 2);
    assert(data[0] == MIDI_STATUS_CONTROL_CHANGE);
    assert(data[1] == MIDI_CONTROL_ALL_SOUND_OFF);

    // test regular all-notes-off event
    e.type  = kEngineControlEventTypeAllNotesOff;
    e.dumpToMidiData(0, size, data);
    assert(size == 2);
    assert(data[0] == MIDI_STATUS_CONTROL_CHANGE);
    assert(data[1] == MIDI_CONTROL_ALL_NOTES_OFF);

    // test channel bit
    e.dumpToMidiData(0, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 0);
    e.dumpToMidiData(1, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 1);
    e.dumpToMidiData(15, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 15);
    e.dumpToMidiData(16, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 0);
    e.dumpToMidiData(17, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 1);
    e.dumpToMidiData(31, size, data);
    assert(MIDI_GET_CHANNEL_FROM_DATA(data) == 15);
}

int main()
{
    testControlEventDump();
    return 0;
}
