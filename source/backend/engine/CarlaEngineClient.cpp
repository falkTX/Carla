/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineUtils.hpp"

#include "CarlaString.hpp"
#include "CarlaStringList.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Carla Engine client (Abstract)

struct CarlaEngineClient::ProtectedData {
    const CarlaEngine& engine;

    bool     active;
    uint32_t latency;

    CarlaStringList audioInList;
    CarlaStringList audioOutList;
    CarlaStringList cvInList;
    CarlaStringList cvOutList;
    CarlaStringList eventInList;
    CarlaStringList eventOutList;

    ProtectedData(const CarlaEngine& eng) noexcept
        :  engine(eng),
           active(false),
           latency(0),
           audioInList(),
           audioOutList(),
           cvInList(),
           cvOutList(),
           eventInList(),
           eventOutList() {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

CarlaEngineClient::CarlaEngineClient(const CarlaEngine& engine)
    : pData(new ProtectedData(engine))
{
    carla_debug("CarlaEngineClient::CarlaEngineClient()");
}

CarlaEngineClient::~CarlaEngineClient() noexcept
{
    CARLA_SAFE_ASSERT(! pData->active);
    carla_debug("CarlaEngineClient::~CarlaEngineClient()");

    delete pData;
}

void CarlaEngineClient::activate() noexcept
{
    CARLA_SAFE_ASSERT(! pData->active);
    carla_debug("CarlaEngineClient::activate()");

    pData->active = true;
}

void CarlaEngineClient::deactivate() noexcept
{
    CARLA_SAFE_ASSERT(pData->active);
    carla_debug("CarlaEngineClient::deactivate()");

    pData->active = false;
}

bool CarlaEngineClient::isActive() const noexcept
{
    return pData->active;
}

bool CarlaEngineClient::isOk() const noexcept
{
    return true;
}

uint32_t CarlaEngineClient::getLatency() const noexcept
{
    return pData->latency;
}

void CarlaEngineClient::setLatency(const uint32_t samples) noexcept
{
    pData->latency = samples;
}

CarlaEnginePort* CarlaEngineClient::addPort(const EnginePortType portType, const char* const name, const bool isInput, const uint32_t indexOffset)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngineClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        _addAudioPortName(isInput, name);
        return new CarlaEngineAudioPort(*this, isInput, indexOffset);
    case kEnginePortTypeCV:
        _addCVPortName(isInput, name);
        return new CarlaEngineCVPort(*this, isInput, indexOffset);
    case kEnginePortTypeEvent:
        _addEventPortName(isInput, name);
        return new CarlaEngineEventPort(*this, isInput, indexOffset);
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

const CarlaEngine& CarlaEngineClient::getEngine() const noexcept
{
    return pData->engine;
}

EngineProcessMode CarlaEngineClient::getProcessMode() const noexcept
{
    return pData->engine.getProccessMode();
}

const char* CarlaEngineClient::getAudioPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->audioInList : pData->audioOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index, nullptr);
}

const char* CarlaEngineClient::getCVPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->cvInList : pData->cvOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index, nullptr);
}

const char* CarlaEngineClient::getEventPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->eventInList : pData->eventOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index, nullptr);
}

void CarlaEngineClient::_addAudioPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? pData->audioInList : pData->audioOutList);
    portList.append(name);
}

void CarlaEngineClient::_addCVPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? pData->cvInList : pData->cvOutList);
    portList.append(name);
}

void CarlaEngineClient::_addEventPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? pData->eventInList : pData->eventOutList);
    portList.append(name);
}

static void getUniquePortName(CarlaString& sname, const CarlaStringList& list)
{
    for (CarlaStringList::Itenerator it = list.begin2(); it.valid(); it.next())
    {
        const char* const portName(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr && portName[0] != '\0');

        // Check if unique name doesn't exist
          if (sname != portName)
              continue;

        // Check if string has already been modified
        {
            const std::size_t len(sname.length());

            // 1 digit, ex: " (2)"
            if (sname[len-4] == ' ' && sname[len-3] == '(' && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                const int number = sname[len-2] - '0';

                if (number == 9)
                {
                    // next number is 10, 2 digits
                    sname.truncate(len-4);
                    sname += " (10)";
                    //sname.replace(" (9)", " (10)");
                }
                else
                    sname[len-2] = char('0' + number + 1);

                continue;
            }

            // 2 digits, ex: " (11)"
            if (sname[len-5] == ' ' && sname[len-4] == '(' && sname.isDigit(len-3) && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                char n2 = sname[len-2];
                char n3 = sname[len-3];

                if (n2 == '9')
                {
                    n2 = '0';
                    n3 = static_cast<char>(n3 + 1);
                }
                else
                    n2 = static_cast<char>(n2 + 1);

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }
        }

        // Modify string if not
        sname += " (2)";
    }
}

const char* CarlaEngineClient::_getUniquePortName(const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);

    CarlaString sname;
    sname = name;

    getUniquePortName(sname, pData->audioInList);
    getUniquePortName(sname, pData->audioOutList);
    getUniquePortName(sname, pData->cvInList);
    getUniquePortName(sname, pData->cvOutList);
    getUniquePortName(sname, pData->eventInList);
    getUniquePortName(sname, pData->eventOutList);

    return sname.dup();
}

void CarlaEngineClient::_clearPorts()
{
    pData->audioInList.clear();
    pData->audioOutList.clear();
    pData->cvInList.clear();
    pData->cvOutList.clear();
    pData->eventInList.clear();
    pData->eventOutList.clear();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
