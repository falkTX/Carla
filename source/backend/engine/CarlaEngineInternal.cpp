/*
 * Carla Plugin Host
 * Copyright (C) 2011-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "jackbridge/JackBridge.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine Internal helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); lastError = err; return false;   }
#define CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); lastError = err; return nullptr; }

// -----------------------------------------------------------------------
// InternalEvents

EngineInternalEvents::EngineInternalEvents() noexcept
    : in(nullptr),
      out(nullptr) {}

EngineInternalEvents::~EngineInternalEvents() noexcept
{
    CARLA_SAFE_ASSERT(in == nullptr);
    CARLA_SAFE_ASSERT(out == nullptr);
}

void EngineInternalEvents::clear() noexcept
{
    if (in != nullptr)
    {
        delete[] in;
        in = nullptr;
    }

    if (out != nullptr)
    {
        delete[] out;
        out = nullptr;
    }
}

// -----------------------------------------------------------------------
// InternalTime

static const float kTicksPerBeat = 1920.0f;

EngineInternalTime::EngineInternalTime(EngineTimeInfo& ti, const EngineTransportMode& tm) noexcept
    : bpm(120.0),
      sampleRate(0.0),
      tick(0.0),
      needsReset(true),
      timeInfo(ti),
      transportMode(tm) {}

EngineInternalTime::~EngineInternalTime() noexcept
{
#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
    hylia_cleanup(hylia.instance);
#endif
}

void EngineInternalTime::init(const uint32_t bufferSize, const double sr)
{
    sampleRate = sr;

#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
    hylia.instance = hylia_create(bpm, bufferSize, sr);

    if (hylia.enabled)
        hylia_enable(hylia.instance, true, bpm);
#endif

    fillEngineTimeInfo(0);
}

void EngineInternalTime::updateAudioValues(const uint32_t bufferSize, double sampleRate)
{
    // TODO
}

void EngineInternalTime::preProcess(const uint32_t numFrames)
{
#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
    if (hylia.enabled)
    {
        hylia_process(hylia.instance, numFrames, &hylia.timeInfo);
        const double new_bpm = hylia.timeInfo.bpm;

        if (new_bpm > 0.0 && bpm != new_bpm)
        {
            bpm = new_bpm;
            needsReset = true;

            if (transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
                fillEngineTimeInfo(0);
        }
    }
#else
    return;

    // unused
    (void)numFrames;
#endif
}

void EngineInternalTime::postProcess(const uint32_t numFrames)
{
    if (timeInfo.playing && transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
    {
        timeInfo.frame += numFrames;
        fillEngineTimeInfo(numFrames);
    }
}

void EngineInternalTime::enableLink(const bool enable)
{
#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
    if (hylia.enabled == enable)
        return;

    hylia.enabled = enable;

    if (hylia.instance != nullptr)
        hylia_enable(hylia.instance, enable, bpm);
#else
    return;

    // unused
    (void)enable;
#endif
}

void EngineInternalTime::setNeedsReset()
{
    needsReset = true;
}

void EngineInternalTime::fillEngineTimeInfo(const uint32_t newFrames) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_isNotZero(sampleRate),);

    timeInfo.usecs = 0;

    if (newFrames == 0 || needsReset)
    {
        timeInfo.valid = EngineTimeInfo::kValidBBT;
        timeInfo.bbt.beatsPerBar = 4.0f;
        timeInfo.bbt.beatType = 4.0f;
        timeInfo.bbt.ticksPerBeat = kTicksPerBeat;
        timeInfo.bbt.beatsPerMinute = bpm;

        double abs_beat, abs_tick;

#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
        if (hylia.enabled && hylia.timeInfo.beats >= 0.0)
        {
            const double beats = hylia.timeInfo.beats;

            abs_beat = std::floor(beats);
            abs_tick = beats * kTicksPerBeat;
        }
        else
#endif
        {
            const double min = timeInfo.frame / (sampleRate * 60.0);
            abs_tick = min * bpm * kTicksPerBeat;
            abs_beat = abs_tick / kTicksPerBeat;
        }

        timeInfo.bbt.bar  = abs_beat / timeInfo.bbt.beatsPerBar;
        timeInfo.bbt.beat = abs_beat - (timeInfo.bbt.bar * timeInfo.bbt.beatsPerBar) + 1;
        tick              = abs_tick - (abs_beat * kTicksPerBeat);
        timeInfo.bbt.barStartTick = timeInfo.bbt.bar * timeInfo.bbt.beatsPerBar * kTicksPerBeat;
        timeInfo.bbt.bar++;
    }
    else
    {
        tick += newFrames * kTicksPerBeat * bpm / (sampleRate * 60);

        while (tick >= kTicksPerBeat)
        {
            tick -= kTicksPerBeat;

            if (++timeInfo.bbt.beat > timeInfo.bbt.beatsPerBar)
            {
                timeInfo.bbt.beat = 1;
                ++timeInfo.bbt.bar;
                timeInfo.bbt.barStartTick += timeInfo.bbt.beatsPerBar * kTicksPerBeat;
            }
        }
    }

    timeInfo.bbt.tick = (int)(tick + 0.5);
    needsReset = false;
}

void EngineInternalTime::fillJackTimeInfo(jack_position_t* const pos, const uint32_t newFrames) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_isNotZero(sampleRate),);

    if (newFrames == 0 || needsReset)
    {
        pos->valid = JackPositionBBT;
        pos->beats_per_bar = 4.0f;
        pos->beat_type = 4.0f;
        pos->ticks_per_beat = kTicksPerBeat;
        pos->beats_per_minute = bpm;

        double abs_beat, abs_tick;

#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
        if (hylia.enabled && hylia.timeInfo.beats >= 0.0)
        {
            const double beats = hylia.timeInfo.beats;

            abs_beat = std::floor(beats);
            abs_tick = beats * kTicksPerBeat;
        }
        else
#endif
        {
            const double min = pos->frame / (sampleRate * 60.0);
            abs_tick = min * bpm * kTicksPerBeat;
            abs_beat = abs_tick / kTicksPerBeat;
        }

        pos->bar  = abs_beat / pos->beats_per_bar;
        pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
        tick      = abs_tick - (abs_beat * kTicksPerBeat);
        pos->bar_start_tick = pos->bar * pos->beats_per_bar * kTicksPerBeat;
        pos->bar++;
    }
    else
    {
        tick += newFrames * kTicksPerBeat * bpm / (sampleRate * 60);

        while (tick >= kTicksPerBeat)
        {
            tick -= kTicksPerBeat;

            if (++pos->beat > pos->beats_per_bar)
            {
                pos->beat = 1;
                ++pos->bar;
                pos->bar_start_tick += pos->beats_per_bar * pos->ticks_per_beat;
            }
        }
    }

    pos->tick = (int)(tick + 0.5);
    needsReset = false;
}

#ifndef BUILD_BRIDGE
EngineInternalTime::Hylia::Hylia()
    : enabled(false),
      instance(nullptr)
{
    carla_zeroStruct(timeInfo);
}
#endif

// -----------------------------------------------------------------------
// NextAction

EngineNextAction::EngineNextAction() noexcept
    : opcode(kEnginePostActionNull),
      pluginId(0),
      value(0),
      mutex(false) {}

EngineNextAction::~EngineNextAction() noexcept
{
    CARLA_SAFE_ASSERT(opcode == kEnginePostActionNull);
}

void EngineNextAction::ready() const noexcept
{
    mutex.lock();
    mutex.unlock();
}

void EngineNextAction::clearAndReset() noexcept
{
    mutex.lock();
    opcode   = kEnginePostActionNull;
    pluginId = 0;
    value    = 0;
    mutex.unlock();
}

// -----------------------------------------------------------------------
// CarlaEngine::ProtectedData

CarlaEngine::ProtectedData::ProtectedData(CarlaEngine* const engine) noexcept
    : thread(engine),
#ifdef HAVE_LIBLO
      osc(engine),
      oscData(nullptr),
#endif
      callback(nullptr),
      callbackPtr(nullptr),
      fileCallback(nullptr),
      fileCallbackPtr(nullptr),
#ifndef BUILD_BRIDGE
      firstLinuxSamplerInstance(true),
      loadingProject(false),
#endif
      hints(0x0),
      bufferSize(0),
      sampleRate(0.0),
      aboutToClose(false),
      isIdling(0),
      curPluginCount(0),
      maxPluginNumber(0),
      nextPluginId(0),
      envMutex(),
      lastError(),
      name(),
      options(),
      timeInfo(),
#ifndef BUILD_BRIDGE
      plugins(nullptr),
#endif
      events(),
#ifndef BUILD_BRIDGE
      graph(engine),
#endif
      time(timeInfo, options.transportMode),
      nextAction()
{
#ifdef BUILD_BRIDGE
    carla_zeroStructs(plugins, 1);
#endif
}

CarlaEngine::ProtectedData::~ProtectedData() noexcept
{
    CARLA_SAFE_ASSERT(curPluginCount == 0);
    CARLA_SAFE_ASSERT(maxPluginNumber == 0);
    CARLA_SAFE_ASSERT(nextPluginId == 0);
    CARLA_SAFE_ASSERT(isIdling == 0);
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT(plugins == nullptr);
#endif
}

// -----------------------------------------------------------------------

bool CarlaEngine::ProtectedData::init(const char* const clientName)
{
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(name.isEmpty(), "Invalid engine internal data (err #1)");
#ifdef HAVE_LIBLO
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(oscData == nullptr, "Invalid engine internal data (err #2)");
#endif
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(events.in  == nullptr, "Invalid engine internal data (err #4)");
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(events.out == nullptr, "Invalid engine internal data (err #5)");
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(clientName != nullptr && clientName[0] != '\0', "Invalid client name");
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN_INTERNAL_ERR(plugins == nullptr, "Invalid engine internal data (err #3)");
#endif

    aboutToClose   = false;
    curPluginCount = 0;
    nextPluginId   = 0;

    switch (options.processMode)
    {
    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        maxPluginNumber = MAX_RACK_PLUGINS;
        options.forceStereo = true;
        break;
    case ENGINE_PROCESS_MODE_PATCHBAY:
        maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;
    case ENGINE_PROCESS_MODE_BRIDGE:
        maxPluginNumber = 1;
        break;
    default:
        maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;
    }

    switch (options.processMode)
    {
    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
    case ENGINE_PROCESS_MODE_PATCHBAY:
    case ENGINE_PROCESS_MODE_BRIDGE:
        events.in  = new EngineEvent[kMaxEngineEventInternalCount];
        events.out = new EngineEvent[kMaxEngineEventInternalCount];
        break;
    default:
        break;
    }

    nextPluginId = maxPluginNumber;

    name = clientName;
    name.toBasic();

    timeInfo.clear();

#ifdef HAVE_LIBLO
    osc.init(clientName);
# ifndef BUILD_BRIDGE
    oscData = osc.getControlData();
# endif
#endif

#ifndef BUILD_BRIDGE
    plugins = new EnginePluginData[maxPluginNumber];
    carla_zeroStructs(plugins, maxPluginNumber);
#endif

    nextAction.ready();
    thread.startThread();

    return true;
}

void CarlaEngine::ProtectedData::close()
{
    CARLA_SAFE_ASSERT(name.isNotEmpty());
    CARLA_SAFE_ASSERT(plugins != nullptr);
    CARLA_SAFE_ASSERT(nextPluginId == maxPluginNumber);
    CARLA_SAFE_ASSERT(nextAction.opcode == kEnginePostActionNull);

    aboutToClose = true;

    thread.stopThread(500);
    nextAction.ready();

#ifdef HAVE_LIBLO
    osc.close();
    oscData = nullptr;
#endif

    aboutToClose    = false;
    curPluginCount  = 0;
    maxPluginNumber = 0;
    nextPluginId    = 0;

#ifndef BUILD_BRIDGE
    if (plugins != nullptr)
    {
        delete[] plugins;
        plugins = nullptr;
    }
#endif

    events.clear();
    name.clear();
}

void CarlaEngine::ProtectedData::initTime(const char* const features)
{
    time.init(bufferSize, sampleRate);

#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
    const bool linkEnabled = features != nullptr && std::strstr(features, ":link:") != nullptr;
    time.enableLink(linkEnabled);
#endif
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void CarlaEngine::ProtectedData::doPluginRemove() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(curPluginCount > 0,);
    CARLA_SAFE_ASSERT_RETURN(nextAction.pluginId < curPluginCount,);
    --curPluginCount;

    // move all plugins 1 spot backwards
    for (uint i=nextAction.pluginId; i < curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(plugins[i+1].plugin);

        CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

        plugin->setId(i);

        plugins[i].plugin      = plugin;
        plugins[i].insPeak[0]  = 0.0f;
        plugins[i].insPeak[1]  = 0.0f;
        plugins[i].outsPeak[0] = 0.0f;
        plugins[i].outsPeak[1] = 0.0f;
    }

    const uint id(curPluginCount);

    // reset last plugin (now removed)
    plugins[id].plugin      = nullptr;
    plugins[id].insPeak[0]  = 0.0f;
    plugins[id].insPeak[1]  = 0.0f;
    plugins[id].outsPeak[0] = 0.0f;
    plugins[id].outsPeak[1] = 0.0f;
}

void CarlaEngine::ProtectedData::doPluginsSwitch() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(curPluginCount >= 2,);

    const uint idA(nextAction.pluginId);
    const uint idB(nextAction.value);

    CARLA_SAFE_ASSERT_RETURN(idA < curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(idB < curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(plugins[idA].plugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugins[idB].plugin != nullptr,);

#if 0
    std::swap(plugins[idA].plugin, plugins[idB].plugin);
#else
    CarlaPlugin* const tmp(plugins[idA].plugin);

    plugins[idA].plugin = plugins[idB].plugin;
    plugins[idB].plugin = tmp;
#endif
}
#endif

void CarlaEngine::ProtectedData::doNextPluginAction(const bool unlock) noexcept
{
    switch (nextAction.opcode)
    {
    case kEnginePostActionNull:
        break;
    case kEnginePostActionZeroCount:
        curPluginCount = 0;
        break;
#ifndef BUILD_BRIDGE
    case kEnginePostActionRemovePlugin:
        doPluginRemove();
        break;
    case kEnginePostActionSwitchPlugins:
        doPluginsSwitch();
        break;
#endif
    }

    nextAction.opcode   = kEnginePostActionNull;
    nextAction.pluginId = 0;
    nextAction.value    = 0;

    if (unlock)
    {
        nextAction.mutex.tryLock();
        nextAction.mutex.unlock();
    }
}

// -----------------------------------------------------------------------
// PendingRtEventsRunner

PendingRtEventsRunner::PendingRtEventsRunner(CarlaEngine* const engine, const uint32_t frames) noexcept
    : pData(engine->pData),
      numFrames(frames)
{
    pData->time.preProcess(frames);
}

PendingRtEventsRunner::~PendingRtEventsRunner() noexcept
{
    pData->doNextPluginAction(true);

    if (pData->timeInfo.playing && pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
    {
        pData->timeInfo.frame += numFrames;
        pData->time.fillEngineTimeInfo(numFrames);
    }
}

// -----------------------------------------------------------------------
// ScopedActionLock

ScopedActionLock::ScopedActionLock(CarlaEngine* const engine, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept
    : pData(engine->pData)
{
    CARLA_SAFE_ASSERT_RETURN(action != kEnginePostActionNull,);

    pData->nextAction.mutex.lock();

    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull,);

    pData->nextAction.opcode   = action;
    pData->nextAction.pluginId = pluginId;
    pData->nextAction.value    = value;

    if (lockWait)
    {
        // block wait for unlock on processing side
        carla_stdout("ScopedPluginAction(%i) - blocking START", pluginId);
        pData->nextAction.mutex.lock();
        carla_stdout("ScopedPluginAction(%i) - blocking DONE", pluginId);
    }
    else
    {
        pData->doNextPluginAction(false);
    }
}

ScopedActionLock::~ScopedActionLock() noexcept
{
    CARLA_SAFE_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    pData->nextAction.mutex.tryLock();
    pData->nextAction.mutex.unlock();
}

// -----------------------------------------------------------------------
// ScopedThreadStopper

ScopedThreadStopper::ScopedThreadStopper(CarlaEngine* const e) noexcept
    : engine(e),
      pData(e->pData)
{
    pData->thread.stopThread(500);
}

ScopedThreadStopper::~ScopedThreadStopper() noexcept
{
    if (engine->isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();
}

// -----------------------------------------------------------------------
// ScopedEngineEnvironmentLocker

ScopedEngineEnvironmentLocker::ScopedEngineEnvironmentLocker(CarlaEngine* const engine) noexcept
    : pData(engine->pData)
{
    pData->envMutex.lock();
}

ScopedEngineEnvironmentLocker::~ScopedEngineEnvironmentLocker() noexcept
{
    pData->envMutex.unlock();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
