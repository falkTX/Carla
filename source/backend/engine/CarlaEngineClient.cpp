// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaEngineClient.hpp"
#include "CarlaEngineUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

static void _getUniquePortName(String& sname, const CarlaStringList& list)
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

// -----------------------------------------------------------------------
// Carla Engine Client

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
CarlaEngineClient::ProtectedData::ProtectedData(const CarlaEngine& eng,
                                                EngineInternalGraph& eg,
                                                const CarlaPluginPtr p) noexcept
#else
CarlaEngineClient::ProtectedData::ProtectedData(const CarlaEngine& eng) noexcept
#endif
    :  engine(eng),
       active(false),
       latency(0),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
       cvSourcePorts(),
       egraph(eg),
       plugin(p),
#endif
       audioInList(),
       audioOutList(),
       cvInList(),
       cvOutList(),
       eventInList(),
       eventOutList() {}

CarlaEngineClient::ProtectedData::~ProtectedData()
{
    carla_debug("CarlaEngineClient::ProtectedData::~ProtectedData()");
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT(plugin.get() == nullptr);
#endif
}

void CarlaEngineClient::ProtectedData::addAudioPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? audioInList : audioOutList);
    portList.append(name);
}

void CarlaEngineClient::ProtectedData::addCVPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? cvInList : cvOutList);
    portList.append(name);
}

void CarlaEngineClient::ProtectedData::addEventPortName(const bool isInput, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    CarlaStringList& portList(isInput ? eventInList : eventOutList);
    portList.append(name);
}

const char* CarlaEngineClient::ProtectedData::getUniquePortName(const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);

    String sname;
    sname = name;

    _getUniquePortName(sname, audioInList);
    _getUniquePortName(sname, audioOutList);
    _getUniquePortName(sname, cvInList);
    _getUniquePortName(sname, cvOutList);
    _getUniquePortName(sname, eventInList);
    _getUniquePortName(sname, eventOutList);

    return carla_strdup(sname);
}

void CarlaEngineClient::ProtectedData::clearPorts()
{
    audioInList.clear();
    audioOutList.clear();
    cvInList.clear();
    cvOutList.clear();
    eventInList.clear();
    eventOutList.clear();
}

// -----------------------------------------------------------------------

CarlaEngineClient::CarlaEngineClient(ProtectedData* const p)
    : pData(p)
{
    carla_debug("CarlaEngineClient::CarlaEngineClient()");
}

CarlaEngineClient::~CarlaEngineClient() noexcept
{
    carla_debug("CarlaEngineClient::~CarlaEngineClient()");
}

void CarlaEngineClient::activate() noexcept
{
    CARLA_SAFE_ASSERT(! pData->active);
    carla_debug("CarlaEngineClient::activate()");

    pData->active = true;
}

void CarlaEngineClient::deactivate(const bool willClose) noexcept
{
    CARLA_SAFE_ASSERT(pData->active || willClose);
    carla_debug("CarlaEngineClient::deactivate(%s)", bool2str(willClose));

    pData->active = false;

    if (willClose)
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        pData->cvSourcePorts.resetGraphAndPlugin();
        pData->plugin.reset();
#endif
    }
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
    carla_debug("CarlaEngineClient::addPort(%i:%s, \"%s\", %s, %u)", portType, EnginePortType2Str(portType), name, bool2str(isInput), indexOffset);

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        pData->addAudioPortName(isInput, name);
        return new CarlaEngineAudioPort(*this, isInput, indexOffset);
    case kEnginePortTypeCV:
        pData->addCVPortName(isInput, name);
        return new CarlaEngineCVPort(*this, isInput, indexOffset);
    case kEnginePortTypeEvent:
        pData->addEventPortName(isInput, name);
        return new CarlaEngineEventPort(*this, isInput, indexOffset);
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

bool CarlaEngineClient::removePort(const EnginePortType portType, const char* const name, const bool isInput)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', false);
    carla_debug("CarlaEngineClient::removePort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio: {
        CarlaStringList& portList(isInput ? pData->audioInList : pData->audioOutList);
        portList.append(name);
        return portList.removeOne(name);
    }
    case kEnginePortTypeCV: {
        CarlaStringList& portList(isInput ? pData->cvInList : pData->cvOutList);
        return portList.removeOne(name);
    }
    case kEnginePortTypeEvent: {
        CarlaStringList& portList(isInput ? pData->eventInList : pData->eventOutList);
        return portList.removeOne(name);
    }
    }

    return false;
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
CarlaEngineCVSourcePorts* CarlaEngineClient::createCVSourcePorts()
{
    pData->cvSourcePorts.setGraphAndPlugin(pData->egraph.getPatchbayGraphOrNull(), pData->plugin);
    return &pData->cvSourcePorts;
}
#endif

const CarlaEngine& CarlaEngineClient::getEngine() const noexcept
{
    return pData->engine;
}

EngineProcessMode CarlaEngineClient::getProcessMode() const noexcept
{
    return pData->engine.getProccessMode();
}

uint CarlaEngineClient::getPortCount(const EnginePortType portType, const bool isInput) const noexcept
{
    size_t ret = 0;

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        ret = isInput ? pData->audioInList.count() : pData->audioOutList.count();
        break;
    case kEnginePortTypeCV:
        ret = isInput ? pData->cvInList.count() : pData->cvOutList.count();
        break;
    case kEnginePortTypeEvent:
        ret = isInput ? pData->eventInList.count() : pData->eventOutList.count();
        break;
    }

    return static_cast<uint>(ret);
}

const char* CarlaEngineClient::getAudioPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->audioInList : pData->audioOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index);
}

const char* CarlaEngineClient::getCVPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->cvInList : pData->cvOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index);
}

const char* CarlaEngineClient::getEventPortName(const bool isInput, const uint index) const noexcept
{
    CarlaStringList& portList(isInput ? pData->eventInList : pData->eventOutList);
    CARLA_SAFE_ASSERT_RETURN(index < portList.count(), nullptr);

    return portList.getAt(index);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
