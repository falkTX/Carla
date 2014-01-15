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
    e.type = kEngineControlEventTypeAllSoundOff;
    e.dumpToMidiData(0, size, data);
    assert(size == 2);
    assert(data[0] == MIDI_STATUS_CONTROL_CHANGE);
    assert(data[1] == MIDI_CONTROL_ALL_SOUND_OFF);

    // test regular all-notes-off event
    e.type = kEngineControlEventTypeAllNotesOff;
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

void testEventMidiFill()
{
    uint8_t size;
    uint8_t data[25];

    //EngineControlEvent c;
    EngineEvent e;
    e.time = 0;

    // test safety first
    e.fillFromMidiData(0, data);
    assert(e.type == kEngineEventTypeNull);
    assert(e.channel == 0);
    e.fillFromMidiData(50, nullptr);
    assert(e.type == kEngineEventTypeNull);
    assert(e.channel == 0);

    // test bad midi event
    size = 3;
    data[0] = 0x45;
    data[1] = 100;
    data[2] = 100;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeNull);
    assert(e.channel == 0);

    // test simple midi event
    size = 3;
    data[0] = 0x82; // MIDI_STATUS_NOTE_OFF + 2
    data[1] = 127;
    data[2] = 127;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeMidi);
    assert(e.channel == 2);
    assert(e.midi.size == size);
    assert(e.midi.dataExt == nullptr);
    assert(e.midi.data[0] == 0x80); // without channel bit
    assert(e.midi.data[1] == data[1]);
    assert(e.midi.data[2] == data[2]);

    // test sysex midi event
    size = 11;
    data[ 0] = 0xF0;
    data[ 1] = 0x41;
    data[ 2] = 0x10;
    data[ 3] = 0x42;
    data[ 4] = 0x12;
    data[ 5] = 0x40;
    data[ 6] = 0x00;
    data[ 7] = 0x7F;
    data[ 8] = 0x00;
    data[ 9] = 0x41;
    data[10] = 0xF7;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeMidi);
    assert(e.channel == 0);
    assert(e.midi.size == size);
    assert(e.midi.dataExt == data);
    assert(e.midi.data[0] == 0);
    assert(e.midi.data[1] == 0);
    assert(e.midi.data[2] == 0);
    assert(e.midi.data[3] == 0);

    // test all-sound-off event
    size = 2;
    data[0] = 0xB5; // MIDI_STATUS_CONTROL_CHANGE + 5
    data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeControl);
    assert(e.channel == 5);
    assert(e.ctrl.type == kEngineControlEventTypeAllSoundOff);
    assert(e.ctrl.param == 0);
    assert(e.ctrl.value == 0.0f);

    // test all-notes-off event
    size = 2;
    data[0] = 0xBF; // MIDI_STATUS_CONTROL_CHANGE + 15
    data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeControl);
    assert(e.channel == 15);
    assert(e.ctrl.type == kEngineControlEventTypeAllNotesOff);
    assert(e.ctrl.param == 0);
    assert(e.ctrl.value == 0.0f);

    // test midi-bank event
    size = 3;
    data[0] = 0xB1; // MIDI_STATUS_CONTROL_CHANGE + 1
    data[1] = MIDI_CONTROL_BANK_SELECT;
    data[2] = 123;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeControl);
    assert(e.channel == 1);
    assert(e.ctrl.type == kEngineControlEventTypeMidiBank);
    assert(e.ctrl.param == 123);
    assert(e.ctrl.value == 0.0f);

    // test midi-program event
    size = 2;
    data[0] = MIDI_STATUS_PROGRAM_CHANGE;
    data[1] = 77;
    e.fillFromMidiData(size, data);
    assert(e.type == kEngineEventTypeControl);
    assert(e.channel == 0);
    assert(e.ctrl.type == kEngineControlEventTypeMidiProgram);
    assert(e.ctrl.param == 77);
    assert(e.ctrl.value == 0.0f);
}

int main()
{
    testControlEventDump();
    testEventMidiFill();
    return 0;
}
