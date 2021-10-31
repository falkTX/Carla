/*
 * Carla State utils
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#include "water/streams/MemoryOutputStream.h"
#include "water/xml/XmlElement.h"

#include <string>

using water::MemoryOutputStream;
using water::String;
using water::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// getNewLineSplittedString

static void getNewLineSplittedString(MemoryOutputStream& stream, const String& string)
{
    static const int kLineWidth = 120;

    int i = 0;
    const int length = string.length();
    const char* const raw = string.toUTF8();

    stream.preallocate(static_cast<std::size_t>(length + length/kLineWidth + 3));

    for (; i+kLineWidth < length; i += kLineWidth)
    {
        stream.write(raw+i, kLineWidth);
        stream.writeByte('\n');
    }

    stream << (raw+i);
}

// -----------------------------------------------------------------------
// xmlSafeStringFast

/* Based on some code by James Kanze from stackoverflow
 * https://stackoverflow.com/questions/7724011/in-c-whats-the-fastest-way-to-replace-all-occurrences-of-a-substring-within */

static std::string replaceStdString(const std::string& original, const std::string& before, const std::string& after)
{
    std::string::const_iterator current = original.begin(), end = original.end(), next;
    std::string retval;

    for (; (next = std::search(current, end, before.begin(), before.end())) != end;)
    {
        retval.append(current, next);
        retval.append(after);
        current = next + static_cast<ssize_t>(before.size());
    }
    retval.append(current, next);
    return retval;
}

static std::string xmlSafeStringFast(const char* const cstring, const bool toXml)
{
    std::string string(cstring);

    if (toXml)
    {
        string = replaceStdString(string, "&","&amp;");
        string = replaceStdString(string, "<","&lt;");
        string = replaceStdString(string, ">","&gt;");
        string = replaceStdString(string, "'","&apos;");
        string = replaceStdString(string, "\"","&quot;");
    }
    else
    {
        string = replaceStdString(string, "&lt;","<");
        string = replaceStdString(string, "&gt;",">");
        string = replaceStdString(string, "&apos;","'");
        string = replaceStdString(string, "&quot;","\"");
        string = replaceStdString(string, "&amp;","&");
    }

    return string;
}

// -----------------------------------------------------------------------
// xmlSafeStringCharDup

/*
static const char* xmlSafeStringCharDup(const char* const cstring, const bool toXml)
{
    return carla_strdup(xmlSafeString(cstring, toXml).toRawUTF8());
}
*/

static const char* xmlSafeStringCharDup(const String& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toRawUTF8());
}

// -----------------------------------------------------------------------
// StateParameter

CarlaStateSave::Parameter::Parameter() noexcept
    : dummy(true),
      index(-1),
      name(nullptr),
      symbol(nullptr),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
      value(0.0f),
      mappedControlIndex(CONTROL_INDEX_NONE),
      midiChannel(0),
      mappedRangeValid(false),
      mappedMinimum(0.0f),
      mappedMaximum(1.0f) {}
#else
      value(0.0f) {}
#endif

CarlaStateSave::Parameter::~Parameter() noexcept
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

CarlaStateSave::CustomData::CustomData() noexcept
    : type(nullptr),
      key(nullptr),
      value(nullptr) {}

CarlaStateSave::CustomData::~CustomData() noexcept
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

bool CarlaStateSave::CustomData::isValid() const noexcept
{
    if (type  == nullptr || type[0] == '\0') return false;
    if (key   == nullptr || key [0] == '\0') return false;
    if (value == nullptr)                    return false;
    return true;
}

// -----------------------------------------------------------------------
// StateSave

CarlaStateSave::CarlaStateSave() noexcept
    : type(nullptr),
      name(nullptr),
      label(nullptr),
      binary(nullptr),
      uniqueId(0),
      options(0x0),
      temporary(false),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
      active(false),
      dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f),
      ctrlChannel(-1),
#endif
      currentProgramIndex(-1),
      currentProgramName(nullptr),
      currentMidiBank(-1),
      currentMidiProgram(-1),
      chunk(nullptr),
      parameters(),
      customData() {}

CarlaStateSave::~CarlaStateSave() noexcept
{
    clear();
}

void CarlaStateSave::clear() noexcept
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
    options  = 0x0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    active = false;
    dryWet = 1.0f;
    volume = 1.0f;
    balanceLeft  = -1.0f;
    balanceRight = 1.0f;
    panning      = 0.0f;
    ctrlChannel  = -1;
#endif

    currentProgramIndex = -1;
    currentMidiBank     = -1;
    currentMidiProgram  = -1;

    for (ParameterItenerator it = parameters.begin2(); it.valid(); it.next())
    {
        Parameter* const stateParameter(it.getValue(nullptr));
        delete stateParameter;
    }

    for (CustomDataItenerator it = customData.begin2(); it.valid(); it.next())
    {
        CustomData* const stateCustomData(it.getValue(nullptr));
        delete stateCustomData;
    }

    parameters.clear();
    customData.clear();
}

// -----------------------------------------------------------------------
// fillFromXmlElement

bool CarlaStateSave::fillFromXmlElement(const XmlElement* const xmlElement)
{
    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr, false);

    clear();

    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        const String& tagName(elem->getTagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName == "Info")
        {
            for (XmlElement* xmlInfo = elem->getFirstChildElement(); xmlInfo != nullptr; xmlInfo = xmlInfo->getNextElement())
            {
                const String& tag(xmlInfo->getTagName());
                const String  text(xmlInfo->getAllSubText().trim());

                /**/ if (tag == "Type")
                    type = xmlSafeStringCharDup(text, false);
                else if (tag == "Name")
                    name = xmlSafeStringCharDup(text, false);
                else if (tag == "Label" || tag == "URI" || tag == "Identifier" || tag == "Setup")
                    label = xmlSafeStringCharDup(text, false);
                else if (tag == "Binary" || tag == "Filename")
                    binary = xmlSafeStringCharDup(text, false);
                else if (tag == "UniqueID")
                    uniqueId = text.getLargeIntValue();
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (tagName == "Data")
        {
            for (XmlElement* xmlData = elem->getFirstChildElement(); xmlData != nullptr; xmlData = xmlData->getNextElement())
            {
                const String& tag(xmlData->getTagName());
                const String  text(xmlData->getAllSubText().trim());

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                // -------------------------------------------------------
                // Internal Data

                /**/ if (tag == "Active")
                {
                    active = (text == "Yes");
                }
                else if (tag == "DryWet")
                {
                    dryWet = carla_fixedValue(0.0f, 1.0f, text.getFloatValue());
                }
                else if (tag == "Volume")
                {
                    volume = carla_fixedValue(0.0f, 1.27f, text.getFloatValue());
                }
                else if (tag == "Balance-Left")
                {
                    balanceLeft = carla_fixedValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag == "Balance-Right")
                {
                    balanceRight = carla_fixedValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag == "Panning")
                {
                    panning = carla_fixedValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag == "ControlChannel")
                {
                    if (! text.startsWithIgnoreCase("n"))
                    {
                        const int value(text.getIntValue());
                        if (value >= 1 && value <= MAX_MIDI_CHANNELS)
                            ctrlChannel = static_cast<int8_t>(value-1);
                    }
                }
                else if (tag == "Options")
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

                else if (tag == "CurrentProgramIndex")
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentProgramIndex = value-1;
                }
                else if (tag == "CurrentProgramName")
                {
                    currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag == "CurrentMidiBank")
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiBank = value-1;
                }
                else if (tag == "CurrentMidiProgram")
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        currentMidiProgram = value-1;
                }

                // -------------------------------------------------------
                // Parameters

                else if (tag == "Parameter")
                {
                    Parameter* const stateParameter(new Parameter());
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    bool hasMappedMinimum = false, hasMappedMaximum = false;
#endif

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& pTag(xmlSubData->getTagName());
                        const String  pText(xmlSubData->getAllSubText().trim());

                        /**/ if (pTag == "Index")
                        {
                            const int index(pText.getIntValue());
                            if (index >= 0)
                                stateParameter->index = index;
                        }
                        else if (pTag == "Name")
                        {
                            stateParameter->name = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag == "Symbol")
                        {
                            stateParameter->symbol = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag == "Value")
                        {
                            stateParameter->dummy = false;
                            stateParameter->value = pText.getFloatValue();
                        }
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        else if (pTag == "MidiChannel")
                        {
                            const int channel(pText.getIntValue());
                            if (channel >= 1 && channel <= MAX_MIDI_CHANNELS)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel-1);
                        }
                        else if (pTag == "MidiCC")
                        {
                            const int cc(pText.getIntValue());
                            if (cc > 0 && cc < MAX_MIDI_CONTROL)
                                stateParameter->mappedControlIndex = static_cast<int16_t>(cc);
                        }
                        else if (pTag == "MappedControlIndex")
                        {
                            const int ctrl(pText.getIntValue());
                            if (ctrl > CONTROL_INDEX_NONE && ctrl <= CONTROL_INDEX_MAX_ALLOWED)
                                if (ctrl != CONTROL_INDEX_MIDI_LEARN)
                                    stateParameter->mappedControlIndex = static_cast<int16_t>(ctrl);
                        }
                        else if (pTag == "MappedMinimum")
                        {
                            hasMappedMinimum = true;
                            stateParameter->mappedMinimum = pText.getFloatValue();
                        }
                        else if (pTag == "MappedMaximum")
                        {
                            hasMappedMaximum = true;
                            stateParameter->mappedMaximum = pText.getFloatValue();
                        }
#endif
                    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    if (hasMappedMinimum && hasMappedMaximum)
                        stateParameter->mappedRangeValid = true;
#endif

                    parameters.append(stateParameter);
                }

                // -------------------------------------------------------
                // Custom Data

                else if (tag == "CustomData")
                {
                    CustomData* const stateCustomData(new CustomData());

                    for (XmlElement* xmlSubData = xmlData->getFirstChildElement(); xmlSubData != nullptr; xmlSubData = xmlSubData->getNextElement())
                    {
                        const String& cTag(xmlSubData->getTagName());
                        const String  cText(xmlSubData->getAllSubText().trim());

                        /**/ if (cTag == "Type")
                            stateCustomData->type = xmlSafeStringCharDup(cText, false);
                        else if (cTag == "Key")
                            stateCustomData->key = xmlSafeStringCharDup(cText, false);
                        else if (cTag == "Value")
                            stateCustomData->value = carla_strdup(cText.toRawUTF8()); //xmlSafeStringCharDup(cText, false);
                    }

                    if (stateCustomData->isValid())
                        customData.append(stateCustomData);
                    else
                        carla_stderr("Reading CustomData property failed, missing data");
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag == "Chunk")
                {
                    chunk = carla_strdup(text.toRawUTF8());
                }
            }
        }
    }

    return true;
}

// -----------------------------------------------------------------------
// fillXmlStringFromStateSave

void CarlaStateSave::dumpToMemoryStream(MemoryOutputStream& content) const
{
    {
        MemoryOutputStream infoXml;

        infoXml << "  <Info>\n";
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
            infoXml << "   <UniqueID>" << water::int64(uniqueId)       << "</UniqueID>\n";
            break;
        case PLUGIN_DSSI:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_LV2:
            infoXml << "   <URI>"      << xmlSafeString(label, true)  << "</URI>\n";
            break;
        case PLUGIN_VST2:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <UniqueID>" << water::int64(uniqueId)       << "</UniqueID>\n";
            break;
        case PLUGIN_VST3:
            infoXml << "   <Binary>"   << xmlSafeString(binary, true) << "</Binary>\n";
            infoXml << "   <Label>"    << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_AU:
            infoXml << "   <Identifier>" << xmlSafeString(label, true) << "</Identifier>\n";
            break;
        case PLUGIN_JSFX:
        case PLUGIN_DLS:
        case PLUGIN_GIG:
        case PLUGIN_SF2:
            infoXml << "   <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            infoXml << "   <Label>"      << xmlSafeString(label, true)  << "</Label>\n";
            break;
        case PLUGIN_SFZ:
            infoXml << "   <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            break;
        case PLUGIN_JACK:
            infoXml << "   <Filename>"   << xmlSafeString(binary, true) << "</Filename>\n";
            infoXml << "   <Setup>"      << xmlSafeString(label, true)  << "</Setup>\n";
            break;
        }

        infoXml << "  </Info>\n\n";

        content << infoXml;
    }

    content << "  <Data>\n";

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    {
        MemoryOutputStream dataXml;

        dataXml << "   <Active>" << (active ? "Yes" : "No") << "</Active>\n";

        if (carla_isNotEqual(dryWet, 1.0f))
            dataXml << "   <DryWet>"        << String(dryWet, 7)       << "</DryWet>\n";
        if (carla_isNotEqual(volume, 1.0f))
            dataXml << "   <Volume>"        << String(volume, 7)       << "</Volume>\n";
        if (carla_isNotEqual(balanceLeft, -1.0f))
            dataXml << "   <Balance-Left>"  << String(balanceLeft, 7)  << "</Balance-Left>\n";
        if (carla_isNotEqual(balanceRight, 1.0f))
            dataXml << "   <Balance-Right>" << String(balanceRight, 7) << "</Balance-Right>\n";
        if (carla_isNotEqual(panning, 0.0f))
            dataXml << "   <Panning>"       << String(panning, 7)      << "</Panning>\n";

        if (ctrlChannel < 0)
            dataXml << "   <ControlChannel>N</ControlChannel>\n";
        else
            dataXml << "   <ControlChannel>" << int(ctrlChannel+1) << "</ControlChannel>\n";

        dataXml << "   <Options>0x" << String::toHexString(static_cast<int>(options)) << "</Options>\n";

        content << dataXml;
    }
#endif

    for (ParameterItenerator it = parameters.begin2(); it.valid(); it.next())
    {
        Parameter* const stateParameter(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(stateParameter != nullptr);

        MemoryOutputStream parameterXml;

        parameterXml << "\n";
        parameterXml << "   <Parameter>\n";
        parameterXml << "    <Index>" << String(stateParameter->index)             << "</Index>\n";
        parameterXml << "    <Name>"  << xmlSafeString(stateParameter->name, true) << "</Name>\n";

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameterXml << "    <Symbol>" << xmlSafeString(stateParameter->symbol, true) << "</Symbol>\n";

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (stateParameter->mappedControlIndex > CONTROL_INDEX_NONE && stateParameter->mappedControlIndex <= CONTROL_INDEX_MAX_ALLOWED)
        {
            parameterXml << "    <MidiChannel>"   << stateParameter->midiChannel+1 << "</MidiChannel>\n";
            parameterXml << "    <MappedControlIndex>" << stateParameter->mappedControlIndex << "</MappedControlIndex>\n";

            if (stateParameter->mappedRangeValid)
            {
                parameterXml << "    <MappedMinimum>" << String(stateParameter->mappedMinimum, 15) << "</MappedMinimum>\n";
                parameterXml << "    <MappedMaximum>" << String(stateParameter->mappedMaximum, 15) << "</MappedMaximum>\n";
            }

            // backwards compatibility for older carla versions
            if (stateParameter->mappedControlIndex > 0 && stateParameter->mappedControlIndex < MAX_MIDI_CONTROL)
                parameterXml << "    <MidiCC>" << stateParameter->mappedControlIndex << "</MidiCC>\n";
        }
#endif

        if (! stateParameter->dummy)
            parameterXml << "    <Value>" << String(stateParameter->value, 15) << "</Value>\n";

        parameterXml << "   </Parameter>\n";

        content << parameterXml;
    }

    if (currentProgramIndex >= 0 && currentProgramName != nullptr && currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (currentProgramIndex > 0 || ! String(currentProgramName).equalsIgnoreCase("default"))
        {
            MemoryOutputStream programXml;

            programXml << "\n";
            programXml << "   <CurrentProgramIndex>" << currentProgramIndex+1                   << "</CurrentProgramIndex>\n";
            programXml << "   <CurrentProgramName>"  << xmlSafeString(currentProgramName, true) << "</CurrentProgramName>\n";

            content << programXml;
        }
    }

    if (currentMidiBank >= 0 && currentMidiProgram >= 0)
    {
        MemoryOutputStream midiProgramXml;

        midiProgramXml << "\n";
        midiProgramXml << "   <CurrentMidiBank>"    << currentMidiBank+1    << "</CurrentMidiBank>\n";
        midiProgramXml << "   <CurrentMidiProgram>" << currentMidiProgram+1 << "</CurrentMidiProgram>\n";

        content << midiProgramXml;
    }

    for (CustomDataItenerator it = customData.begin2(); it.valid(); it.next())
    {
        CustomData* const stateCustomData(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData != nullptr);
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->isValid());

        MemoryOutputStream customDataXml;

        customDataXml << "\n";
        customDataXml << "   <CustomData>\n";
        customDataXml << "    <Type>" << xmlSafeString(stateCustomData->type, true) << "</Type>\n";
        customDataXml << "    <Key>"  << xmlSafeString(stateCustomData->key, true)  << "</Key>\n";

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customDataXml << "    <Value>\n";
            customDataXml << xmlSafeStringFast(stateCustomData->value, true);
            customDataXml << "\n    </Value>\n";
        }
        else
        {
            customDataXml << "    <Value>";
            customDataXml << xmlSafeStringFast(stateCustomData->value, true);
            customDataXml << "</Value>\n";
        }

        customDataXml << "   </CustomData>\n";

        content << customDataXml;
    }

    if (chunk != nullptr && chunk[0] != '\0')
    {
        MemoryOutputStream chunkXml, chunkSplt;
        getNewLineSplittedString(chunkSplt, chunk);

        chunkXml << "\n   <Chunk>\n";
        chunkXml << chunkSplt;
        chunkXml << "\n   </Chunk>\n";

        content << chunkXml;
    }

    content << "  </Data>\n";
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
