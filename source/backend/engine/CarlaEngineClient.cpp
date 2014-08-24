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

    ProtectedData(const CarlaEngine& eng)
        :  engine(eng),
           active(false),
           latency(0),
           audioInList(),
           audioOutList() {}

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

CarlaEnginePort* CarlaEngineClient::addPort(const EnginePortType portType, const char* const name, const bool isInput)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngineClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

    _addName(isInput, name);

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        return new CarlaEngineAudioPort(*this, isInput);
    case kEnginePortTypeCV:
        return new CarlaEngineCVPort(*this, isInput);
    case kEnginePortTypeEvent:
        return new CarlaEngineEventPort(*this, isInput);
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

const char* CarlaEngineClient::getAudioInputPortName(const uint index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->audioInList.count(), nullptr);

    return pData->audioInList.getAt(index, nullptr);
}

const char* CarlaEngineClient::getAudioOutputPortName(const uint index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->audioOutList.count(), nullptr);

    return pData->audioOutList.getAt(index, nullptr);
}

void CarlaEngineClient::_addName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    if (isInput)
        pData->audioInList.append(name);
    else
        pData->audioOutList.append(name);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
