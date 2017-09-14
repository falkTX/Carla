/*
 * Carla Bridge utils
 * Copyright (C) 2013-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeUtils.hpp"

#ifndef BUILD_BRIDGE
# include "CarlaShmUtils.hpp"
#endif

// must be last
#include "jackbridge/JackBridge.hpp"

#if defined(CARLA_OS_WIN) && defined(BUILDING_CARLA_FOR_WINDOWS)
# define PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL    "Global\\carla-bridge_shm_ap_"
# define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "Global\\carla-bridge_shm_rtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "Global\\carla-bridge_shm_nonrtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "Global\\carla-bridge_shm_nonrtS_"
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
      filename()
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
#ifndef BUILD_BRIDGE
    char tmpFileBase[64];
    std::sprintf(tmpFileBase, PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    filename = tmpFileBase;
    return true;
#else
    return false;
#endif
}

bool BridgeAudioPool::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

#ifdef BUILD_BRIDGE
    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
#else
    return false;
#endif
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
#ifndef BUILD_BRIDGE
        jackbridge_shm_unmap(shm, data);
#endif
        data = nullptr;
    }

    dataSize = 0;
    jackbridge_shm_close(shm);
    jackbridge_shm_init(shm);
}

void BridgeAudioPool::resize(const uint32_t bufferSize, const uint32_t audioPortCount, const uint32_t cvPortCount) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(jackbridge_shm_is_valid(shm),);

    if (data != nullptr)
        jackbridge_shm_unmap(shm, data);

    dataSize = (audioPortCount+cvPortCount)*bufferSize*sizeof(float);

    if (dataSize == 0)
        dataSize = sizeof(float);

    data = (float*)jackbridge_shm_map(shm, dataSize);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

    std::memset(data, 0, dataSize);
}

// -------------------------------------------------------------------------------------------------------------------

BridgeRtClientControl::BridgeRtClientControl() noexcept
    : data(nullptr),
      filename(),
      needsSemDestroy(false)
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
#ifndef BUILD_BRIDGE
    char tmpFileBase[64];
    std::sprintf(tmpFileBase, PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT "XXXXXX");

    const carla_shm_t shm2 = carla_shm_create_temp(tmpFileBase);
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm2), false);

    void* const shmptr = shm;
    carla_shm_t& shm1  = *(carla_shm_t*)shmptr;
    carla_copyStruct(shm1, shm2);

    // NEW
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
    // NEW

    filename = tmpFileBase;
    needsSemDestroy = true;
    return true;
#else
    return false;
#endif
}

bool BridgeRtClientControl::attachClient(const char* const basename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(basename != nullptr && basename[0] != '\0', false);

#ifdef BUILD_BRIDGE
    // must be invalid right now
    CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

    filename  = PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT;
    filename += basename;

    jackbridge_shm_attach(shm, filename);

    return jackbridge_shm_is_valid(shm);
#else
    return false;
#endif
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

    if (jackbridge_shm_map2<BridgeRtClientData>(shm, data))
    {
#ifndef BUILD_BRIDGE
        std::memset(data, 0, sizeof(BridgeRtClientData));
        setRingBuffer(&data->ringBuffer, true);
#else
        CARLA_SAFE_ASSERT(data->midiOut[0] == 0);
        setRingBuffer(&data->ringBuffer, false);

        CARLA_SAFE_ASSERT_RETURN(jackbridge_sem_connect(&data->sem.server), false);
        CARLA_SAFE_ASSERT_RETURN(jackbridge_sem_connect(&data->sem.client), false);
#endif
        return true;
    }

    return false;
}

void BridgeRtClientControl::unmapData() noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

    jackbridge_shm_unmap(shm, data);
#endif

    data = nullptr;
    setRingBuffer(nullptr, false);
}

bool BridgeRtClientControl::waitForClient(const uint msecs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0, false);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

#ifndef BUILD_BRIDGE
    jackbridge_sem_post(&data->sem.server, true);

    return jackbridge_sem_timedwait(&data->sem.client, msecs, true);
#else
    return false;
#endif
}

PluginBridgeRtClientOpcode BridgeRtClientControl::readOpcode() noexcept
{
#ifdef BUILD_BRIDGE
    return static_cast<PluginBridgeRtClientOpcode>(readUInt());
#else
    return kPluginBridgeRtClientNull;
#endif
}

void BridgeRtClientControl::writeOpcode(const PluginBridgeRtClientOpcode opcode) noexcept
{
#ifndef BUILD_BRIDGE
    writeUInt(static_cast<uint32_t>(opcode));
#endif
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
