/*
 * Carla Backend utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_STATE_UTILS_HPP__
#define __CARLA_STATE_UTILS_HPP__

#include "CarlaBackendUtils.hpp"

#include <QtXml/QDomNode>
#include <QtCore/QVector>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------

struct StateParameter {
    uint32_t    index;
    const char* name;
    const char* symbol;
    float       value;
    uint8_t     midiChannel;
    int16_t     midiCC;

    StateParameter()
        : index(0),
          name(nullptr),
          symbol(nullptr),
          value(0.0f),
          midiChannel(1),
          midiCC(-1) {}

    ~StateParameter()
    {
        if (name != nullptr)
            delete[] name;
        if (symbol != nullptr)
            delete[] symbol;
    }
};

struct StateCustomData {
    const char* type;
    const char* key;
    const char* value;

    StateCustomData()
        : type(nullptr),
          key(nullptr),
          value(nullptr) {}

    ~StateCustomData()
    {
        if (type != nullptr)
            delete[] type;
        if (key != nullptr)
            delete[] key;
        if (value != nullptr)
            delete[] value;
    }
};

typedef QVector<StateParameter*> StateParameterVector;
typedef QVector<StateCustomData*> StateCustomDataVector;

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

    StateParameterVector  parameters;
    StateCustomDataVector customData;

    SaveState()
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
          ctrlChannel(0),
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
        ctrlChannel  = 0;
        currentProgramIndex = -1;
        currentMidiBank     = -1;
        currentMidiProgram  = -1;

        for (auto it = parameters.begin(); it != parameters.end(); ++it)
        {
            StateParameter* stateParameter = *it;
            delete stateParameter;
        }

        for (auto it = customData.begin(); it != customData.end(); ++it)
        {
            StateCustomData* stateCustomData = *it;
            delete stateCustomData;
        }

        parameters.clear();
        customData.clear();
    }
};

// -------------------------------------------------

static inline
QString xmlSafeString(const QString& string, const bool toXml)
{
    QString newString(string);

    if (toXml)
        return newString.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&amp;","&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"");
}

static inline
const char* xmlSafeStringChar(const QString& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toUtf8().constData());
}

// -------------------------------------------------

static inline
const SaveState& getSaveStateDictFromXML(const QDomNode& xmlNode)
{
    static SaveState saveState;
    saveState.reset();

    QDomNode node(xmlNode.firstChild());

    while (! node.isNull())
    {
        // ------------------------------------------------------
        // Info

        if (node.toElement().tagName().compare("Info", Qt::CaseInsensitive) == 0)
        {
            QDomNode xmlInfo(node.toElement().firstChild());

            while (! xmlInfo.isNull())
            {
                const QString tag  = xmlInfo.toElement().tagName();
                const QString text = xmlInfo.toElement().text().trimmed();

                if (tag.compare("Type", Qt::CaseInsensitive) == 0)
                    saveState.type = xmlSafeStringChar(text, false);
                else if (tag.compare("Name", Qt::CaseInsensitive) == 0)
                    saveState.name = xmlSafeStringChar(text, false);
                else if (tag.compare("Label", Qt::CaseInsensitive) == 0 || tag.compare("URI", Qt::CaseInsensitive) == 0)
                    saveState.label = xmlSafeStringChar(text, false);
                else if (tag.compare("Binary", Qt::CaseInsensitive) == 0)
                    saveState.binary = xmlSafeStringChar(text, false);
                else if (tag.compare("UniqueID", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    long uniqueID = text.toLong(&ok);
                    if (ok) saveState.uniqueID = uniqueID;
                }

                xmlInfo = xmlInfo.nextSibling();
            }
        }

        // ------------------------------------------------------
        // Data

        else if (node.toElement().tagName().compare("Data", Qt::CaseInsensitive) == 0)
        {
            QDomNode xmlData(node.toElement().firstChild());

            while (! xmlData.isNull())
            {
                const QString tag  = xmlData.toElement().tagName();
                const QString text = xmlData.toElement().text().trimmed();

                // ----------------------------------------------
                // Internal Data

                if (tag.compare("Active", Qt::CaseInsensitive) == 0)
                {
                    saveState.active = (text.compare("Yes", Qt::CaseInsensitive) == 0);
                }
                else if (tag.compare("DryWet", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    float value = text.toFloat(&ok);
                    if (ok) saveState.dryWet = value;
                }
                else if (tag.compare("Volume", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    float value = text.toFloat(&ok);
                    if (ok) saveState.volume = value;
                }
                else if (tag.compare("Balance-Left", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    float value = text.toFloat(&ok);
                    if (ok) saveState.balanceLeft = value;
                }
                else if (tag.compare("Balance-Right", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    float value = text.toFloat(&ok);
                    if (ok) saveState.balanceRight = value;
                }
                else if (tag.compare("Panning", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    float value = text.toFloat(&ok);
                    if (ok) saveState.panning = value;
                }
                else if (tag.compare("ControlChannel", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    short value = text.toShort(&ok);
                    if (ok && value < INT8_MAX)
                        saveState.ctrlChannel = static_cast<int8_t>(value-1);
                }

                // ----------------------------------------------
                // Program (current)

                else if (tag.compare("CurrentProgramIndex", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentProgramIndex = value-1;
                }
                else if (tag.compare("CurrentProgramName", Qt::CaseInsensitive) == 0)
                {
                    saveState.currentProgramName = xmlSafeStringChar(text, false);
                }

                // ----------------------------------------------
                // Midi Program (current)

                else if (tag.compare("CurrentMidiBank", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiBank = value-1;
                }
                else if (tag.compare("CurrentMidiProgram", Qt::CaseInsensitive) == 0)
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiProgram = value-1;
                }

                // ----------------------------------------------
                // Parameters

                else if (tag.compare("Parameter", Qt::CaseInsensitive) == 0)
                {
                    StateParameter* stateParameter(new StateParameter);

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString pTag  = xmlSubData.toElement().tagName();
                        const QString pText = xmlSubData.toElement().text().trimmed();

                        if (pTag.compare("Index", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            uint index = pText.toUInt(&ok);
                            if (ok) stateParameter->index = index;
                        }
                        else if (pTag.compare("Name", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->name = xmlSafeStringChar(pText, false);
                        }
                        else if (pTag.compare("Symbol", Qt::CaseInsensitive) == 0)
                        {
                            stateParameter->symbol = xmlSafeStringChar(pText, false);
                        }
                        else if (pTag.compare("Value", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            float value = pText.toFloat(&ok);
                            if (ok) stateParameter->value = value;
                        }
                        else if (pTag.compare("MidiChannel", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            ushort channel = pText.toUShort(&ok);
                            if (ok && channel < 16)
                                stateParameter->midiChannel = static_cast<uint8_t>(channel);
                        }
                        else if (pTag.compare("MidiCC", Qt::CaseInsensitive) == 0)
                        {
                            bool ok;
                            int cc = pText.toInt(&ok);
                            if (ok && cc < INT16_MAX)
                                stateParameter->midiCC = static_cast<int16_t>(cc);
                        }

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.parameters.push_back(stateParameter);
                }

                // ----------------------------------------------
                // Custom Data

                else if (tag.compare("CustomData", Qt::CaseInsensitive) == 0)
                {
                    StateCustomData* stateCustomData(new StateCustomData);

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString cTag  = xmlSubData.toElement().tagName();
                        const QString cText = xmlSubData.toElement().text().trimmed();

                        if (cTag.compare("Type", Qt::CaseInsensitive) == 0)
                            stateCustomData->type = xmlSafeStringChar(cText, false);
                        else if (cTag.compare("Key", Qt::CaseInsensitive) == 0)
                            stateCustomData->key = xmlSafeStringChar(cText, false);
                        else if (cTag.compare("Value", Qt::CaseInsensitive) == 0)
                            stateCustomData->value = xmlSafeStringChar(cText, false);

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.customData.push_back(stateCustomData);
                }

                // ----------------------------------------------
                // Chunk

                else if (tag.compare("Chunk", Qt::CaseInsensitive) == 0)
                {
                    saveState.chunk = xmlSafeStringChar(text, false);
                }

                // ----------------------------------------------

                xmlData = xmlData.nextSibling();
            }
        }

        // ------------------------------------------------------

        node = node.nextSibling();
    }

    return saveState;
}

// -------------------------------------------------

static inline
QString getXMLFromSaveState(const SaveState& saveState)
{
    QString content;

    {
        QString info("  <Info>\n");

        info += QString("   <Type>%1</Type>\n").arg(saveState.type);
        info += QString("   <Name>%1</Name>\n").arg(xmlSafeString(saveState.name, true));

        switch (getPluginTypeFromString(saveState.type))
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(saveState.label, true));
            break;
        case PLUGIN_LADSPA:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(saveState.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(saveState.label, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(saveState.uniqueID);
            break;
        case PLUGIN_DSSI:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(saveState.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(saveState.label, true));
            break;
        case PLUGIN_LV2:
            info += QString("   <URI>%1</URI>\n").arg(xmlSafeString(saveState.label, true));
            break;
        case PLUGIN_VST:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(saveState.binary, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(saveState.uniqueID);
            break;
        case PLUGIN_VST3:
            // TODO?
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(saveState.binary, true));
            info += QString("   <UniqueID>%1</UniqueID>\n").arg(saveState.uniqueID);
            break;
        case PLUGIN_GIG:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
            info += QString("   <Binary>%1</Binary>\n").arg(xmlSafeString(saveState.binary, true));
            info += QString("   <Label>%1</Label>\n").arg(xmlSafeString(saveState.label, true));
            break;
        }

        info += "  </Info>\n\n";

        content += info;
    }

    {
        QString data("  <Data>\n");

        data += QString("   <Active>%1</Active>\n").arg(saveState.active ? "Yes" : "No");

        if (saveState.dryWet != 1.0f)
            data += QString("   <DryWet>%1</DryWet>\n").arg(saveState.dryWet);
        if (saveState.volume != 1.0f)
            data += QString("   <Volume>%1</Volume>\n").arg(saveState.volume);
        if (saveState.balanceLeft != -1.0f)
            data += QString("   <Balance-Left>%1</Balance-Left>\n").arg(saveState.balanceLeft);
        if (saveState.balanceRight != 1.0f)
            data += QString("   <Balance-Right>%1</Balance-Right>\n").arg(saveState.balanceRight);
        if (saveState.panning != 0.0f)
            data += QString("   <Panning>%1</Panning>\n").arg(saveState.panning);

        data += QString("   <ControlChannel>%1</ControlChannel>\n").arg(saveState.ctrlChannel+1);

        content += data;
    }

    for (auto it = saveState.parameters.begin(); it != saveState.parameters.end(); ++it)
    {
        StateParameter* stateParameter = *it;

        QString parameter("\n""   <Parameter>\n");

        parameter += QString("    <Index>%1</Index>\n").arg(stateParameter->index);
        parameter += QString("    <Name>%1</Name>\n").arg(xmlSafeString(stateParameter->name, true));

        if (stateParameter->symbol != nullptr && *stateParameter->symbol != 0)
            parameter += QString("    <Symbol>%1</Symbol>\n").arg(xmlSafeString(stateParameter->symbol, true));

        parameter += QString("    <Value>%1</Value>\n").arg(stateParameter->value);

        if (stateParameter->midiCC > 0)
        {
            parameter += QString("    <MidiCC>%1</MidiCC>\n").arg(stateParameter->midiCC);
            parameter += QString("    <MidiChannel>%1</MidiChannel>\n").arg(stateParameter->midiChannel);
        }

        parameter += "   </Parameter>\n";

        content += parameter;
    }

    if (saveState.currentProgramIndex >= 0)
    {
        QString program("\n");
        program += QString("   <CurrentProgramIndex>%1</CurrentProgramIndex>\n").arg(saveState.currentProgramIndex+1);
        program += QString("   <CurrentProgramName>%1</CurrentProgramName>\n").arg(xmlSafeString(saveState.currentProgramName, true));

        content += program;
    }

    if (saveState.currentMidiBank >= 0 && saveState.currentMidiProgram >= 0)
    {
        QString midiProgram("\n");
        midiProgram += QString("   <CurrentMidiBank>%1</CurrentMidiBank>\n").arg(saveState.currentMidiBank+1);
        midiProgram += QString("   <CurrentMidiProgram>%1</CurrentMidiProgram>\n").arg(saveState.currentMidiProgram+1);

        content += midiProgram;
    }

    for (auto it = saveState.customData.begin(); it != saveState.customData.end(); ++it)
    {
        StateCustomData* stateCustomData = *it;

        QString customData("\n""   <CustomData>\n");
        customData += QString("    <Type>%1</Type>\n").arg(xmlSafeString(stateCustomData->type, true));
        customData += QString("    <Key>%1</Key>\n").arg(xmlSafeString(stateCustomData->key, true));

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_CHUNK) == 0)
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

    if (saveState.chunk != nullptr && *saveState.chunk != 0)
    {
        QString chunk("\n""   <Chunk>\n");
        chunk += QString("%1\n").arg(saveState.chunk);
        chunk += "   </Chunk>\n";

        content += chunk;
    }

    content += "  </Data>\n";

    return content;
}

// -------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_STATE_UTILS_HPP__
