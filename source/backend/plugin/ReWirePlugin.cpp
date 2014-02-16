/*
 * Carla ReWire Plugin
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef CARLA_OS_WIN64
# undef WANT_REWIRE
#endif

#ifdef WANT_REWIRE

#include "CarlaLibUtils.hpp"
#include "CarlaMathUtils.hpp"

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

// -----------------------------------------------------------------------
// ReWire assumes sizeof(long) == 4

typedef int32_t rlong;
typedef uint32_t rulong;

static_assert(sizeof(rlong) == 4, "Incorrect rlong size");
static_assert(sizeof(rulong) == 4, "Incorrect rulong size");

// rewire is not for linux, so for easy testing+development, we can skip this
#ifndef CARLA_OS_LINUX
static_assert(sizeof(long) == 4, "Incorrect long size");
static_assert(sizeof(ulong) == 4, "Incorrect ulong size");
static_assert(sizeof(long) == sizeof(rlong), "long size mismatch");
static_assert(sizeof(ulong) == sizeof(rulong), "ulon size mismatch");
#endif

struct RwOpenInfo {
    rulong size1; // 16
    rulong size2; // 12
    rlong sampleRate;
    rlong bufferSize;
};

struct RwDevInfo {
    rulong size; // 8288
    char name[32];
    rlong channelCount; // max limited to 255
    char channelNames[256][32];
    rulong defaultChannels[8];
    rulong stereoPairs[4];
    rulong eventBufferSize;
    rlong version;
};

struct RwAudioInfo {
    rulong size; // 12
    rlong sampleRate;
    rlong bufferSize;
};

struct RwBusInfo {
    rulong size; // 40
    rulong channel;
    char name[32];
};

struct RwEventInfo {
    rulong size; // 132 FIXME?
    rulong bus[32];
};

struct RwEvent { // 24
    ushort type;
    uchar d1, d2;
    rulong s1, s2, s3, s4, s5;
};

struct RwEventBuffer {
    rulong size1; // 20
    rulong size2; // 24 (of RwEvent)
    rulong count;
    rulong maxCount;
#ifndef CARLA_OS_LINUX // pointers on 64bit linux are size 8, skip this
    RwEvent* buf;
#else
    rulong buf;
#endif
};

struct RwAudioInInfo {
    rulong size; // 1116
    RwEventBuffer evBuf;
    rulong channels[8];
#ifndef CARLA_OS_LINUX // pointers on 64bit linux are size 8, skip this
    float* audioBuf[256];
#else
    rulong audioBuf[256];
#endif
    rlong tickStart;
    rulong frames;
    rulong playMode;
    rulong tempo; // bpm
    rulong signNumerator;
    rulong signDenominator;
    rlong loopStartPos;
    rlong loopEndPos;
    rulong loopOn;
};

struct RwAudioOutInfo {
    rulong size; // 56
    RwEventBuffer evBuf;
    rulong channels[8];
};

static_assert(sizeof(RwOpenInfo) == 16, "Incorrect ReWire struct size");
static_assert(sizeof(RwDevInfo) == 8288, "Incorrect ReWire struct size");
static_assert(sizeof(RwAudioInfo) == 12, "Incorrect ReWire struct size");
static_assert(sizeof(RwBusInfo) == 40, "Incorrect ReWire struct size");
static_assert(sizeof(RwEventInfo) == 132, "Incorrect ReWire struct size");
static_assert(sizeof(RwEvent) == 24, "Incorrect ReWire struct size");
static_assert(sizeof(RwEventBuffer) == 20, "Incorrect ReWire struct size");
static_assert(sizeof(RwAudioInInfo) == 1116, "Incorrect ReWire struct size");
static_assert(sizeof(RwAudioOutInfo) == 56, "Incorrect ReWire struct size");

// -----------------------------------------------------------------------

typedef void (*Fn_RWDEFCloseDevice)();
typedef void (*Fn_RWDEFDriveAudio)(RwAudioInInfo* in, RwAudioOutInfo* out);
typedef void (*Fn_RWDEFGetDeviceInfo)(RwDevInfo* info);
typedef void (*Fn_RWDEFGetDeviceNameAndVersion)(long* version, char* name);
typedef void (*Fn_RWDEFGetEventBusInfo)(ushort index, RwBusInfo* info);
typedef void (*Fn_RWDEFGetEventChannelInfo)(void* v1, void* v2);
typedef void (*Fn_RWDEFGetEventControllerInfo)(void* v1, ushort index, void* v2);
typedef void (*Fn_RWDEFGetEventInfo)(RwEventInfo* info);
typedef void (*Fn_RWDEFGetEventNoteInfo)(void* v1, ushort index, void* v2);
typedef void (*Fn_RWDEFIdle)();
typedef char (*Fn_RWDEFIsCloseOK)();
typedef char (*Fn_RWDEFIsPanelAppLaunched)();
typedef int  (*Fn_RWDEFLaunchPanelApp)();
typedef int  (*Fn_RWDEFOpenDevice)(RwOpenInfo* info);
typedef int  (*Fn_RWDEFQuitPanelApp)();
typedef void (*Fn_RWDEFSetAudioInfo)(RwAudioInfo* info);

// -----------------------------------------------------------------------------

struct RewireBridge {
    void* lib;

    Fn_RWDEFCloseDevice RWDEFCloseDevice;
    Fn_RWDEFDriveAudio RWDEFDriveAudio;
    Fn_RWDEFGetDeviceInfo RWDEFGetDeviceInfo;
    Fn_RWDEFGetDeviceNameAndVersion RWDEFGetDeviceNameAndVersion;
    Fn_RWDEFGetEventBusInfo RWDEFGetEventBusInfo;
    Fn_RWDEFGetEventChannelInfo RWDEFGetEventChannelInfo;
    Fn_RWDEFGetEventControllerInfo RWDEFGetEventControllerInfo;
    Fn_RWDEFGetEventInfo RWDEFGetEventInfo;
    Fn_RWDEFGetEventNoteInfo RWDEFGetEventNoteInfo;
    Fn_RWDEFIdle RWDEFIdle;
    Fn_RWDEFIsCloseOK RWDEFIsCloseOK;
    Fn_RWDEFIsPanelAppLaunched RWDEFIsPanelAppLaunched;
    Fn_RWDEFLaunchPanelApp RWDEFLaunchPanelApp;
    Fn_RWDEFOpenDevice RWDEFOpenDevice;
    Fn_RWDEFQuitPanelApp RWDEFQuitPanelApp;
    Fn_RWDEFSetAudioInfo RWDEFSetAudioInfo;

    RewireBridge()
        : lib(nullptr),
          RWDEFCloseDevice(nullptr),
          RWDEFDriveAudio(nullptr),
          RWDEFGetDeviceInfo(nullptr),
          RWDEFGetDeviceNameAndVersion(nullptr),
          RWDEFGetEventBusInfo(nullptr),
          RWDEFGetEventChannelInfo(nullptr),
          RWDEFGetEventControllerInfo(nullptr),
          RWDEFGetEventInfo(nullptr),
          RWDEFGetEventNoteInfo(nullptr),
          RWDEFIdle(nullptr),
          RWDEFIsCloseOK(nullptr),
          RWDEFIsPanelAppLaunched(nullptr),
          RWDEFLaunchPanelApp(nullptr),
          RWDEFOpenDevice(nullptr),
          RWDEFQuitPanelApp(nullptr),
          RWDEFSetAudioInfo(nullptr) {}

    ~RewireBridge()
    {
        cleanup();
    }

    int init(const char* const filename)
    {
        CARLA_SAFE_ASSERT_RETURN(lib == nullptr, -2);
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', -2);

        lib = lib_open(filename);

        if (lib == nullptr)
            return -1;

        #define JOIN(a, b) a ## b
        #define LIB_SYMBOL(NAME) NAME = (Fn_##NAME)lib_symbol(lib, #NAME);
        //if (NAME == nullptr) cleanup(); return -2;

        LIB_SYMBOL(RWDEFCloseDevice)
        LIB_SYMBOL(RWDEFDriveAudio)
        LIB_SYMBOL(RWDEFGetDeviceInfo)
        LIB_SYMBOL(RWDEFGetDeviceNameAndVersion)
        LIB_SYMBOL(RWDEFGetEventBusInfo)
        LIB_SYMBOL(RWDEFGetEventChannelInfo)
        LIB_SYMBOL(RWDEFGetEventControllerInfo)
        LIB_SYMBOL(RWDEFGetEventInfo)
        LIB_SYMBOL(RWDEFGetEventNoteInfo)
        LIB_SYMBOL(RWDEFIdle)
        LIB_SYMBOL(RWDEFIsCloseOK)
        LIB_SYMBOL(RWDEFIsPanelAppLaunched)
        LIB_SYMBOL(RWDEFLaunchPanelApp)
        LIB_SYMBOL(RWDEFOpenDevice)
        LIB_SYMBOL(RWDEFQuitPanelApp)
        LIB_SYMBOL(RWDEFSetAudioInfo)

        #undef JOIN
        #undef LIB_SYMBOL

        return 0;
    }

    void cleanup()
    {
        if (lib != nullptr)
        {
            lib_close(lib);
            lib = nullptr;
        }

        RWDEFCloseDevice = nullptr;
        RWDEFDriveAudio = nullptr;
        RWDEFGetDeviceInfo = nullptr;
        RWDEFGetDeviceNameAndVersion = nullptr;
        RWDEFGetEventBusInfo = nullptr;
        RWDEFGetEventChannelInfo = nullptr;
        RWDEFGetEventControllerInfo = nullptr;
        RWDEFGetEventInfo = nullptr;
        RWDEFGetEventNoteInfo = nullptr;
        RWDEFIdle = nullptr;
        RWDEFIsCloseOK = nullptr;
        RWDEFIsPanelAppLaunched = nullptr;
        RWDEFLaunchPanelApp = nullptr;
        RWDEFOpenDevice = nullptr;
        RWDEFQuitPanelApp = nullptr;
        RWDEFSetAudioInfo = nullptr;
    }
};

// -----------------------------------------------------

class ReWirePlugin : public CarlaPlugin
{
public:
    ReWirePlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fIsOpen(false),
          fIsPanelLaunched(false),
          fLabel(nullptr)
    {
        carla_debug("ReWirePlugin::ReWirePlugin(%p, %i)", engine, id);

        carla_zeroStruct<RwAudioInInfo>(fRwAudioIn);
        fRwAudioIn.size = sizeof(RwAudioInInfo);

        carla_zeroStruct<RwAudioOutInfo>(fRwAudioOut);
        fRwAudioOut.size = sizeof(RwAudioOutInfo);
    }

    ~ReWirePlugin() override
    {
        carla_debug("ReWirePlugin::~ReWirePlugin()");

        // close panel
        if (fIsPanelLaunched)
        {
            fRw.RWDEFQuitPanelApp();
            fIsPanelLaunched = false;
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fIsOpen)
        {
            for (; fRw.RWDEFIsCloseOK() == 0;)
                carla_msleep(2);

            fRw.RWDEFCloseDevice();
            fIsOpen = false;
        }

        if (fLabel != nullptr)
        {
            delete[] fLabel;
            fLabel = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_REWIRE;
    }

    PluginCategory getCategory() const noexcept override
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t getChunkData(void** const dataPtr) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        // TODO

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getOptionsAvailable() const noexcept override
    {
        unsigned int options = 0x0;

        if (getMidiInCount() > 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // TODO

        return 0.0f;
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fLabel != nullptr,)

        std::strcpy(strBuf, fLabel);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fLabel != nullptr,)

        std::strcpy(strBuf, fLabel);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        // TODO
        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        // TODO
        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));

        // TODO

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setChunkData(const char* const stringData) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(stringData != nullptr,);

        // TODO
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo)
        {
            if (! fRw.RWDEFIsPanelAppLaunched())
                fRw.RWDEFLaunchPanelApp();
            fIsPanelLaunched = true;
        }
        else
        {
            if (fRw.RWDEFIsPanelAppLaunched())
                fRw.RWDEFQuitPanelApp();
            fIsPanelLaunched = false;
        }
    }

    void idle() override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);

        fRw.RWDEFIdle();

        // check if panel has been closed
        if (fIsPanelLaunched && ! fRw.RWDEFIsPanelAppLaunched())
        {
            // FIXME
            //fIsPanelLaunched = true;
            //pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
//             static int counter = 0;
//
//             if (counter % 1000)
//             {
//                 carla_stdout("Panel is closed?");
//             }
//
//             ++counter;
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);
        carla_debug("ReWirePlugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t aIns, aOuts, mIns, mOuts, params;
        aIns = aOuts = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        RwDevInfo devInfo;
        carla_zeroStruct<RwDevInfo>(devInfo);
        devInfo.size = sizeof(RwDevInfo);

        fRw.RWDEFGetDeviceInfo(&devInfo);

        if (devInfo.channelCount > 0)
            aIns = aOuts = static_cast<uint32_t>(devInfo.channelCount);

        mIns = mOuts = 0; // TODO, should always be 1

        params = 0; // TODO?

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "i";
            portName += devInfo.channelNames[j];

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "o";
            portName += devInfo.channelNames[j];
            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[j].rindex = j;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin hints
        pData->hints = 0x0;

        pData->hints |= PLUGIN_IS_SYNTH;
        pData->hints |= PLUGIN_HAS_CUSTOM_UI;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());

        if (pData->active)
            activate();

        carla_debug("ReWirePlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);

        // TODO
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRw.lib != nullptr,);

        // TODO
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FLOAT_CLEAR(outBuffer[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FLOAT_CLEAR(pData->latencyBuffers[i], pData->latency);
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        //fRwAudioIn.playMode; // ???
        //fRwAudioIn.frames = timeInfo.frame; // not sure if buf or tranport frames

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
        {
            double ppqBar  = double(timeInfo.bbt.bar - 1) * timeInfo.bbt.beatsPerBar;
            double ppqBeat = double(timeInfo.bbt.beat - 1);
            double ppqTick = double(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat;

            // PPQ Pos, ???
            fRwAudioIn.tickStart = static_cast<rlong>(ppqBar + ppqBeat + ppqTick);

            // Tempo
            fRwAudioIn.tempo = static_cast<rulong>(timeInfo.bbt.beatsPerMinute);

            // Bars
            //fTimeInfo.barStartPos = ppqBar;

            // Time Signature
            fRwAudioIn.signNumerator   = static_cast<rulong>(timeInfo.bbt.beatsPerBar);
            fRwAudioIn.signDenominator = static_cast<rulong>(timeInfo.bbt.beatType);
        }
        else
        {
            fRwAudioIn.tickStart       = 0;
            fRwAudioIn.tempo           = 120;
            fRwAudioIn.signNumerator   = 4;
            fRwAudioIn.signDenominator = 4;
        }

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        {
            processSingle(inBuffer, outBuffer, frames, 0);

        } // End of Plugin processing (no events)
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        //rulong channels[8];
        fRwAudioIn.frames = frames;

#ifndef CARLA_OS_LINUX
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            fRwAudioIn.audioBuf[i] = outBuffer[i]+timeOffset;
            FLOAT_COPY(fRwAudioIn.audioBuf[i], inBuffer[i]+timeOffset, frames);
        }
#endif

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fRw.RWDEFDriveAudio(&fRwAudioIn, &fRwAudioOut);

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && pData->postProc.volume != 1.0f;
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = inBuffer[(pData->audioIn.count == 1) ? 0 : i][k+timeOffset];
                        outBuffer[i][k+timeOffset] = (outBuffer[i][k+timeOffset] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        FLOAT_COPY(oldBufLeft, outBuffer[i]+timeOffset, frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            outBuffer[i][k+timeOffset]  = oldBufLeft[k]                * (1.0f - balRangeL);
                            outBuffer[i][k+timeOffset] += outBuffer[i+1][k+timeOffset] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k+timeOffset]  = outBuffer[i][k+timeOffset] * balRangeR;
                            outBuffer[i][k+timeOffset] += oldBufLeft[k]              * balRangeL;
                        }
                    }
                }

                // Volume
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("ReWirePlugin::bufferSizeChanged(%i)", newBufferSize);

        if (pData->active)
            deactivate();

        RwAudioInfo audioInfo;
        audioInfo.size = sizeof(RwAudioInfo);
        audioInfo.bufferSize = static_cast<rlong>(newBufferSize);
        audioInfo.sampleRate = static_cast<rlong>(pData->engine->getSampleRate());

        fRw.RWDEFSetAudioInfo(&audioInfo);

        if (pData->active)
            activate();
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("ReWirePlugin::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
            deactivate();

        RwAudioInfo audioInfo;
        audioInfo.size = sizeof(RwAudioInfo);
        audioInfo.bufferSize = static_cast<rlong>(pData->engine->getBufferSize());
        audioInfo.sampleRate = static_cast<rlong>(newSampleRate);

        fRw.RWDEFSetAudioInfo(&audioInfo);

        if (pData->active)
            activate();
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    // TODO

    // -------------------------------------------------------------------

public:
    bool init(const char* const filename, const char* const name)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        int ret = fRw.init(filename);

        if (ret != 0)
        {
            if (ret == -1)
                pData->engine->setLastError(lib_error(filename));
            else
                pData->engine->setLastError("Not a valid ReWire application");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        long version;
        char nameBuf[STR_MAX+1];
        carla_zeroChar(nameBuf, STR_MAX+1);
        fRw.RWDEFGetDeviceNameAndVersion(&version, nameBuf);

        if (nameBuf[0] == '\0')
        {
            pData->engine->setLastError("ReWire application has no name");
            return false;
        }

        fLabel = carla_strdup(nameBuf);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(nameBuf);

        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize app

        RwOpenInfo info;
        info.size1 = sizeof(RwOpenInfo);
        info.size2 = 12;
        info.bufferSize = static_cast<rlong>(pData->engine->getBufferSize());
        info.sampleRate = static_cast<rlong>(pData->engine->getSampleRate());

        ret = fRw.RWDEFOpenDevice(&info);

        carla_stdout("RW open ret = %i", ret);

        // TODO check ret
        fIsOpen = true;

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

            // TODO
            if (getMidiInCount() > 0)
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // set identifier string
            CarlaString identifier("ReWire/");
            identifier += fLabel;
            pData->identifier = identifier.dup();

            // load settings
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());

            // ignore settings, we need this anyway
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;
        }

        return true;
    }

private:
    RewireBridge fRw;
    RwAudioInInfo  fRwAudioIn;
    RwAudioOutInfo fRwAudioOut;

    bool fIsOpen;
    bool fIsPanelLaunched;
    const char* fLabel;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReWirePlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif // WANT_REWIRE

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newReWire(const Initializer& init)
{
    carla_debug("CarlaPlugin::newReWire({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.uniqueId);

#ifdef WANT_REWIRE
    ReWirePlugin* const plugin(new ReWirePlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo ReWire applications, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("ReWire support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
