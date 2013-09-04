/*
 * Carla State utils
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include "juce_core.h"

using juce::String;
using juce::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

struct StateParameter {
    uint32_t    index;
    const char* name;
    const char* symbol;
    float       value;
    uint8_t     midiChannel;
    int16_t     midiCC;

    StateParameter() noexcept
        : index(0),
          name(nullptr),
          symbol(nullptr),
          value(0.0f),
          midiChannel(0),
          midiCC(-1) {}

    ~StateParameter()
    {
        if (name != nullptr)
        {
            delete[] name;
            name = nullptr;
        }
        if (symbol != nullptr)
        {
            delete[] symbol;
            symbol = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(StateParameter)
};

struct StateCustomData {
    const char* type;
    const char* key;
    const char* value;

    StateCustomData() noexcept
        : type(nullptr),
          key(nullptr),
          value(nullptr) {}

    ~StateCustomData()
    {
        if (type != nullptr)
        {
            delete[] type;
            type = nullptr;
        }
        if (key != nullptr)
        {
            delete[] key;
            key = nullptr;
        }
        if (value != nullptr)
        {
            delete[] value;
            value = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(StateCustomData)
};

typedef NonRtList<StateParameter*> StateParameterList;
typedef NonRtList<StateCustomData*> StateCustomDataList;

typedef NonRtList<StateParameter*>::Itenerator StateParameterItenerator;
typedef NonRtList<StateCustomData*>::Itenerator StateCustomDataItenerator;

struct SaveState {
    const char* type;
    const char* name;
    const char* label;
    const char* binary;
    long        uniqueID;

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

    SaveState() noexcept
        : type(nullptr),
          name(nullptr),
          label(nullptr),
          binary(nullptr),
          uniqueID(0),
          active(false),
          dryWet(1.0f),
          volume(1.0f),
          balanceLeft(-1.0f),
          balanceRight(1.0f),
          panning(0.0f),
          ctrlChannel(-1),
          currentProgramIndex(-1),
          currentProgramName(nullptr),
          currentMidiBank(-1),
          currentMidiProgram(-1),
          chunk(nullptr) {}

    ~SaveState()
    {
        reset();
    }

    void reset()
    {
        if (type != nullptr)
        {
            delete[] type;
            type = nullptr;
        }
        if (name != nullptr)
        {
            delete[] name;
            name = nullptr;
        }
        if (label != nullptr)
        {
            delete[] label;
            label = nullptr;
        }
        if (binary != nullptr)
        {
            delete[] binary;
            binary = nullptr;
        }
        if (currentProgramName != nullptr)
        {
            delete[] currentProgramName;
            currentProgramName = nullptr;
        }
        if (chunk != nullptr)
        {
            delete[] chunk;
            chunk = nullptr;
        }

        uniqueID = 0;
        active = false;
        dryWet = 1.0f;
        volume = 1.0f;
        balanceLeft  = -1.0f;
        balanceRight = 1.0f;
        panning      = 0.0f;
        ctrlChannel  = -1;
        currentProgramIndex = -1;
        currentMidiBank     = -1;
        currentMidiProgram  = -1;

        for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
        {
            StateParameter* const stateParameter(*it);
            delete stateParameter;
        }

        for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
        {
            StateCustomData* const stateCustomData(*it);
            delete stateCustomData;
        }

        parameters.clear();
        customData.clear();
    }

    CARLA_DECLARE_NON_COPY_STRUCT(SaveState)
};

// -----------------------------------------------------------------------

static inline
String xmlSafeString(const String& string, const bool toXml)
{
    String newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&amp;","&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"");
}

static inline
const char* xmlSafeStringCharDup(const String& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toRawUTF8());
}

// -----------------------------------------------------------------------

static inline
void fillSaveStateFromXmlElement(SaveState& saveState, const XmlElement& xmlElement)
{
    for (XmlElement* elem = xmlElement.getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        // ---------------------------------------------------------------
        // Info

        if (elem->getTagName().equalsIgnoreCase("info"))
        {
            for (XmlElement* xmlInfo = elem->getFirstChildElement(); xmlInfo != nullptr; xmlInfo = xmlInfo->getNextElement())
            {
                const String& tag(xmlInfo->getTagName());
                const String  text(xmlInfo->getAllSubText().trim());

                if (tag.equalsIgnoreCase("type"))
                    saveState.type = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("name"))
                    saveState.name = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("label") || tag.equalsIgnoreCase("uri"))
                    saveState.label = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("binary") || tag.equalsIgnoreCase("filename"))
                    saveState.binary = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("uniqueid"))
                    saveState.uniqueID = text.getLargeIntValue();
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (elem->getTagName().equalsIgnoreCase("data"))
        {
            for (XmlElement* xmlData = elem->getFirstChildElement(); xmlData != nullptr; xmlData = xmlData->getNextElement())
            {
                const String& tag(xmlData->getTagName());
                const String  text(xmlData->getAllSubText().trim());

                // -------------------------------------------------------
                // Internal Data

                if (tag.equalsIgnoreCase("active"))
                {
                    saveState.active = (text.equalsIgnoreCase("yes"));
                }
                else if (tag.equalsIgnoreCase("drywet"))
                {
                    saveState.dryWet = carla_fixValue(0.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("volume"))
                {
                    saveState.volume = carla_fixValue(0.0f, 1.27f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceleft") || tag.equalsIgnoreCase("balance-left"))
                {
                    saveState.balanceLeft = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceright") || tag.equalsIgnoreCase("balance-right"))
                {
                    saveState.balanceRight = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("panning"))
                {
                    saveState.panning = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("controlchannel") || tag.equalsIgnoreCase("control-channel"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1 && value <= MAX_MIDI_CHANNELS)
                        saveState.ctrlChannel = static_cast<int8_t>(value-1);
                }

                // -------------------------------------------------------
                // Program (current)

                else if (tag.equalsIgnoreCase("currentprogramindex") || tag.equalsIgnoreCase("current-program-index"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        saveState.currentProgramIndex = value-1;
                }
                else if (tag.equalsIgnoreCase("currentprogramname") || tag.equalsIgnoreCase("current-program-name"))
                {
                    saveState.currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.equalsIgnoreCase("currentmidibank") || tag.equalsIgnoreCase("current-midi-bank"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        saveState.currentMidiBank = value-1;
                }
                else if (tag.equalsIgnoreCase("currentmidiprogram") || tag.equalsIgnoreCase("current-midi-program"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        saveState.currentMidiProgram = value-1;
                }

                // -------------------------------------------------------
                // Parameters

                else if (tag.equalsIgnoreCase("parameter"))
                {
                    StateParameter* const stateParameter(new StateParameter());

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& pTag(xmlSubData->getTagName());
                        const String  pText(xmlSubData->getAllSubText().trim());

                        if (pTag.equalsIgnoreCase("index"))
                        {
                            const int index(pText.getIntValue());
                            if (index >= 0)
                                stateParameter->index = index;
                        }
                        else if (pTag.equalsIgnoreCase("name"))
                        {
                            stateParameter->name = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.equalsIgnoreCase("symbol"))
                        {
                            stateParameter->symbol = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.equalsIgnoreCase("value"))
                        {
                            stateParameter->value = pText.getFloatValue();
                        }
                        else if (pTag.equalsIgnoreCase("midichannel") || pTag.equalsIgnoreCase("midi-channel"))
                        {
                            const int channel(pText.getIntValue());
                            if (channel >= 1 && channel <= MAX_MIDI_CHANNELS)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel-1);
                        }
                        else if (pTag.equalsIgnoreCase("midicc") || pTag.equalsIgnoreCase("midi-cc"))
                        {
                            const int cc(pText.getIntValue());
                            if (cc >= 1 && cc < 0x5F)
                                stateParameter->midiCC = static_cast<int16_t>(cc);
                        }
                    }

                    saveState.parameters.append(stateParameter);
                }

                // -------------------------------------------------------
                // Custom Data

                else if (tag.equalsIgnoreCase("customdata") || tag.equalsIgnoreCase("custom-data"))
                {
                    StateCustomData* const stateCustomData(new StateCustomData());

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& cTag(xmlSubData->getTagName());
                        const String  cText(xmlSubData->getAllSubText().trim());

                        if (cTag.equalsIgnoreCase("type"))
                            stateCustomData->type = xmlSafeStringCharDup(cText, false);
                        else if (cTag.equalsIgnoreCase("key"))
                            stateCustomData->key = xmlSafeStringCharDup(cText, false);
                        else if (cTag.equalsIgnoreCase("value"))
                            stateCustomData->value = xmlSafeStringCharDup(cText, false);
                    }

                    saveState.customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.equalsIgnoreCase("chunk"))
                {
                    saveState.chunk = xmlSafeStringCharDup(text, false);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------

static inline
void fillXmlStringFromSaveState(String& content, const SaveState& saveState)
{
    {
        String info("  <Info>\n");

        info << "   <Type>" << saveState.type                      << "</Type>\n";
        info << "   <Name>" << xmlSafeString(saveState.name, true) << "</Name>\n";

        switch (getPluginTypeFromString(saveState.type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            info << "   <Label>"    << xmlSafeString(saveState.label, true)  << "</Label>\n";
            break;
        case PLUGIN_LADSPA:
            info << "   <Binary>"   << xmlSafeString(saveState.binary, true) << "</Binary>\n";
            info << "   <Label>"    << xmlSafeString(saveState.label, true)  << "</Label>\n";
            info << "   <UniqueID>" << saveState.uniqueID                    << "</UniqueID>\n";
            break;
        case PLUGIN_DSSI:
            info << "   <Binary>"   << xmlSafeString(saveState.binary, true) << "</Binary>\n";
            info << "   <Label>"    << xmlSafeString(saveState.label, true)  << "</Label>\n";
            break;
        case PLUGIN_LV2:
            info << "   <URI>"      << xmlSafeString(saveState.label, true)  << "</URI>\n";
            break;
        case PLUGIN_VST:
            info << "   <Binary>"   << xmlSafeString(saveState.binary, true) << "</Binary>\n";
            info << "   <UniqueID>" << saveState.uniqueID                    << "</UniqueID>\n";
            break;
        case PLUGIN_AU:
            // TODO?
            info << "   <Binary>"   << xmlSafeString(saveState.binary, true) << "</Binary>\n";
            info << "   <UniqueID>" << saveState.uniqueID                    << "</UniqueID>\n";
            break;
        case PLUGIN_CSOUND:
        case PLUGIN_GIG:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
            info << "   <Filename>" << xmlSafeString(saveState.binary, true) << "</Filename>\n";
            info << "   <Label>"    << xmlSafeString(saveState.label, true)  << "</Label>\n";
            break;
        }

        info << "  </Info>\n\n";

        content << info;
    }

    {
        String data("  <Data>\n");

        data << "   <Active>" << (saveState.active ? "Yes" : "No") << "</Active>\n";

        if (saveState.dryWet != 1.0f)
            data << "   <DryWet>"        << saveState.dryWet       << "</DryWet>\n";
        if (saveState.volume != 1.0f)
            data << "   <Volume>"        << saveState.volume       << "</Volume>\n";
        if (saveState.balanceLeft != -1.0f)
            data << "   <Balance-Left>"  << saveState.balanceLeft  << "</Balance-Left>\n";
        if (saveState.balanceRight != 1.0f)
            data << "   <Balance-Right>" << saveState.balanceRight << "</Balance-Right>\n";
        if (saveState.panning != 0.0f)
            data << "   <Panning>"       << saveState.panning      << "</Panning>\n";

        if (saveState.ctrlChannel < 0)
            data << "   <ControlChannel>N</ControlChannel>\n";
        else
            data << "   <ControlChannel>" << saveState.ctrlChannel+1 << "</ControlChannel>\n";

        content << data;
    }

    for (StateParameterItenerator it = saveState.parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(*it);

        String parameter("\n""   <Parameter>\n");

        parameter << "    <Index>" << (long)stateParameter->index               << "</Index>\n"; // FIXME
        parameter << "    <Name>"  << xmlSafeString(stateParameter->name, true) << "</Name>\n";

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameter << "    <Symbol>" << xmlSafeString(stateParameter->symbol, true) << "</Symbol>\n";

        parameter << "    <Value>" << stateParameter->value << "</Value>\n";

        if (stateParameter->midiCC > 0)
        {
            parameter << "    <MidiCC>"      << stateParameter->midiCC        << "</MidiCC>\n";
            parameter << "    <MidiChannel>" << stateParameter->midiChannel+1 << "</MidiChannel>\n";
        }

        parameter << "   </Parameter>\n";

        content << parameter;
    }

    if (saveState.currentProgramIndex >= 0 && saveState.currentProgramName != nullptr)
    {
        // ignore 'default' program
#ifdef __USE_GNU
        if ((saveState.currentProgramIndex > 0 || strcasecmp(saveState.currentProgramName, "default") != 0))
#else
        if ((saveState.currentProgramIndex > 0 || std::strcmp(saveState.currentProgramName, "Default") != 0))
#endif
        {
            String program("\n");
            program << "   <CurrentProgramIndex>" << saveState.currentProgramIndex+1                   << "</CurrentProgramIndex>\n";
            program << "   <CurrentProgramName>"  << xmlSafeString(saveState.currentProgramName, true) << "</CurrentProgramName>\n";

            content << program;
        }
    }

    if (saveState.currentMidiBank >= 0 && saveState.currentMidiProgram >= 0)
    {
        String midiProgram("\n");
        midiProgram << "   <CurrentMidiBank>"    << saveState.currentMidiBank+1    << "</CurrentMidiBank>\n";
        midiProgram << "   <CurrentMidiProgram>" << saveState.currentMidiProgram+1 << "</CurrentMidiProgram>\n";

        content << midiProgram;
    }

    for (StateCustomDataItenerator it = saveState.customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(*it);

        String customData("\n""   <CustomData>\n");
        customData << "    <Type>" << xmlSafeString(stateCustomData->type, true) << "</Type>\n";
        customData << "    <Key>"  << xmlSafeString(stateCustomData->key, true)  << "</Key>\n";

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
            customData << "    <Value>\n" << xmlSafeString(stateCustomData->value, true) << "\n    </Value>\n";
        else
            customData << "    <Value>"   << xmlSafeString(stateCustomData->value, true) << "</Value>\n";

        customData << "   </CustomData>\n";

        content << customData;
    }

    if (saveState.chunk != nullptr && saveState.chunk[0] != '\0')
    {
        String chunk("\n""   <Chunk>\n");
        chunk << saveState.chunk << "\n   </Chunk>\n";

        content << chunk;
    }

    content << "  </Data>\n";
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_STATE_UTILS_HPP_INCLUDED
