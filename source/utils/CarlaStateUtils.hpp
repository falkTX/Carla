/*
 * Carla State utils
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_STATE_UTILS_HPP_INCLUDED
#define CARLA_STATE_UTILS_HPP_INCLUDED

#include "LinkedList.hpp"

class QDomNode;
class QString;

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------

struct StateParameter {
    int32_t     index;
    const char* name;
    const char* symbol;
    float       value;
    uint8_t     midiChannel;
    int16_t     midiCC;

    StateParameter() noexcept;
    ~StateParameter();

    CARLA_DECLARE_NON_COPY_STRUCT(StateParameter)
};

struct StateCustomData {
    const char* type;
    const char* key;
    const char* value;

    StateCustomData() noexcept;
    ~StateCustomData();

    CARLA_DECLARE_NON_COPY_STRUCT(StateCustomData)
};

typedef LinkedList<StateParameter*> StateParameterList;
typedef LinkedList<StateCustomData*> StateCustomDataList;

typedef LinkedList<StateParameter*>::Itenerator StateParameterItenerator;
typedef LinkedList<StateCustomData*>::Itenerator StateCustomDataItenerator;

struct SaveState {
    const char* type;
    const char* name;
    const char* label;
    const char* binary;
    int64_t     uniqueId;

    bool  active;
    float dryWet;
    float volume;
    float balanceLeft;
    float balanceRight;
    float panning;
    int8_t ctrlChannel;

    int32_t     currentProgramIndex;
    const char* currentProgramName;
    int32_t     currentMidiBank;
    int32_t     currentMidiProgram;
    const char* chunk;

    StateParameterList parameters;
    StateCustomDataList customData;

    SaveState() noexcept;
    ~SaveState();
    void reset();

    CARLA_DECLARE_NON_COPY_STRUCT(SaveState)
};

// -----------------------------------------------------------------------

void fillSaveStateFromXmlNode(SaveState& saveState, const QDomNode& xmlNode);
void fillXmlStringFromSaveState(QString& content, const SaveState& saveState);

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_STATE_UTILS_HPP_INCLUDED
