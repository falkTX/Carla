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

#ifdef HAVE_JUCE_LATER
# include "juce_core.h"
using juce::String;
using juce::XmlElement;
#else
# include <QtCore/QString>
# include <QtXml/QDomNode>
#endif

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// StateParameter

StateParameter::StateParameter() noexcept
    : isInput(true),
      index(-1),
      name(nullptr),
      symbol(nullptr),
      value(0.0f),
      midiChannel(0),
      midiCC(-1) {}

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

StateSave::~StateSave() noexcept
{
    reset();
}

void StateSave::reset() noexcept
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
// xmlSafeString

#ifdef HAVE_JUCE_LATER
static String xmlSafeString(const String& string, const bool toXml)
{
    String newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}
#else
static QString xmlSafeString(const QString& string, const bool toXml)
{
    QString newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"").replace("&amp;","&");
}
#endif

// -----------------------------------------------------------------------
// xmlSafeStringCharDup

#ifdef HAVE_JUCE_LATER
static const char* xmlSafeStringCharDup(const String& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toRawUTF8());
}
#else
static const char* xmlSafeStringCharDup(const QString& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toUtf8().constData());
}
#endif

// -----------------------------------------------------------------------
// fillStateSaveFromXmlNode

#ifdef HAVE_JUCE_LATER
void fillStateSaveFromXmlElement(StateSave& stateSave, const XmlElement* const xmlElement)
{
    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr,);

    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        String tagName(elem->getTagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName.equalsIgnoreCase("info"))
        {
            for (XmlElement* xmlInfo = elem->getFirstChildElement(); xmlInfo != nullptr; xmlInfo = xmlInfo->getNextElement())
            {
                const String& tag(xmlInfo->getTagName());
                const String  text(xmlInfo->getAllSubText().trim());

                if (tag.equalsIgnoreCase("type"))
                    stateSave.type = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("name"))
                    stateSave.name = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("label") || tag.equalsIgnoreCase("uri"))
                    stateSave.label = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("binary") || tag.equalsIgnoreCase("bundle") || tag.equalsIgnoreCase("filename"))
                    stateSave.binary = xmlSafeStringCharDup(text, false);
                else if (tag.equalsIgnoreCase("uniqueid"))
                    stateSave.uniqueId = text.getLargeIntValue();
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

                // -------------------------------------------------------
                // Internal Data

                if (tag.equalsIgnoreCase("active"))
                {
                    stateSave.active = (text.equalsIgnoreCase("yes") || text.equalsIgnoreCase("true"));
                }
                else if (tag.equalsIgnoreCase("drywet"))
                {
                    stateSave.dryWet = carla_fixValue(0.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("volume"))
                {
                    stateSave.volume = carla_fixValue(0.0f, 1.27f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceleft") || tag.equalsIgnoreCase("balance-left"))
                {
                    stateSave.balanceLeft = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("balanceright") || tag.equalsIgnoreCase("balance-right"))
                {
                    stateSave.balanceRight = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("panning"))
                {
                    stateSave.panning = carla_fixValue(-1.0f, 1.0f, text.getFloatValue());
                }
                else if (tag.equalsIgnoreCase("controlchannel") || tag.equalsIgnoreCase("control-channel"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1 && value <= MAX_MIDI_CHANNELS)
                        stateSave.ctrlChannel = static_cast<int8_t>(value-1);
                }

                // -------------------------------------------------------
                // Program (current)

                else if (tag.equalsIgnoreCase("currentprogramindex") || tag.equalsIgnoreCase("current-program-index"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        stateSave.currentProgramIndex = value-1;
                }
                else if (tag.equalsIgnoreCase("currentprogramname") || tag.equalsIgnoreCase("current-program-name"))
                {
                    stateSave.currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.equalsIgnoreCase("currentmidibank") || tag.equalsIgnoreCase("current-midi-bank"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        stateSave.currentMidiBank = value-1;
                }
                else if (tag.equalsIgnoreCase("currentmidiprogram") || tag.equalsIgnoreCase("current-midi-program"))
                {
                    const int value(text.getIntValue());
                    if (value >= 1)
                        stateSave.currentMidiProgram = value-1;
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

                    stateSave.parameters.append(stateParameter);
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

                    stateSave.customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.equalsIgnoreCase("chunk"))
                {
                    stateSave.chunk = xmlSafeStringCharDup(text, false);
                }
            }
        }
    }
}
#else
void fillStateSaveFromXmlNode(StateSave& stateSave, const QDomNode& xmlNode)
{
    CARLA_SAFE_ASSERT_RETURN(! xmlNode.isNull(),);

    for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
    {
        QString tagName(node.toElement().tagName());

        // ---------------------------------------------------------------
        // Info

        if (tagName.compare("info", Qt::CaseInsensitive) == 0)
        {
            for (QDomNode xmlInfo = node.toElement().firstChild(); ! xmlInfo.isNull(); xmlInfo = xmlInfo.nextSibling())
            {
                const QString tag(xmlInfo.toElement().tagName());
                const QString text(xmlInfo.toElement().text().trimmed());

                if (tag.compare("type", Qt::CaseInsensitive) == 0)
                {
                    stateSave.type = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("name", Qt::CaseInsensitive) == 0)
                {
                    stateSave.name = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("label", Qt::CaseInsensitive) == 0 || tag.compare("uri", Qt::CaseInsensitive) == 0)
                {
                    stateSave.label = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("binary", Qt::CaseInsensitive) == 0 || tag.compare("bundle", Qt::CaseInsensitive) == 0 || tag.compare("filename", Qt::CaseInsensitive) == 0)
                {
                    stateSave.binary = xmlSafeStringCharDup(text, false);
                }
                else if (tag.compare("uniqueid", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const qlonglong uniqueId(text.toLongLong(&ok));
                    if (ok) stateSave.uniqueId = static_cast<int64_t>(uniqueId);
                }
            }
        }

        // ---------------------------------------------------------------
        // Data

        else if (tagName.compare("data", Qt::CaseInsensitive) == 0)
        {
            for (QDomNode xmlData = node.toElement().firstChild(); ! xmlData.isNull(); xmlData = xmlData.nextSibling())
            {
                const QString tag(xmlData.toElement().tagName());
                const QString text(xmlData.toElement().text().trimmed());

                // -------------------------------------------------------
                // Internal Data

                if (tag.compare("active", Qt::CaseInsensitive) == 0)
                {
                    stateSave.active = (text.compare("yes", Qt::CaseInsensitive) == 0 || text.compare("true", Qt::CaseInsensitive) == 0);
                }
                else if (tag.compare("drywet", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) stateSave.dryWet = carla_fixValue(0.0f, 1.0f, value);
                }
                else if (tag.compare("volume", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) stateSave.volume = carla_fixValue(0.0f, 1.27f, value);
                }
                else if (tag.compare("balanceleft", Qt::CaseInsensitive) == 0 || tag.compare("balance-left", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) stateSave.balanceLeft = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("balanceright", Qt::CaseInsensitive) == 0 || tag.compare("balance-right", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) stateSave.balanceRight = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("panning", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const float value(text.toFloat(&ok));
                    if (ok) stateSave.panning = carla_fixValue(-1.0f, 1.0f, value);
                }
                else if (tag.compare("controlchannel", Qt::CaseInsensitive) == 0 || tag.compare("control-channel", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const short value(text.toShort(&ok));
                    if (ok && value >= 1 && value <= MAX_MIDI_CHANNELS)
                        stateSave.ctrlChannel = static_cast<int8_t>(value-1);
                }

                // -------------------------------------------------------
                // Program (current)

                else if (tag.compare("currentprogramindex", Qt::CaseInsensitive) == 0 || tag.compare("current-program-index", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        stateSave.currentProgramIndex = value-1;
                }
                else if (tag.compare("currentprogramname", Qt::CaseInsensitive) == 0 || tag.compare("current-program-name", Qt::CaseInsensitive) == 0)
                {
                    stateSave.currentProgramName = xmlSafeStringCharDup(text, false);
                }

                // -------------------------------------------------------
                // Midi Program (current)

                else if (tag.compare("currentmidibank", Qt::CaseInsensitive) == 0 || tag.compare("current-midi-bank", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        stateSave.currentMidiBank = value-1;
                }
                else if (tag.compare("currentmidiprogram", Qt::CaseInsensitive) == 0 || tag.compare("current-midi-program", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    const int value(text.toInt(&ok));
                    if (ok && value >= 1)
                        stateSave.currentMidiProgram = value-1;
                }

                // -------------------------------------------------------
                // Parameters

                else if (tag.compare("parameter", Qt::CaseInsensitive) == 0)
                {
                    StateParameter* const stateParameter(new StateParameter());

                    for (QDomNode xmlSubData = xmlData.toElement().firstChild(); ! xmlSubData.isNull(); xmlSubData = xmlSubData.nextSibling())
                    {
                        const QString pTag(xmlSubData.toElement().tagName());
                        const QString pText(xmlSubData.toElement().text().trimmed());

                        if (pTag.compare("index", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const int index(pText.toInt(&ok));
                            if (ok && index >= 0) stateParameter->index = index;
                        }
                        else if (pTag.compare("name", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->name = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.compare("symbol", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->symbol = xmlSafeStringCharDup(pText, false);
                        }
                        else if (pTag.compare("value", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const float value(pText.toFloat(&ok));
                            if (ok) stateParameter->value = value;
                        }
                        else if (pTag.compare("midichannel", Qt::CaseInsensitive) == 0 || pTag.compare("midi-channel", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const ushort channel(pText.toUShort(&ok));
                            if (ok && channel >= 1 && channel <= MAX_MIDI_CHANNELS)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel-1);
                        }
                        else if (pTag.compare("midicc", Qt::CaseInsensitive) == 0 || pTag.compare("midi-cc", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            const int cc(pText.toInt(&ok));
                            if (ok && cc >= 1 && cc < 0x5F)
                                stateParameter->midiCC = static_cast<int16_t>(cc);
                        }
                    }

                    stateSave.parameters.append(stateParameter);
                }

                // -------------------------------------------------------
                // Custom Data

                else if (tag.compare("customdata", Qt::CaseInsensitive) == 0 || tag.compare("custom-data", Qt::CaseInsensitive) == 0)
                {
                    StateCustomData* const stateCustomData(new StateCustomData());

                    for (QDomNode xmlSubData = xmlData.toElement().firstChild(); ! xmlSubData.isNull(); xmlSubData = xmlSubData.nextSibling())
                    {
                        const QString cTag(xmlSubData.toElement().tagName());
                        const QString cText(xmlSubData.toElement().text().trimmed());

                        if (cTag.compare("type", Qt::CaseInsensitive) == 0)
                            stateCustomData->type = xmlSafeStringCharDup(cText, false);
                        else if (cTag.compare("key", Qt::CaseInsensitive) == 0)
                            stateCustomData->key = xmlSafeStringCharDup(cText, false);
                        else if (cTag.compare("value", Qt::CaseInsensitive) == 0)
                            stateCustomData->value = xmlSafeStringCharDup(cText, false);
                    }

                    stateSave.customData.append(stateCustomData);
                }

                // -------------------------------------------------------
                // Chunk

                else if (tag.compare("chunk", Qt::CaseInsensitive) == 0)
                {
                    stateSave.chunk = xmlSafeStringCharDup(text, false);
                }
            }
        }
    }
}
#endif

// -----------------------------------------------------------------------
// fillXmlStringFromStateSave

#ifdef HAVE_JUCE_LATER
void fillXmlStringFromStateSave(String& content, const StateSave& stateSave)
{
    {
        String info(" <Info>\n");

        info << " <Type>" << String(stateSave.type != nullptr ? stateSave.type : "") << "</Type>\n";
        info << " <Name>" << xmlSafeString(stateSave.name, true) << "</Name>\n";

        switch (getPluginTypeFromString(stateSave.type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            info << "   <Label>"    << xmlSafeString(stateSave.label, true)  << "</Label>\n";
            break;
        case PLUGIN_LADSPA:
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            info << "   <Label>"    << xmlSafeString(stateSave.label, true)  << "</Label>\n";
            info << "   <UniqueID>" << stateSave.uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_DSSI:
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            info << "   <Label>"    << xmlSafeString(stateSave.label, true)  << "</Label>\n";
            break;
        case PLUGIN_LV2:
            info << "   <Bundle>"   << xmlSafeString(stateSave.binary, true) << "</Bundle>\n";
            info << "   <URI>"      << xmlSafeString(stateSave.label, true)  << "</URI>\n";
            break;
        case PLUGIN_VST:
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            info << "   <UniqueID>" << stateSave.uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_VST3:
            // TODO?
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            info << "   <UniqueID>" << stateSave.uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_AU:
            // TODO?
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            info << "   <UniqueID>" << stateSave.uniqueId                    << "</UniqueID>\n";
            break;
        case PLUGIN_JACK:
            info << "   <Binary>"   << xmlSafeString(stateSave.binary, true) << "</Binary>\n";
            break;
        case PLUGIN_REWIRE:
            info << "   <Label>"    << xmlSafeString(stateSave.label, true)  << "</Label>\n";
            break;
        case PLUGIN_FILE_CSD:
        case PLUGIN_FILE_GIG:
        case PLUGIN_FILE_SF2:
        case PLUGIN_FILE_SFZ:
            info << "   <Filename>"   << xmlSafeString(stateSave.binary, true) << "</Filename>\n";
            info << "   <Label>"      << xmlSafeString(stateSave.label, true)  << "</Label>\n";
            break;
        }

        info << " </Info>\n\n";

        content << info;
    }

    content << "  <Data>\n";

    {
        String data;

        data << "   <Active>" << (stateSave.active ? "Yes" : "No") << "</Active>\n";

        if (stateSave.dryWet != 1.0f)
            data << "   <DryWet>"        << stateSave.dryWet       << "</DryWet>\n";
        if (stateSave.volume != 1.0f)
            data << "   <Volume>"        << stateSave.volume       << "</Volume>\n";
        if (stateSave.balanceLeft != -1.0f)
            data << "   <Balance-Left>"  << stateSave.balanceLeft  << "</Balance-Left>\n";
        if (stateSave.balanceRight != 1.0f)
            data << "   <Balance-Right>" << stateSave.balanceRight << "</Balance-Right>\n";
        if (stateSave.panning != 0.0f)
            data << "   <Panning>"       << stateSave.panning      << "</Panning>\n";

        if (stateSave.ctrlChannel < 0)
            data << "   <ControlChannel>N</ControlChannel>\n";
        else
            data << "   <ControlChannel>" << int(stateSave.ctrlChannel+1) << "</ControlChannel>\n";

        content << data;
    }

    for (StateParameterItenerator it = stateSave.parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        String parameter("\n""   <Parameter>\n");

        parameter << "    <Index>" << String(stateParameter->index)             << "</Index>\n";
        parameter << "    <Name>"  << xmlSafeString(stateParameter->name, true) << "</Name>\n";

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameter << "    <Symbol>" << xmlSafeString(stateParameter->symbol, true) << "</Symbol>\n";

        if (stateParameter->isInput)
            parameter << "    <Value>" << stateParameter->value << "</Value>\n";

        if (stateParameter->midiCC > 0)
        {
            parameter << "    <MidiCC>"      << stateParameter->midiCC        << "</MidiCC>\n";
            parameter << "    <MidiChannel>" << stateParameter->midiChannel+1 << "</MidiChannel>\n";
        }

        parameter << "   </Parameter>\n";

        content << parameter;
    }

    if (stateSave.currentProgramIndex >= 0 && stateSave.currentProgramName != nullptr && stateSave.currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (stateSave.currentProgramIndex > 0 || ! String(stateSave.currentProgramName).equalsIgnoreCase("default"))
        {
            String program("\n");
            program << "   <CurrentProgramIndex>" << stateSave.currentProgramIndex+1                   << "</CurrentProgramIndex>\n";
            program << "   <CurrentProgramName>"  << xmlSafeString(stateSave.currentProgramName, true) << "</CurrentProgramName>\n";

            content << program;
        }
    }

    if (stateSave.currentMidiBank >= 0 && stateSave.currentMidiProgram >= 0)
    {
        String midiProgram("\n");
        midiProgram << "   <CurrentMidiBank>"    << stateSave.currentMidiBank+1    << "</CurrentMidiBank>\n";
        midiProgram << "   <CurrentMidiProgram>" << stateSave.currentMidiProgram+1 << "</CurrentMidiProgram>\n";

        content << midiProgram;
    }

    for (StateCustomDataItenerator it = stateSave.customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->type  != nullptr && stateCustomData->type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->key   != nullptr && stateCustomData->key[0]  != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->value != nullptr);

        String customData("\n""   <CustomData>\n");
        customData << "    <Type>" << xmlSafeString(stateCustomData->type, true) << "</Type>\n";
        customData << "    <Key>"  << xmlSafeString(stateCustomData->key, true)  << "</Key>\n";

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customData << "    <Value>\n";
            customData << xmlSafeString(stateCustomData->value, true);
            customData << "    </Value>\n";
        }
        else
        {
            customData << "    <Value>";
            customData << xmlSafeString(stateCustomData->value, true);
            customData << "</Value>\n";
        }

        customData << "   </CustomData>\n";

        content << customData;
    }

    if (stateSave.chunk != nullptr && stateSave.chunk[0] != '\0')
    {
        String chunk("\n""   <Chunk>\n");
        chunk << stateSave.chunk << "\n   </Chunk>\n";

        content << chunk;
    }

    content << "  </Data>\n";
}
#else
void fillXmlStringFromStateSave(QString& content, const StateSave& stateSave)
{
    {
        QString info("  <Info>\n");

        info += QString("   <Type>%1</Type>\n").arg((stateSave.type != nullptr) ? stateSave.type : "");
        info += QString("   <Name>%1</Name>\n").arg(xmlSafeString(stateSave.name, true));

        switch (getPluginTypeFromString(stateSave.type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(stateSave.label, true));
            break;
        case PLUGIN_LADSPA:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(stateSave.label, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(stateSave.uniqueId);
            break;
        case PLUGIN_DSSI:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(stateSave.label, true));
            break;
        case PLUGIN_LV2:
            info += QString("   <Bundle>%1</Bundle>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <URI>%1</URI>\n").arg(xmlSafeString(stateSave.label, true));
            break;
        case PLUGIN_VST:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(stateSave.uniqueId);
            break;
        case PLUGIN_VST3:
            // TODO?
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(stateSave.uniqueId);
            break;
        case PLUGIN_AU:
            // TODO?
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(stateSave.uniqueId);
            break;
        case PLUGIN_JACK:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(stateSave.binary, true));
            break;
        case PLUGIN_REWIRE:
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(stateSave.label, true));
            break;
        case PLUGIN_FILE_CSD:
        case PLUGIN_FILE_GIG:
        case PLUGIN_FILE_SF2:
        case PLUGIN_FILE_SFZ:
            info += QString("   <Filename>%1</Filename>\n").arg(xmlSafeString(stateSave.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(stateSave.label, true));
            break;
        }

        info += "  </Info>\n\n";

        content += info;
    }

    content += "  <Data>\n";

    {
        QString data;

        data += QString("   <Active>%1</Active>\n").arg(stateSave.active ? "Yes" : "No");

        if (stateSave.dryWet != 1.0f)
            data += QString("   <DryWet>%1</DryWet>\n").arg(stateSave.dryWet, 0, 'g', 7);
        if (stateSave.volume != 1.0f)
            data += QString("   <Volume>%1</Volume>\n").arg(stateSave.volume, 0, 'g', 7);
        if (stateSave.balanceLeft != -1.0f)
            data += QString("   <Balance-Left>%1</Balance-Left>\n").arg(stateSave.balanceLeft, 0, 'g', 7);
        if (stateSave.balanceRight != 1.0f)
            data += QString("   <Balance-Right>%1</Balance-Right>\n").arg(stateSave.balanceRight, 0, 'g', 7);
        if (stateSave.panning != 0.0f)
            data += QString("   <Panning>%1</Panning>\n").arg(stateSave.panning, 0, 'g', 7);

        if (stateSave.ctrlChannel < 0)
            data += QString("   <ControlChannel>N</ControlChannel>\n");
        else
            data += QString("   <ControlChannel>%1</ControlChannel>\n").arg(stateSave.ctrlChannel+1);

        content += data;
    }

    for (StateParameterItenerator it = stateSave.parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        QString parameter("\n""   <Parameter>\n");

        parameter += QString("    <Index>%1</Index>\n").arg(stateParameter->index);
        parameter += QString("    <Name>%1</Name>\n").arg(xmlSafeString(stateParameter->name, true));

        if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            parameter += QString("    <Symbol>%1</Symbol>\n").arg(xmlSafeString(stateParameter->symbol, true));

        if (stateParameter->isInput)
            parameter += QString("    <Value>%1</Value>\n").arg(stateParameter->value, 0, 'g', 15);

        if (stateParameter->midiCC > 0)
        {
            parameter += QString("    <MidiCC>%1</MidiCC>\n").arg(stateParameter->midiCC);
            parameter += QString("    <MidiChannel>%1</MidiChannel>\n").arg(stateParameter->midiChannel+1);
        }

        parameter += "   </Parameter>\n";

        content += parameter;
    }

    if (stateSave.currentProgramIndex >= 0 && stateSave.currentProgramName != nullptr && stateSave.currentProgramName[0] != '\0')
    {
        // ignore 'default' program
        if (stateSave.currentProgramIndex > 0 || QString(stateSave.currentProgramName).compare("default", Qt::CaseInsensitive) != 0)
        {
            QString program("\n");
            program += QString("   <CurrentProgramIndex>%1</CurrentProgramIndex>\n").arg(stateSave.currentProgramIndex+1);
            program += QString("   <CurrentProgramName>%1</CurrentProgramName>\n").arg(xmlSafeString(stateSave.currentProgramName, true));

            content += program;
        }
    }

    if (stateSave.currentMidiBank >= 0 && stateSave.currentMidiProgram >= 0)
    {
        QString midiProgram("\n");
        midiProgram += QString("   <CurrentMidiBank>%1</CurrentMidiBank>\n").arg(stateSave.currentMidiBank+1);
        midiProgram += QString("   <CurrentMidiProgram>%1</CurrentMidiProgram>\n").arg(stateSave.currentMidiProgram+1);

        content += midiProgram;
    }

    for (StateCustomDataItenerator it = stateSave.customData.begin(); it.valid(); it.next())
    {
        StateCustomData* const stateCustomData(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->type  != nullptr && stateCustomData->type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->key   != nullptr && stateCustomData->key[0]  != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->value != nullptr);

        QString customData("\n""   <CustomData>\n");
        customData += QString("    <Type>%1</Type>\n").arg(xmlSafeString(stateCustomData->type, true));
        customData += QString("    <Key>%1</Key>\n").arg(xmlSafeString(stateCustomData->key, true));

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_TYPE_CHUNK) == 0 || std::strlen(stateCustomData->value) >= 128)
        {
            customData += "    <Value>\n";
            customData += QString("%1\n").arg(xmlSafeString(stateCustomData->value, true));
            customData += "    </Value>\n";
        }
        else
            customData += QString("    <Value>%1</Value>\n").arg(xmlSafeString(stateCustomData->value, true));

        customData += "   </CustomData>\n";

        content += customData;
    }

    if (stateSave.chunk != nullptr && stateSave.chunk[0] != '\0')
    {
        QString chunk("\n""   <Chunk>\n");
        chunk += QString("%1\n").arg(stateSave.chunk);
        chunk += "   </Chunk>\n";

        content += chunk;
    }

    content += "  </Data>\n";
}
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
