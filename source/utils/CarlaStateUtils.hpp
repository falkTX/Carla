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

#include "CarlaBackend.hpp"
#include "CarlaUtils.hpp"

#include <QtXml/QDomNode>
#include <vector>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------
// Carla GUI stuff

struct StateParameter {
    uint32_t    index;
    const char* name;
    const char* symbol;
    double      value;
    uint8_t     midiChannel;
    int16_t     midiCC;

    StateParameter()
        : index(0),
          name(nullptr),
          symbol(nullptr),
          value(0.0),
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

typedef std::vector<StateParameter> StateParameterVector;
typedef std::vector<StateCustomData> StateCustomDataVector;

struct SaveState {
    const char* type;
    const char* name;
    const char* label;
    const char* binary;
    long        uniqueID;

    bool   active;
    double dryWet;
    double volume;
    double balanceLeft;
    double balanceRight;
    double panning;

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
          dryWet(1.0),
          volume(1.0),
          balanceLeft(-1.0),
          balanceRight(1.0),
          panning(0.0),
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
        dryWet = 1.0;
        volume = 1.0;
        balanceLeft  = -1.0;
        balanceRight = 1.0;
        panning      = 0.0;
        currentProgramIndex = -1;
        currentMidiBank     = -1;
        currentMidiProgram  = -1;

        parameters.clear();
        customData.clear();
    }
};

// -------------------------------------------------
// Carla XML helpers (xml to state)

static inline
QString xmlSafeString(const QString& string, const bool toXml)
{
    QString newString(string);

    if (toXml)
        return newString.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return newString.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"");
}

static inline
const char* xmlSafeStringChar(const QString& string, const bool toXml)
{
    return carla_strdup(xmlSafeString(string, toXml).toUtf8().constData());
}

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

        if (node.toElement().tagName() == "Info")
        {
            QDomNode xmlInfo(node.toElement().firstChild());

            while (! xmlInfo.isNull())
            {
                const QString tag  = xmlInfo.toElement().tagName();
                const QString text = xmlInfo.toElement().text(); //.strip();

                if (tag == "Type")
                    saveState.type = xmlSafeStringChar(text, false);
                else if (tag == "Name")
                    saveState.name = xmlSafeStringChar(text, false);
                else if (tag == "Label" || tag == "URI")
                    saveState.label = xmlSafeStringChar(text, false);
                else if (tag == "Binary")
                    saveState.binary = xmlSafeStringChar(text, false);
                else if (tag == "UniqueID")
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

        else if (node.toElement().tagName() == "Data")
        {
            QDomNode xmlData(node.toElement().firstChild());

            while (! xmlData.isNull())
            {
                const QString tag  = xmlData.toElement().tagName();
                const QString text = xmlData.toElement().text(); //.strip();

                // ----------------------------------------------
                // Internal Data

                if (tag == "Active")
                {
                    saveState.active = bool(text == "Yes");
                }
                else if (tag == "DryWet")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.dryWet = value;
                }
                else if (tag == "Volume")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.volume = value;
                }
                else if (tag == "Balance-Left")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.balanceLeft = value;
                }
                else if (tag == "Balance-Right")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.balanceRight = value;
                }

                // ----------------------------------------------
                // Program (current)

                else if (tag == "CurrentProgramIndex")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentProgramIndex = value;
                }
                else if (tag == "CurrentProgramName")
                {
                    saveState.currentProgramName = xmlSafeStringChar(text, false);
                }

                // ----------------------------------------------
                // Midi Program (current)

                else if (tag == "CurrentMidiBank")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiBank = value;
                }
                else if (tag == "CurrentMidiProgram")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiProgram = value;
                }

                // ----------------------------------------------
                // Parameters

                else if (tag == "Parameter")
                {
                    StateParameter stateParameter;

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString pTag  = xmlSubData.toElement().tagName();
                        const QString pText = xmlSubData.toElement().text(); //.strip();

                        if (pTag == "index")
                        {
                            bool ok;
                            uint index = pText.toUInt(&ok);
                            if (ok) stateParameter.index = index;
                        }
                        else if (pTag == "name")
                        {
                            stateParameter.name = xmlSafeStringChar(pText, false);
                        }
                        else if (pTag == "symbol")
                        {
                            stateParameter.symbol = xmlSafeStringChar(pText, false);
                        }
                        else if (pTag == "value")
                        {
                            bool ok;
                            double value = pText.toDouble(&ok);
                            if (ok) stateParameter.value = value;
                        }
                        else if (pTag == "midiChannel")
                        {
                            bool ok;
                            uint channel = pText.toUInt(&ok);
                            if (ok && channel < 16)
                                stateParameter.midiChannel = static_cast<uint8_t>(channel);
                        }
                        else if (pTag == "midiCC")
                        {
                            bool ok;
                            int cc = pText.toInt(&ok);
                            if (ok && cc < INT16_MAX)
                                stateParameter.midiCC = static_cast<int16_t>(cc);
                        }

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.parameters.push_back(stateParameter);
                }

                // ----------------------------------------------
                // Custom Data

                else if (tag == "CustomData")
                {
                    StateCustomData stateCustomData;

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString cTag  = xmlSubData.toElement().tagName();
                        const QString cText = xmlSubData.toElement().text(); //.strip();

                        if (cTag == "type")
                            stateCustomData.type = xmlSafeStringChar(cText, false);
                        else if (cTag == "key")
                            stateCustomData.key = xmlSafeStringChar(cText, false);
                        else if (cTag == "value")
                            stateCustomData.value = xmlSafeStringChar(cText, false);

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.customData.push_back(stateCustomData);
                }

                // ----------------------------------------------
                // Chunk

                else if (tag == "Chunk")
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

static inline
QString getXMLFromSaveState(const SaveState& saveState)
{
    return "";

    // TODO
    (void)saveState;
}

// -------------------------------------------------
// Carla XML helpers (state to xml)

// -------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_STATE_UTILS_HPP__
