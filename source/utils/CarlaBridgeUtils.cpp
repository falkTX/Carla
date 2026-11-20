// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaBridgeUtils.hpp"
#include "CarlaShmUtils.hpp"

#include "extra/Sleep.hpp"

// must be last
#include "jackbridge/JackBridge.hpp"

#if defined(CARLA_OS_WIN) && !defined(BUILDING_CARLA_FOR_WINE)
# define PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL    "Local\\carla-bridge_shm_ap_"
# define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "Local\\carla-bridge_shm_rtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "Local\\carla-bridge_shm_nonrtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "Local\\carla-bridge_shm_nonrtS_"
#else
# define PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL    "/crlbrdg_shm_ap_"
# define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "/crlbrdg_shm_rtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "/crlbrdg_shm_nonrtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "/crlbrdg_shm_nonrtS_"
#endif

// -------------------------------------------------------------------------------------------------------------------

template<typename T>
bool jackbridge_shm_map2(void* shm, T*& value) noexcept
{
    value = (T*)jackbridge_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

// -------------------------------------------------------------------------------------------------------------------

BridgeAudioPool::BridgeAudioPool() noexcept
    : data(nullptr),
      dataSize(0),
      filename(),
      isServer(false)
{
    carla_zeroChars(shm, 64);
    jackbridge_shm_init(shm);
}

BridgeAudioPool::~BridgeAudioPool() noexcept
{
    // should be cleared by now
    CARLA_SAFE_ASSERT(data == nullptr);

    clear();
}

bool BridgeAudioPool::initializeServer() noexcept
{
    char tmpFileBase[64] = {};
    std::snprintf(tmpFileBase, sizeof(tmpFileBase)-1, PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    filename = tmpFileBase;
    isServer = true;
    return true;
}

bool BridgeAudioPool::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
}

void BridgeAudioPool::clear() noexcept
{
    filename.clear();

    if (! jackbridge_shm_is_valid(shm))
    {
        CARLA_SAFE_ASSERT(data == nullptr);
        return;
    }

    if (data != nullptr)
    {
        if (isServer)
            jackbridge_shm_unmap(shm, data);
        data = nullptr;
    }

    dataSize = 0;
    jackbridge_shm_close(shm);
    jackbridge_shm_init(shm);
}

void BridgeAudioPool::resize(const uint32_t bufferSize, const uint32_t audioPortCount, const uint32_t cvPortCount) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(jackbridge_shm_is_valid(shm),);
    CARLA_SAFE_ASSERT_RETURN(isServer,);

    if (data != nullptr)
        jackbridge_shm_unmap(shm, data);

    dataSize = (audioPortCount+cvPortCount)*bufferSize*sizeof(float);

    if (dataSize == 0)
        dataSize = sizeof(float);

    data = (float*)jackbridge_shm_map(shm, dataSize);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

    std::memset(data, 0, dataSize);
}

const char* BridgeAudioPool::getFilenameSuffix() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename.isNotEmpty(), nullptr);

    const std::size_t prefixLength(std::strlen(PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL));
    CARLA_SAFE_ASSERT_RETURN(filename.length() > prefixLength, nullptr);

    return filename.buffer() + prefixLength;
}

// -------------------------------------------------------------------------------------------------------------------

BridgeRtClientControl::BridgeRtClientControl() noexcept
    : data(nullptr),
      filename(),
      needsSemDestroy(false),
      isServer(false)
{
    carla_zeroChars(shm, 64);
    jackbridge_shm_init(shm);
}

BridgeRtClientControl::~BridgeRtClientControl() noexcept
{
    // should be cleared by now
    CARLA_SAFE_ASSERT(data == nullptr);

    clear();
}

bool BridgeRtClientControl::initializeServer() noexcept
{
    char tmpFileBase[64] = {};
    std::snprintf(tmpFileBase, sizeof(tmpFileBase)-1, PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    filename = tmpFileBase;
    isServer = true;

    if (! mapData())
    {
        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
        return false;
    }

    CARLA_SAFE_ASSERT(data != nullptr);

    if (! jackbridge_sem_init(&data->sem.server))
    {
        unmapData();
        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
        return false;
    }

    if (! jackbridge_sem_init(&data->sem.client))
    {
        jackbridge_sem_destroy(&data->sem.server);
        unmapData();
        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
        return false;
    }

    needsSemDestroy = true;
    return true;
}

bool BridgeRtClientControl::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
}

void BridgeRtClientControl::clear() noexcept
{
    filename.clear();

    if (needsSemDestroy)
    {
        jackbridge_sem_destroy(&data->sem.client);
        jackbridge_sem_destroy(&data->sem.server);
        needsSemDestroy = false;
    }

    if (data != nullptr)
        unmapData();

    if (! jackbridge_shm_is_valid(shm))
        return;

    jackbridge_shm_close(shm);
    jackbridge_shm_init(shm);
}

bool BridgeRtClientControl::mapData() noexcept
{
    CARLA_SAFE_ASSERT(data == nullptr);

    if (! jackbridge_shm_map2<BridgeRtClientData>(shm, data))
        return false;

    if (isServer)
    {
        std::memset(data, 0, sizeof(BridgeRtClientData));
        setRingBuffer(&data->ringBuffer, true);
    }
    else
    {
        CARLA_SAFE_ASSERT(data->midiOut[0] == 0);
        setRingBuffer(&data->ringBuffer, false);

        CARLA_SAFE_ASSERT_RETURN(jackbridge_sem_connect(&data->sem.server), false);
        CARLA_SAFE_ASSERT_RETURN(jackbridge_sem_connect(&data->sem.client), false);
    }

    return true;
}

void BridgeRtClientControl::unmapData() noexcept
{
    if (isServer)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        jackbridge_shm_unmap(shm, data);
    }

    data = nullptr;
    setRingBuffer(nullptr, false);
}

bool BridgeRtClientControl::waitForClient(const uint msecs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0, false);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(isServer, false);

    jackbridge_sem_post(&data->sem.server, true);

    return jackbridge_sem_timedwait(&data->sem.client, msecs, true);
}

bool BridgeRtClientControl::writeOpcode(const PluginBridgeRtClientOpcode opcode) noexcept
{
    return writeUInt(static_cast<uint32_t>(opcode));
}

PluginBridgeRtClientOpcode BridgeRtClientControl::readOpcode() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! isServer, kPluginBridgeRtClientNull);

    return static_cast<PluginBridgeRtClientOpcode>(readUInt());
}

BridgeRtClientControl::WaitHelper::WaitHelper(BridgeRtClientControl& c) noexcept
    : data(c.data),
      ok(jackbridge_sem_timedwait(&data->sem.server, 5000, false)) {}

BridgeRtClientControl::WaitHelper::~WaitHelper() noexcept
{
    if (ok)
        jackbridge_sem_post(&data->sem.client, false);
}

// -------------------------------------------------------------------------------------------------------------------

BridgeNonRtClientControl::BridgeNonRtClientControl() noexcept
    : data(nullptr),
      filename(),
      mutex(),
      isServer(false)
{
    carla_zeroChars(shm, 64);
    jackbridge_shm_init(shm);
}

BridgeNonRtClientControl::~BridgeNonRtClientControl() noexcept
{
    // should be cleared by now
    CARLA_SAFE_ASSERT(data == nullptr);

    clear();
}

bool BridgeNonRtClientControl::initializeServer() noexcept
{
    char tmpFileBase[64] = {};
    std::snprintf(tmpFileBase, sizeof(tmpFileBase)-1, PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    filename = tmpFileBase;
    isServer = true;

    if (! mapData())
    {
        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
        return false;
    }

    CARLA_SAFE_ASSERT(data != nullptr);
    return true;
}

bool BridgeNonRtClientControl::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
}

void BridgeNonRtClientControl::clear() noexcept
{
    filename.clear();

    if (data != nullptr)
        unmapData();

    if (! jackbridge_shm_is_valid(shm))
    {
        if (! isServer) {
            CARLA_SAFE_ASSERT(data == nullptr);
        }
        return;
    }

    jackbridge_shm_close(shm);
    jackbridge_shm_init(shm);
}

bool BridgeNonRtClientControl::mapData() noexcept
{
    CARLA_SAFE_ASSERT(data == nullptr);

    if (jackbridge_shm_map2<BridgeNonRtClientData>(shm, data))
    {
        setRingBuffer(&data->ringBuffer, isServer);
        return true;
    }

    return false;
}

void BridgeNonRtClientControl::unmapData() noexcept
{
    if (isServer)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        jackbridge_shm_unmap(shm, data);
    }

    data = nullptr;
    setRingBuffer(nullptr, false);
}

void BridgeNonRtClientControl::waitIfDataIsReachingLimit() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(isServer,);

    if (getWritableDataSize() < BigStackBuffer::size/4)
        return;

    for (int i=50; --i >= 0;)
    {
        if (getWritableDataSize() >= BigStackBuffer::size*3/4)
        {
            writeOpcode(kPluginBridgeNonRtClientPing);
            commitWrite();
            return;
        }
        d_msleep(20);
    }

    carla_stderr("Server waitIfDataIsReachingLimit() reached and failed");
}

bool BridgeNonRtClientControl::writeOpcode(const PluginBridgeNonRtClientOpcode opcode) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(isServer, false);

    return writeUInt(static_cast<uint32_t>(opcode));
}

PluginBridgeNonRtClientOpcode BridgeNonRtClientControl::readOpcode() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! isServer, kPluginBridgeNonRtClientNull);

    return static_cast<PluginBridgeNonRtClientOpcode>(readUInt());
}

// -------------------------------------------------------------------------------------------------------------------

BridgeNonRtServerControl::BridgeNonRtServerControl() noexcept
    : data(nullptr),
      filename(),
      mutex(),
      isServer(false)
{
    carla_zeroChars(shm, 64);
    jackbridge_shm_init(shm);
}

BridgeNonRtServerControl::~BridgeNonRtServerControl() noexcept
{
    // should be cleared by now
    CARLA_SAFE_ASSERT(data == nullptr);

    clear();
}

bool BridgeNonRtServerControl::initializeServer() noexcept
{
    char tmpFileBase[64] = {};
    std::snprintf(tmpFileBase, sizeof(tmpFileBase)-1, PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    filename = tmpFileBase;
    isServer = true;

    if (! mapData())
    {
        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
        return false;
    }

    CARLA_SAFE_ASSERT(data != nullptr);
    return true;
}

bool BridgeNonRtServerControl::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
}

void BridgeNonRtServerControl::clear() noexcept
{
    filename.clear();

    if (data != nullptr)
        unmapData();

    if (! jackbridge_shm_is_valid(shm))
    {
        CARLA_SAFE_ASSERT(data == nullptr);
        return;
    }

    jackbridge_shm_close(shm);
    jackbridge_shm_init(shm);
}

bool BridgeNonRtServerControl::mapData() noexcept
{
    CARLA_SAFE_ASSERT(data == nullptr);

    if (jackbridge_shm_map2<BridgeNonRtServerData>(shm, data))
    {
        setRingBuffer(&data->ringBuffer, isServer);
        return true;
    }

    return false;
}

void BridgeNonRtServerControl::unmapData() noexcept
{
    if (isServer)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        jackbridge_shm_unmap(shm, data);
    }

    data = nullptr;
    setRingBuffer(nullptr, false);
}

PluginBridgeNonRtServerOpcode BridgeNonRtServerControl::readOpcode() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(isServer, kPluginBridgeNonRtServerNull);

    return static_cast<PluginBridgeNonRtServerOpcode>(readUInt());
}

void BridgeNonRtServerControl::waitIfDataIsReachingLimit() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! isServer,);

    if (getWritableDataSize() < HugeStackBuffer::size/4)
        return;

    for (int i=50; --i >= 0;)
    {
        if (getWritableDataSize() >= HugeStackBuffer::size*3/4)
        {
            writeOpcode(kPluginBridgeNonRtServerPong);
            commitWrite();
            return;
        }
        d_msleep(20);
    }

    carla_stderr("Client waitIfDataIsReachingLimit() reached and failed");
}

bool BridgeNonRtServerControl::writeOpcode(const PluginBridgeNonRtServerOpcode opcode) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! isServer, false);

    return writeUInt(static_cast<uint32_t>(opcode));
}

// -------------------------------------------------------------------------------------------------------------------
