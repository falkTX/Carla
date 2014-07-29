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

#include "CarlaStateUtils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

#include "juce_core.h"
using juce::String;
using juce::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// getNewLineSplittedString

static String getNewLineSplittedString(const String& string)
{
    static const int kLineWidth = 120;

    int i=0;
    const int length=string.length();

    String newString;
    newString.preallocateBytes(static_cast<size_t>(length + length/120 + 2));

    for (; i+kLineWidth < length; i += kLineWidth)
    {
        newString += string.substring(i, i+kLineWidth);
        newString += "\n";
    }

    newString += string.substring(i);

    return newString;
}

// -----------------------------------------------------------------------
// xmlSafeString

static String xmlSafeString(const String& string, const bool toXml)
{
    String newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}

// -----------------------------------------------------------------------
// xmlSafeStringCharDup

static const char* xmlSafeStringCharDup(const String& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toRawUTF8());
}

// -----------------------------------------------------------------------
// StateParameter

StateParameter::StateParameter() noexcept
    : isInput(true),
      index(-1),
      name(nullptr),
      symbol(nullptr),
#ifndef BUILD_BRIDGE
      value(0.0f),
      midiChannel(0),
      midiCC(-1) {}
#else
      value(0.0f) {}
#endif

StateParameter::~StateParameter() noexcept
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

// -----------------------------------------------------------------------
// StateCustomData

StateCustomData::StateCustomData() noexcept
    : type(nullptr),
      key(nullptr),
      value(nullptr) {}

StateCustomData::~StateCustomData() noexcept
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

// -----------------------------------------------------------------------
// StateSave

StateSave::StateSave() noexcept
    : type(nullptr),
      name(nullptr),
      label(nullptr),
      binary(nullptr),
      uniqueId(0),
#ifndef BUILD_BRIDGE
      active(false),
      dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f),
      ctrlChannel(-1),
      options(0x0),
#endif
      currentProgramIndex(-1),
      currentProgramName(nullptr),
      currentMidiBank(-1),
      currentMidiProgram(-1),
      chunk(nullptr),
      parameters(),
      customData() {}

StateSave::~StateSave() noexcept
{
    clear();
}

void StateSave::clear() noexcept
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

    uniqueId = 0;
#ifndef BUILD_BRIDGE
    active = false;
    dryWet = 1.0f;
    volume = 1.0f;
    balanceLeft  = -1.0f;
    balanceRight = 1.0f;
    panning      = 0.0f;
    ctrlChannel  = -1;
    options      = 0x0;
#endif
    currentProgramIndex = -1;
    currentMidiBank     = -1;
    currentMidiProgram  = -1;

    for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());
        delete stateParameter;
    }

    for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        delete stateCustomData;
    }

    parameters.clear();
    customData.clear();
}

// -----------------------------------------------------------------------
// fillFromXmlElement

bool StateSave::fillFromXmlElement(const XmlElement* const xmlElement)
{
    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr, false);

    clear();

    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        const String& tagName(elem->getTagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName.equalsIgnoreCase("info"))
        {
            for (XmlElement* xmlInfo = elem->getFirstChildElement(); xmlInfo != nullptr; xmlInfo = xmlInfo->getNextElement())
            {
                const String& tag(xmlInfo->getTagName());
                const String  text(xmlInfo->getAllSubText().trim());

                if (tag.equalsIgnoreCase("type"))
                    type = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("name"))
                    name = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("label") || tag.equalsIgnoreCase("uri"))
                    label = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("binary") || tag.equalsIgnoreCase("bundle") || tag.equalsIgnoreCase("filename"))
                    binary = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("uniqueid"))
                    uniqueId = text.getLargeIntValue();
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (tagName.equalsIgnoreCase("data"))
        {
            for (XmlElement* xmlData = elem->getFirstChildElement(); xmlData != nullptr; xmlData = xmlData->getNextElement())
            {
                const String& tag(xmlData->getTagName());
                const String  text(xmlData->getAllSubText().trim());

#ifndef BUILD_BRIDGE
                // -------------------------------------------------------
                // Internal Data

                if (tag.equalsIgnoreCase("active"))
                {
                    active = (text.equalsIgnoreCase("yes") || text.equalsIgnoreCase("true"));
                }
                else if (tag.equalsIgnoreCase("drywet"))
                {
                    dryWet = carla_fixValue(0.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("volume"))
                {
                    volume = carla_fixValue(0.0f, 1.27f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceleft") || tag.equalsIgnoreCase("balance-left"))
                {
                    balanceLeft = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceright") || tag.equalsIgnoreCase("balance-right"))
                {
                    balanceRight = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("panning"))
                {
                    panning = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("controlchannel") || tag.equalsIgnoreCase("control-channel"))
                {
                    if (! text.startsWithIgnoreCase("n"))
                    {
                        const int value(text.getIntValue());
                        if (value >= 1 && value <= MAX_MIDI_CHANNELS)
                            ctrlChannel = static_cast<int8_t>(value-1);
                    }
                }
                else if (tag.equalsIgnoreCase("options"))
                {
                    const int value(text.getHexValue32());
                    if (value > 0)
                        options = static_cast<uint>(value);
                }
#else
                if (false) {}
#endif

                // -------------------------------------------------------
                // Program (current)

                else if (tag.equalsIgnoreCase("currentprogramindex") || tag.equalsIgnoreCase("current-program-index"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentProgramIndex = value-1;
                }
                else if (tag.equalsIgnoreCase("currentprogramname") || tag.equalsIgnoreCase("current-program-name"))
                {
                    currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.equalsIgnoreCase("currentmidibank") || tag.equalsIgnoreCase("current-midi-bank"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiBank = value-1;
                }
                else if (tag.equalsIgnoreCase("currentmidiprogram") || tag.equalsIgnoreCase("current-midi-program"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiProgram = value-1;
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

#ifndef BUILD_BRIDGE
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
#endif
                    }

                    parameters.append(stateParameter);
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
                            stateCustomData->value = carla_strdup(cText.toRawUTF8()); //xmlSafeStringCharDup(cText, false);
                    }

                    customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.equalsIgnoreCase("chunk"))
                {
                    const String nText(text.replace("\n", ""));
                    chunk = xmlSafeStringCharDup(nText, false);
                }
            }
        }
    }

    return true;
}

// -----------------------------------------------------------------------
// fillXmlStringFromStateSave

String StateSave::toString() const
{
    String content;

    {
        String infoXml("  <Info>\n");

        infoXml << "   <Type>" << String(type != nullptr ? type : "") << "</Type>\n";
        infoXml << "   <Name>" << xmlSafeString(name, true) << "</Name>\n";

        switch (getPluginTypeFromString(type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            infoXml << "   <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_LADSPA:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            infoXml << "   <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_DSSI:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_LV2:
            infoXml << "   <Bundle>"   << xmlSafeString(binary, true) << "</Bundle>\n";
            infoXml << "   <URI>"      << xmlSafeString(label, true)  << "</URI>\n";
            break;
        case PLUGIN_VST:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_VST3:
            // TODO?
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_AU:
            // TODO?
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <UniqueID>" << uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_GIG:
        case PLUGIN_SF2:
            infoXml << "   <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            infoXml << "   <Label>"      << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_SFZ:
            infoXml << "   <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            break;
        }

        infoXml << "  </Info>\n\n";

        content << infoXml;
    }

    content << "  <Data>\n";

#ifndef BUILD_BRIDGE
    {
        String dataXml;

        dataXml << "   <Active>" << (active ? "Yes" : "No") << "</Active>\n";

        if (dryWet != 1.0f)
            dataXml << "   <DryWet>"        << String(dryWet, 7)       << "</DryWet>\n";
        if (volume != 1.0f)
            dataXml << "   <Volume>"        << String(volume, 7)       << "</Volume>\n";
        if (balanceLeft != -1.0f)
            dataXml << "   <Balance-Left>"  << String(balanceLeft, 7)  << "</Balance-Left>\n";
        if (balanceRight != 1.0f)
            dataXml << "   <Balance-Right>" << String(balanceRight, 7) << "</Balance-Right>\n";
        if (panning != 0.0f)
            dataXml << "   <Panning>"       << String(panning, 7)      << "</Panning>\n";

        if (ctrlChannel < 0)
            dataXml << "   <ControlChannel>N</ControlChannel>\n";
        else
            dataXml << "   <ControlChannel>" << int(ctrlChannel+1) << "</ControlChannel>\n";

        dataXml << "   <Options>0x" << String::toHexString(static_cast<int>(options)) << "</Options>\n";

        content << dataXml;
    }
#endif

    for (StateParameterItenerator it = parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        String parameterXml("\n""   <Parameter>\n");

        parameterXml << "    <Index>" << String(stateParameter->index)             << "</Index>\n";
        parameterXml << "    <Name>"  << xmlSafeString(stateParameter->name, true) << "</Name>\n";

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameterXml << "    <Symbol>" << xmlSafeString(stateParameter->symbol, true) << "</Symbol>\n";

        if (stateParameter->isInput)
            parameterXml << "    <Value>" << String(stateParameter->value, 15) << "</Value>\n";

#ifndef BUILD_BRIDGE
        if (stateParameter->midiCC > 0)
        {
            parameterXml << "    <MidiCC>"      << stateParameter->midiCC        << "</MidiCC>\n";
            parameterXml << "    <MidiChannel>" << stateParameter->midiChannel+1 << "</MidiChannel>\n";
        }
#endif

        parameterXml << "   </Parameter>\n";

        content << parameterXml;
    }

    if (currentProgramIndex >= 0 && currentProgramName != nullptr && currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (currentProgramIndex > 0 || ! String(currentProgramName).equalsIgnoreCase("default"))
        {
            String programXml("\n");
            programXml << "   <CurrentProgramIndex>" << currentProgramIndex+1                   << "</CurrentProgramIndex>\n";
            programXml << "   <CurrentProgramName>"  << xmlSafeString(currentProgramName, true) << "</CurrentProgramName>\n";

            content << programXml;
        }
    }

    if (currentMidiBank >= 0 && currentMidiProgram >= 0)
    {
        String midiProgramXml("\n");
        midiProgramXml << "   <CurrentMidiBank>"    << currentMidiBank+1    << "</CurrentMidiBank>\n";
        midiProgramXml << "   <CurrentMidiProgram>" << currentMidiProgram+1 << "</CurrentMidiProgram>\n";

        content << midiProgramXml;
    }

    for (StateCustomDataItenerator it = customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->type  != nullptr && stateCustomData->type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->key   != nullptr && stateCustomData->key[0]  != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->value != nullptr);

        String customDataXml("\n""   <CustomData>\n");
        customDataXml << "    <Type>" << xmlSafeString(stateCustomData->type, true) << "</Type>\n";
        customDataXml << "    <Key>"  << xmlSafeString(stateCustomData->key, true)  << "</Key>\n";

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customDataXml << "    <Value>\n";
            customDataXml << xmlSafeString(stateCustomData->value, true);
            customDataXml << "\n    </Value>\n";
        }
        else
        {
            customDataXml << "    <Value>";
            customDataXml << xmlSafeString(stateCustomData->value, true);
            customDataXml << "</Value>\n";
        }

        customDataXml << "   </CustomData>\n";

        content << customDataXml;
    }

    if (chunk != nullptr && chunk[0] != '\0')
    {
        String chunkXml("\n""   <Chunk>\n");
        chunkXml << getNewLineSplittedString(chunk) << "\n   </Chunk>\n";

        content << chunkXml;
    }

    content << "  </Data>\n";

    return content;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
