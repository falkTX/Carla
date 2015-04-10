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

#include "CarlaBackend.h"
#include "LinkedList.hpp"

#include "juce_core.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

struct CarlaStateSave {
    struct Parameter {
        bool        dummy; // if true only midiChannel/CC are used
        int32_t     index;
        const char* name;
        const char* symbol;
        float       value;
#ifndef BUILD_BRIDGE
        uint8_t     midiChannel;
        int16_t     midiCC;
#endif

        Parameter() noexcept;
        ~Parameter() noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(Parameter)
    };

    typedef LinkedList<Parameter*> ParameterList;
    typedef LinkedList<Parameter*>::Itenerator ParameterItenerator;

    struct CustomData {
        const char* type;
        const char* key;
        const char* value;

        CustomData() noexcept;
        ~CustomData() noexcept;
        bool isValid() const noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(CustomData)
    };

    typedef LinkedList<CustomData*> CustomDataList;
    typedef LinkedList<CustomData*>::Itenerator CustomDataItenerator;

    const char* type;
    const char* name;
    const char* label;
    const char* binary;
    int64_t     uniqueId;
    uint        options;

#ifndef BUILD_BRIDGE
    bool   active;
    float  dryWet;
    float  volume;
    float  balanceLeft;
    float  balanceRight;
    float  panning;
    int8_t ctrlChannel;
#endif

    int32_t     currentProgramIndex;
    const char* currentProgramName;
    int32_t     currentMidiBank;
    int32_t     currentMidiProgram;
    const char* chunk;

    ParameterList parameters;
    CustomDataList customData;

    CarlaStateSave() noexcept;
    ~CarlaStateSave() noexcept;
    void clear() noexcept;

    bool fillFromXmlElement(const juce::XmlElement* const xmlElement);
    void dumpToMemoryStream(juce::MemoryOutputStream& stream) const;

    CARLA_DECLARE_NON_COPY_STRUCT(CarlaStateSave)
};

static inline
juce::String xmlSafeString(const char* const cstring, const bool toXml)
{
    juce::String newString = juce::String(juce::CharPointer_UTF8(cstring));

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}

static inline
juce::String xmlSafeString(const juce::String& string, const bool toXml)
{
    juce::String newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_STATE_UTILS_HPP_INCLUDED
