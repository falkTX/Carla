/*
 * Carla Native Plugins
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#define CARLA_NATIVE_PLUGIN_LV2
#include "carla-native-base.cpp"

#include "CarlaMathUtils.hpp"
#include "juce_core.h"

#ifdef VESTIGE_HEADER
# include "vestige/aeffectx.h"
#define effFlagsProgramChunks (1 << 5)
#define effGetParamLabel 6
#define effGetChunk 23
#define effSetChunk 24
#define effGetPlugCategory 35
#define kPlugCategEffect 1
#define kPlugCategSynth 2
#define kVstVersion 2400
struct ERect {
    int16_t top, left, bottom, right;
};
#else
# include "vst/aeffectx.h"
#endif

uint32_t d_lastBufferSize = 0;
double   d_lastSampleRate = 0.0;

// -----------------------------------------------------------------------

class NativePlugin
{
public:
    static const uint32_t kMaxMidiEvents = 512;

    NativePlugin(const audioMasterCallback audioMaster, AEffect* const effect, const NativePluginDescriptor* desc)
        : fAudioMaster(audioMaster),
          fEffect(effect),
          fHandle(nullptr),
          fHost(),
          fDescriptor(desc),
          fBufferSize(d_lastBufferSize),
          fSampleRate(d_lastSampleRate),
          fMidiEventCount(0),
          fTimeInfo(),
          fVstRect(),
          fStateChunk(nullptr),
          leakDetector_NativePlugin()
    {
        fHost.handle      = this;
        fHost.uiName      = carla_strdup("CarlaVST");
        fHost.uiParentId  = 0;

        // find resource dir
        using juce::File;

        File curExe = File::getSpecialLocation(File::currentExecutableFile).getLinkedTarget();
        File resDir = curExe.getSiblingFile("carla-resources");
        if (! resDir.exists())
            resDir = curExe.getSiblingFile("resources");
        if (! resDir.exists())
            resDir = File("/usr/share/carla/resources/");

        fHost.resourceDir = carla_strdup(resDir.getFullPathName().toRawUTF8());

        fHost.get_buffer_size        = host_get_buffer_size;
        fHost.get_sample_rate        = host_get_sample_rate;
        fHost.is_offline             = host_is_offline;
        fHost.get_time_info          = host_get_time_info;
        fHost.write_midi_event       = host_write_midi_event;
        fHost.ui_parameter_changed   = host_ui_parameter_changed;
        fHost.ui_custom_data_changed = host_ui_custom_data_changed;
        fHost.ui_closed              = host_ui_closed;
        fHost.ui_open_file           = host_ui_open_file;
        fHost.ui_save_file           = host_ui_save_file;
        fHost.dispatcher             = host_dispatcher;

        fVstRect.top    = 0;
        fVstRect.left   = 0;
        fVstRect.bottom = 512;
        fVstRect.right  = 740;

        init();
    }

    ~NativePlugin()
    {
        if (fDescriptor->cleanup != nullptr && fHandle != nullptr)
            fDescriptor->cleanup(fHandle);

        fHandle = nullptr;

        if (fStateChunk != nullptr)
        {
            std::free(fStateChunk);
            fStateChunk = nullptr;
        }

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }

        if (fHost.resourceDir != nullptr)
        {
            delete[] fHost.resourceDir;
            fHost.resourceDir = nullptr;
        }
    }

    bool init()
    {
        if (fDescriptor->instantiate == nullptr || fDescriptor->process == nullptr)
        {
            carla_stderr("Plugin is missing something...");
            return false;
        }

        fHandle = fDescriptor->instantiate(&fHost);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);

        carla_zeroStruct<NativeMidiEvent>(fMidiEvents, kMaxMidiEvents);
        carla_zeroStruct<NativeTimeInfo>(fTimeInfo);

        return true;
    }

    // -------------------------------------------------------------------

    intptr_t vst_dispatcher(const int32_t opcode, const int32_t /*index*/, const intptr_t value, void* const ptr, const float opt)
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0);

        intptr_t ret = 0;

        switch (opcode)
        {
        case effSetSampleRate:
            if (carla_compareFloats(fSampleRate, static_cast<double>(opt)))
                return 0;

            fSampleRate = opt;

            if (fDescriptor->dispatcher != nullptr)
                fDescriptor->dispatcher(fHandle, PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, (float)fSampleRate);
            break;

        case effSetBlockSize:
            if (fBufferSize == static_cast<uint32_t>(value))
                return 0;

            fBufferSize = static_cast<uint32_t>(value);

            if (fDescriptor->dispatcher != nullptr)
                fDescriptor->dispatcher(fHandle, PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, fBufferSize, nullptr, 0.0f);
            break;

        case effMainsChanged:
            if (value != 0)
            {
                if (fDescriptor->activate != nullptr)
                    fDescriptor->activate(fHandle);

                fMidiEventCount = 0;
                carla_zeroStruct<NativeMidiEvent>(fMidiEvents, kMaxMidiEvents);
                carla_zeroStruct<NativeTimeInfo>(fTimeInfo);
            }
            else
            {
                if (fDescriptor->deactivate != nullptr)
                    fDescriptor->deactivate(fHandle);
            }
            break;

        case effEditGetRect:
            *(ERect**)ptr = &fVstRect;
            ret = 1;
            break;

        case effEditOpen:
            if (fDescriptor->ui_show != nullptr)
            {
                char strBuf[0xff+1];
                strBuf[0xff] = '\0';
                std::snprintf(strBuf, 0xff, P_INTPTR, (intptr_t)ptr);

                carla_setenv("CARLA_PLUGIN_EMBED_WINID", strBuf);

                fDescriptor->ui_show(fHandle, true);

                carla_setenv("CARLA_PLUGIN_EMBED_WINID", "0");

                ret = 1;
            }
            break;

        case effEditClose:
            if (fDescriptor->ui_show != nullptr)
            {
                fDescriptor->ui_show(fHandle, false);
                ret = 1;
            }
            break;

        case effEditIdle:
            if (fDescriptor->ui_idle != nullptr)
                fDescriptor->ui_idle(fHandle);
            break;

        case effGetChunk:
            if (ptr == nullptr || fDescriptor->get_state == nullptr)
                return 0;

            if (fStateChunk != nullptr)
                std::free(fStateChunk);

            fStateChunk = fDescriptor->get_state(fHandle);

            if (fStateChunk == nullptr)
                return 0;

            ret = static_cast<intptr_t>(std::strlen(fStateChunk)+1);
            *(void**)ptr = fStateChunk;
            break;

        case effSetChunk:
            if (value <= 0 || fDescriptor->set_state == nullptr)
                return 0;
            if (value == 1)
                return 1;

            if (const char* const state = (const char*)ptr)
            {
                fDescriptor->set_state(fHandle, state);
                ret = 1;
            }
            break;

        case effProcessEvents:
            if (const VstEvents* const events = (const VstEvents*)ptr)
            {
                if (events->numEvents == 0)
                    break;

                for (int i=0, count=events->numEvents; i < count; ++i)
                {
                    const VstMidiEvent* const vstMidiEvent((const VstMidiEvent*)events->events[i]);

                    if (vstMidiEvent == nullptr)
                        break;
                    if (vstMidiEvent->type != kVstMidiType || vstMidiEvent->deltaFrames < 0)
                        continue;
                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;

                    fMidiEvents[fMidiEventCount].port = 0;
                    fMidiEvents[fMidiEventCount].time = static_cast<uint32_t>(vstMidiEvent->deltaFrames);
                    fMidiEvents[fMidiEventCount].size = 3;

                    for (uint32_t j=0; j < 3; ++j)
                        fMidiEvents[fMidiEventCount].data[j] = static_cast<uint8_t>(vstMidiEvent->midiData[j]);

                    fMidiEventCount += 1;
                }
            }
            break;

        case effCanDo:
            if (const char* const canDo = (const char*)ptr)
            {
                if (std::strcmp(canDo, "receiveVstEvents") == 0)
                    return 1;
                if (std::strcmp(canDo, "receiveVstMidiEvent") == 0)
                    return 1;
                if (std::strcmp(canDo, "receiveVstTimeInfo") == 0)
                    return 1;
            }
            break;
        }

        return ret;
    }

    float vst_getParameter(const int32_t /*index*/)
    {
        return 0.0f;
    }

    void vst_setParameter(const int32_t /*index*/, const float /*value*/)
    {
    }

    void vst_processReplacing(const float** const inputs, float** const outputs, const int32_t sampleFrames)
    {
        if (sampleFrames <= 0)
            return;

        static const int kWantVstTimeFlags(kVstTransportPlaying|kVstPpqPosValid|kVstTempoValid|kVstBarsValid|kVstTimeSigValid);

        if (const VstTimeInfo* const vstTimeInfo = (const VstTimeInfo*)fAudioMaster(fEffect, audioMasterGetTime, 0, kWantVstTimeFlags, nullptr, 0.0f))
        {
            fTimeInfo.frame     = static_cast<uint64_t>(vstTimeInfo->samplePos);
            fTimeInfo.playing   =  (vstTimeInfo->flags & kVstTransportPlaying);
            fTimeInfo.bbt.valid = ((vstTimeInfo->flags & kVstTempoValid) != 0 || (vstTimeInfo->flags & kVstTimeSigValid) != 0);

            // ticksPerBeat is not possible with VST
            fTimeInfo.bbt.ticksPerBeat = 960.0;

            if (vstTimeInfo->flags & kVstTempoValid)
                fTimeInfo.bbt.beatsPerMinute = vstTimeInfo->tempo;
            else
                fTimeInfo.bbt.beatsPerMinute = 120.0;

            if (vstTimeInfo->flags & kVstTimeSigValid)
            {
                fTimeInfo.bbt.beatsPerBar = static_cast<float>(vstTimeInfo->timeSigNumerator);
                fTimeInfo.bbt.beatType    = static_cast<float>(vstTimeInfo->timeSigDenominator);
            }
            else
            {
                fTimeInfo.bbt.beatsPerBar = 4.0f;
                fTimeInfo.bbt.beatType    = 4.0f;
            }

            if (vstTimeInfo->flags & kVstPpqPosValid)
            {
                const int    ppqPerBar = vstTimeInfo->timeSigNumerator * 4 / vstTimeInfo->timeSigDenominator;
                const double barBeats  = (std::fmod(vstTimeInfo->ppqPos, ppqPerBar) / ppqPerBar) * vstTimeInfo->timeSigDenominator;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimeInfo.bbt.bar  = static_cast<int32_t>(vstTimeInfo->ppqPos)/ppqPerBar + 1;
                fTimeInfo.bbt.beat = static_cast<int32_t>(barBeats-rest+1.0);
                fTimeInfo.bbt.tick = static_cast<int32_t>(rest*fTimeInfo.bbt.ticksPerBeat+0.5);
            }
            else
            {
                fTimeInfo.bbt.bar  = 1;
                fTimeInfo.bbt.beat = 1;
                fTimeInfo.bbt.tick = 0;
            }

            fTimeInfo.bbt.barStartTick = fTimeInfo.bbt.ticksPerBeat*fTimeInfo.bbt.beatsPerBar*(fTimeInfo.bbt.bar-1);
        }

        if (fHandle != nullptr)
            fDescriptor->process(fHandle, const_cast<float**>(inputs), outputs, static_cast<uint32_t>(sampleFrames), fMidiEvents, fMidiEventCount);

        fMidiEventCount = 0;
    }

protected:
    // -------------------------------------------------------------------

    uint32_t handleGetBufferSize() const
    {
        return fBufferSize;
    }

    double handleGetSampleRate() const
    {
        return fSampleRate;
    }

    bool handleIsOffline() const
    {
        return false;
    }

    const NativeTimeInfo* handleGetTimeInfo() const
    {
        return &fTimeInfo;
    }

    bool handleWriteMidiEvent(const NativeMidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->midiOuts > 0, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->data[0] != 0, false);

        // reverse-find first free event, and put it there
        for (uint32_t i=(kMaxMidiEvents*2)-1; i > fMidiEventCount; --i)
        {
            if (fMidiEvents[i].data[0] == 0)
            {
                std::memcpy(&fMidiEvents[i], event, sizeof(NativeMidiEvent));
                return true;
            }
        }

        return false;
    }

    void handleUiParameterChanged(const uint32_t /*index*/, const float /*value*/) const
    {
    }

    void handleUiCustomDataChanged(const char* const /*key*/, const char* const /*value*/) const
    {
    }

    void handleUiClosed()
    {
    }

    const char* handleUiOpenFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    const char* handleUiSaveFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        carla_debug("NativePlugin::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)", opcode, index, value, ptr, opt);
        return 0;

        // unused for now
        (void)opcode; (void)index; (void)value; (void)ptr; (void)opt;
    }

private:
    // VST stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;

    // Native data
    NativePluginHandle   fHandle;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* const fDescriptor;

    // VST host data
    uint32_t fBufferSize;
    double   fSampleRate;

    // Temporary data
    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kMaxMidiEvents];
    NativeTimeInfo  fTimeInfo;
    ERect           fVstRect;

    char* fStateChunk;

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)handle)

    static uint32_t host_get_buffer_size(NativeHostHandle handle)
    {
        return handlePtr->handleGetBufferSize();
    }

    static double host_get_sample_rate(NativeHostHandle handle)
    {
        return handlePtr->handleGetSampleRate();
    }

    static bool host_is_offline(NativeHostHandle handle)
    {
        return handlePtr->handleIsOffline();
    }

    static const NativeTimeInfo* host_get_time_info(NativeHostHandle handle)
    {
        return handlePtr->handleGetTimeInfo();
    }

    static bool host_write_midi_event(NativeHostHandle handle, const NativeMidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void host_ui_parameter_changed(NativeHostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void host_ui_custom_data_changed(NativeHostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void host_ui_closed(NativeHostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    static const char* host_ui_open_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* host_ui_save_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t host_dispatcher(NativeHostHandle handle, NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePlugin)
};

// -----------------------------------------------------------------------

#ifdef VESTIGE_HEADER
# define handlePtr   ((NativePlugin*)effect->ptr2)
# define validEffect effect != nullptr && effect->ptr2 != nullptr
#else
# define handlePtr   ((NativePlugin*)effect->resvd2)
# define validEffect effect != nullptr && effect->resvd2 != 0
#endif

static intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    // handle base opcodes
    switch (opcode)
    {
    case effOpen:
#ifdef VESTIGE_HEADER
        if (effect != nullptr && effect->ptr3 != nullptr)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->ptr3;
#else
        if (effect != nullptr && effect->object != nullptr)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->object;
#endif
            d_lastBufferSize = static_cast<uint32_t>(audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f));
            d_lastSampleRate = static_cast<double>(audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f));

            // some hosts are not ready at this point or return 0 buffersize/samplerate
            if (d_lastBufferSize == 0)
                d_lastBufferSize = 2048;
            if (d_lastSampleRate <= 0.0)
                d_lastSampleRate = 44100.0;

            const NativePluginDescriptor* pluginDesc  = nullptr;
#ifdef CARLA_PLUGIN_PATCHBAY
            const char* const pluginLabel = "carlapatchbay";
#else
            const char* const pluginLabel = "carlarack";
#endif

            PluginListManager& plm(PluginListManager::getInstance());

            for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin(); it.valid(); it.next())
            {
                const NativePluginDescriptor* const& tmpDesc(it.getValue());

                if (std::strcmp(tmpDesc->label, pluginLabel) == 0)
                {
                    pluginDesc = tmpDesc;
                    break;
                }
            }

            CARLA_SAFE_ASSERT_RETURN(pluginDesc != nullptr, 0);

            NativePlugin* const plugin(new NativePlugin(audioMaster, effect, pluginDesc));
#ifdef VESTIGE_HEADER
            effect->ptr2 = plugin;
#else
            effect->resvd2 = (intptr_t)plugin;
#endif
            return 1;
        }
        return 0;

    case effClose:
        if (validEffect)
        {
#ifdef VESTIGE_HEADER
            delete (NativePlugin*)effect->ptr2;
            effect->ptr2 = nullptr;
            effect->ptr3 = nullptr;
#else
            delete (NativePlugin*)effect->resvd2;
            effect->resvd2 = 0;
            effect->object = nullptr;
#endif
            delete effect;
            return 1;
        }
        return 0;

    case effGetPlugCategory:
#ifdef CARLA_PLUGIN_SYNTH
        return kPlugCategSynth;
#else
        return kPlugCategEffect;
#endif

    case effGetEffectName:
        if (ptr != nullptr)
        {
#ifdef CARLA_PLUGIN_PATCHBAY
            std::strncpy((char*)ptr, "Carla-Patchbay", 64);
#else
            std::strncpy((char*)ptr, "Carla-Rack", 64);
#endif
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (ptr != nullptr)
        {
            std::strncpy((char*)ptr, "falkTX", 64);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (ptr != nullptr)
        {
#ifdef CARLA_PLUGIN_PATCHBAY
            std::strncpy((char*)ptr, "CarlaPatchbay", 32);
#else
            std::strncpy((char*)ptr, "CarlaRack", 32);
#endif
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return CARLA_VERSION_HEX;

    case effGetVstVersion:
        return kVstVersion;
    };

    // handle advanced opcodes
    if (validEffect)
        return handlePtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    if (validEffect)
        return handlePtr->vst_getParameter(index);
    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    if (validEffect)
        handlePtr->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validEffect)
        handlePtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validEffect)
        handlePtr->vst_processReplacing(const_cast<const float**>(inputs), outputs, sampleFrames);
}

#undef handlePtr

// -----------------------------------------------------------------------

CARLA_EXPORT
#ifdef CARLA_OS_WIN
const AEffect* VSTPluginMain(audioMasterCallback audioMaster);
#else
const AEffect* VSTPluginMain(audioMasterCallback audioMaster) asm ("main");
#endif

CARLA_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    // old version
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    AEffect* const effect(new AEffect);
    std::memset(effect, 0, sizeof(AEffect));

    // vst fields
    effect->magic    = kEffectMagic;
    effect->uniqueID = CCONST('C', 'r', 'l', 'a');
#ifdef VESTIGE_HEADER
    int32_t* const version = (int32_t*)&effect->unknown1;
    *version = CARLA_VERSION_HEX;
#else
    effect->version = CARLA_VERSION_HEX;
#endif

    // plugin fields
    effect->numParams   = 0;
    effect->numPrograms = 0;
    effect->numInputs   = 2;
    effect->numOutputs  = 2;

    // plugin flags
    effect->flags |= effFlagsCanReplacing;
    effect->flags |= effFlagsHasEditor;
    effect->flags |= effFlagsProgramChunks;
#ifdef CARLA_PLUGIN_SYNTH
    effect->flags |= effFlagsIsSynth;
#endif

    // static calls
    effect->dispatcher   = vst_dispatcherCallback;
    effect->process      = vst_processCallback;
    effect->getParameter = vst_getParameterCallback;
    effect->setParameter = vst_setParameterCallback;
    effect->processReplacing = vst_processReplacingCallback;

    // pointers
#ifdef VESTIGE_HEADER
    effect->ptr3   = (void*)audioMaster;
#else
    effect->object = (void*)audioMaster;
#endif

    return effect;
}

// -----------------------------------------------------------------------
