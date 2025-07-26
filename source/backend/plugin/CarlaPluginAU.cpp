// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaBackendUtils.hpp"
# include "CarlaPluginUI.hpp"
# include "CarlaMacUtils.hpp"
# include "CarlaMathUtils.hpp"
# include <AudioToolbox/AudioUnit.h>
# include <Foundation/Foundation.h>
#endif

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

#ifdef CARLA_OS_MAC
typedef AudioComponentPlugInInterface* (*FactoryFn)(const AudioComponentDescription*);
typedef OSStatus (*InitializeFn)(void*);
typedef OSStatus (*UninitializeFn)(void*);
typedef OSStatus (*GetPropertyInfoFn)(void*, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement, UInt32*, Boolean*);
typedef OSStatus (*GetPropertyFn)(void*, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement, void*, UInt32*);
typedef OSStatus (*SetPropertyFn)(void*, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement, const void*, UInt32);
typedef OSStatus (*GetParameterFn)(void*, AudioUnitParameterID, AudioUnitScope, AudioUnitElement, AudioUnitParameterValue*);
typedef OSStatus (*SetParameterFn)(void*, AudioUnitParameterID, AudioUnitScope, AudioUnitElement, AudioUnitParameterValue, UInt32);
typedef OSStatus (*ScheduleParametersFn)(void*, const AudioUnitParameterEvent*, UInt32);
typedef OSStatus (*ResetFn)(void*, AudioUnitScope, AudioUnitElement);
typedef OSStatus (*RenderFn)(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32, AudioBufferList*);
typedef OSStatus (*MIDIEventFn)(void*, UInt32, UInt32, UInt32, UInt32);

static constexpr FourCharCode getFourCharCodeFromString(const char str[4])
{
    return (str[0] << 24) + (str[1] << 16) + (str[2] << 8) + str[3];
}

class CarlaPluginAU : public CarlaPlugin,
                      private CarlaPluginUI::Callback
{
public:
    CarlaPluginAU(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInterface(nullptr),
          fAudioBufferData(nullptr)
    {
        carla_stdout("CarlaPluginAU::CarlaPluginAU(%p, %i)", engine, id);
    }

    ~CarlaPluginAU() override
    {
        carla_stdout("CarlaPluginAU::~CarlaPluginAU()");

        // close UI
        showCustomUI(false);

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fInterface != nullptr)
        {
            fInterface->Close(fInterface);
            fInterface = nullptr;
        }

        if (fAudioBufferData != nullptr)
        {
            std::free(fAudioBufferData);
            fAudioBufferData = nullptr;
        }

        // if (fLastChunk != nullptr)
        // {
        //     std::free(fLastChunk);
        //     fLastChunk = nullptr;
        // }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_AU;
    }

    PluginCategory getCategory() const noexcept override
    {
        // TODO
        return PLUGIN_CATEGORY_NONE;
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        // TODO
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // -------------------------------------------------------------------
    // Information (current data)

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        // TODO
        return 0x0;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr, 0.f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.f);

        const AudioUnitParameterID paramId = pData->param.data[parameterId].rindex;
        AudioUnitParameterValue value = 0.f;
        if (fFunctions.getParameter(fInterface, paramId, kAudioUnitScope_Global, 0, &value) == noErr)
            return value;

        return 0.f;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fLabel.buffer(), STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fMaker.buffer(), STR_MAX);
        return true;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fName.buffer(), STR_MAX);
        return true;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const AudioUnitParameterID paramId = pData->param.data[parameterId].rindex;
        AudioUnitParameterInfo info = {};
        UInt32 outDataSize = sizeof(AudioUnitParameterInfo);

        if (fFunctions.getProperty(fInterface, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, paramId, &info, &outDataSize) == noErr)
        {
            if (info.flags & kAudioUnitParameterFlag_HasCFNameString)
            {
                *strBuf = '\0';
                if (! CFStringGetCString(info.cfNameString, strBuf, std::min<int>(STR_MAX, CFStringGetLength(info.cfNameString) + 1), kCFStringEncodingUTF8))
                {
                    carla_stdout("CFStringGetCString fail '%s'", info.name);
                    std::strncpy(strBuf, info.name, STR_MAX);
                }
                else
                {
                    carla_stdout("CFStringGetCString ok '%s' '%s'", info.name, strBuf);
                    std::strncpy(strBuf, info.name, STR_MAX);
                }

                if (info.flags & kAudioUnitParameterFlag_CFNameRelease)
                    CFRelease(info.cfNameString);

                return true;
            }

                    carla_stdout("CFStringGetCString not used '%s'", info.name);
            std::strncpy(strBuf, info.name, STR_MAX);
            return true;
        }

        carla_safe_assert("fFunctions.getProperty(...)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const AudioUnitParameterID paramId = pData->param.data[parameterId].rindex;
        const float fixedValue = pData->param.getFixedValue(parameterId, value);

        fFunctions.setParameter(fInterface, paramId, kAudioUnitScope_Global, 0, value, 0);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const AudioUnitParameterID paramId = pData->param.data[parameterId].rindex;
        const float fixedValue = pData->param.getFixedValue(parameterId, value);

        // TODO use scheduled events
        fFunctions.setParameter(fInterface, paramId, kAudioUnitScope_Global, 0, value, frameOffset);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);
        carla_debug("CarlaPluginAU::reload() - start");

        const EngineProcessMode processMode = pData->engine->getProccessMode();

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t audioIns, audioOuts, parametersIns, parametersOuts;
        audioIns = audioOuts = parametersIns = parametersOuts = 0;

        bool needsCtrlIn, needsCtrlOut, hasMidiIn, hasMidiOut;
        needsCtrlIn = needsCtrlOut = hasMidiIn = hasMidiOut = false;

        String portName;
        const uint portNameSize = pData->engine->getMaxPortNameSize();

        UInt32 outDataSize;
        Boolean outWritable = false;

        // audio ports
        outDataSize = 0;
        if (fFunctions.getPropertyInfo(fInterface,
                                       kAudioUnitProperty_SupportedNumChannels,
                                       kAudioUnitScope_Global,
                                       0, &outDataSize, &outWritable) == noErr
            && outDataSize != 0
            && outDataSize % sizeof(AUChannelInfo) == 0)
        {
            const uint32_t numChannels = outDataSize / sizeof(AUChannelInfo);
            AUChannelInfo* const channelInfo = new AUChannelInfo[numChannels];

            carla_stdout("kAudioUnitProperty_SupportedNumChannels returns %u configs", numChannels);

            if (fFunctions.getProperty(fInterface,
                                       kAudioUnitProperty_SupportedNumChannels,
                                       kAudioUnitScope_Global,
                                       0, channelInfo, &outDataSize) == noErr
                && outDataSize == numChannels * sizeof(AUChannelInfo))
            {
                AUChannelInfo* highestInfo = &channelInfo[0];

                carla_stdout("kAudioUnitProperty_SupportedNumChannels returns {%d,%d}... config",
                             channelInfo[0].inChannels,
                             channelInfo[0].outChannels);

                for (uint32_t i=0; i<numChannels; ++i)
                {
                    if (channelInfo[i].inChannels < 0)
                        channelInfo[i].inChannels = 2;
                    if (channelInfo[i].outChannels < 0)
                        channelInfo[i].outChannels = 2;

                    if (channelInfo[i].inChannels > highestInfo->inChannels
                        && channelInfo[i].outChannels > highestInfo->outChannels)
                    {
                        highestInfo = &channelInfo[i];
                    }
                }

                audioIns = std::min<int16_t>(64, highestInfo->inChannels);
                audioOuts = std::min<int16_t>(64, highestInfo->outChannels);
            }
            else
            {
                carla_stdout("kAudioUnitProperty_SupportedNumChannels failed");
            }

            delete[] channelInfo;
        }
        else
        {
            outDataSize = 0;
            if (fFunctions.getPropertyInfo(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Input,
                                           0, &outDataSize, &outWritable) == noErr
                && outDataSize == sizeof(UInt32))
            {
                UInt32 count = 0;
                if (fFunctions.getProperty(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Input,
                                           0, &count, &outDataSize) == noErr
                    && outDataSize == sizeof(UInt32) && count != 0)
                {
                    AudioStreamBasicDescription desc;
                    std::memset(&desc, 0, sizeof(desc));
                    outDataSize = sizeof(AudioStreamBasicDescription);

                    if (fFunctions.getProperty(fInterface,
                                               kAudioUnitProperty_StreamFormat,
                                               kAudioUnitScope_Input,
                                               0, &desc, &outDataSize) == noErr)
                        audioIns = std::min<uint32_t>(64, desc.mChannelsPerFrame);
                }
            }

            outDataSize = 0;
            if (fFunctions.getPropertyInfo(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Output,
                                           0, &outDataSize, &outWritable) == noErr
                && outDataSize == sizeof(UInt32))
            {
                UInt32 count = 0;
                if (fFunctions.getProperty(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Output,
                                           0, &count, &outDataSize) == noErr
                    && outDataSize == sizeof(UInt32) && count != 0)
                {
                    AudioStreamBasicDescription desc;
                    std::memset(&desc, 0, sizeof(desc));
                    outDataSize = sizeof(AudioStreamBasicDescription);

                    if (fFunctions.getProperty(fInterface,
                                               kAudioUnitProperty_StreamFormat,
                                               kAudioUnitScope_Output,
                                               0, &desc, &outDataSize) == noErr)
                        audioOuts = std::min<uint32_t>(64, desc.mChannelsPerFrame);
                }
            }
        }

        if (audioIns > 0)
        {
            pData->audioIn.createNew(audioIns);
        }

        if (audioOuts > 0)
        {
            pData->audioOut.createNew(audioOuts);
            needsCtrlIn = true;
        }

        std::free(fAudioBufferData);

        if (const uint32_t numBuffers = std::max(audioIns, audioOuts))
        {
            fAudioBufferData = static_cast<AudioBufferList*>(std::malloc(sizeof(uint32_t) + sizeof(AudioBuffer) * numBuffers));
            fAudioBufferData->mNumberBuffers = numBuffers;

            for (uint32_t i = 0; i < numBuffers; ++i)
                fAudioBufferData->mBuffers[i].mNumberChannels = 1;
        }
        else
        {
            fAudioBufferData = static_cast<AudioBufferList*>(std::malloc(sizeof(uint32_t)));
            fAudioBufferData->mNumberBuffers = 0;
        }

        // Audio Ins
        for (uint32_t i=0; i < audioIns; ++i)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (audioIns > 1)
            {
                portName += "input_";
                portName += String(i + 1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[i].port = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, i);
            pData->audioIn.ports[i].rindex = i;
        }

        // Audio Outs
        for (uint32_t i=0; i < audioOuts; ++i)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (audioOuts > 1)
            {
                portName += "output_";
                portName += String(i + 1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[i].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, i);
            pData->audioOut.ports[i].rindex = i;
        }

        // parameters
        outDataSize = 0;
        if (fFunctions.getPropertyInfo(fInterface,
                                       kAudioUnitProperty_ParameterList,
                                       kAudioUnitScope_Global,
                                       0, &outDataSize, &outWritable) == noErr
            && outDataSize != 0
            && outDataSize % sizeof(AudioUnitParameterID) == 0)
        {
            const uint32_t numParams = outDataSize / sizeof(AudioUnitParameterID);
            AudioUnitParameterID* const paramIds = new AudioUnitParameterID[numParams];

            if (fFunctions.getProperty(fInterface, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, paramIds, &outDataSize) == noErr && outDataSize == numParams * sizeof(AudioUnitParameterID))
            {
                pData->param.createNew(numParams, false);

                AudioUnitParameterInfo info;
                float min, max, def, step, stepSmall, stepLarge;

                for (uint32_t i=0; i<numParams; ++i)
                {
                    carla_zeroStruct(info);

                    outDataSize = 0;
                    if (fFunctions.getPropertyInfo(fInterface, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, paramIds[i], &outDataSize, &outWritable) != noErr)
                        break;
                    if (outDataSize != sizeof(AudioUnitParameterInfo))
                        break;
                    if (fFunctions.getProperty(fInterface, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, paramIds[i], &info, &outDataSize) != noErr)
                        break;

                    if (info.flags & kAudioUnitParameterFlag_CFNameRelease)
                        CFRelease(info.cfNameString);

                    pData->param.data[i].index  = static_cast<int32_t>(i);
                    pData->param.data[i].rindex = static_cast<int32_t>(paramIds[i]);

                    if (info.flags & kAudioUnitParameterFlag_IsWritable)
                    {
                        pData->param.data[i].type = PARAMETER_INPUT;
                        needsCtrlIn = true;
                    }
                    else if (info.flags & (kAudioUnitParameterFlag_IsReadable|kAudioUnitParameterFlag_MeterReadOnly))
                    {
                        pData->param.data[i].type = PARAMETER_OUTPUT;
                        needsCtrlOut = true;
                    }
                    else
                    {
                        pData->param.data[i].type = PARAMETER_UNKNOWN;
                        continue;
                    }

                    min = info.minValue;
                    max = info.maxValue;
                    def = info.defaultValue;

                    if (min > max)
                        max = min;

                    if (d_isEqual(min, max))
                    {
                        carla_stderr2("WARNING - Broken plugin parameter '%s': max == min", info.name);
                        max = min + 0.1f;
                    }

                    if (def < min)
                        def = min;
                    else if (def > max)
                        def = max;

                    pData->param.data[i].hints |= PARAMETER_IS_ENABLED;

                    if ((info.flags & kAudioUnitParameterFlag_NonRealTime) == 0)
                    {
                        pData->param.data[i].hints |= PARAMETER_IS_AUTOMATABLE;
                        pData->param.data[i].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
                    }

                    if (info.unit == kAudioUnitParameterUnit_Boolean)
                    {
                        step = max - min;
                        stepSmall = step;
                        stepLarge = step;
                        pData->param.data[i].hints |= PARAMETER_IS_BOOLEAN;
                    }
                    else if (info.unit == kAudioUnitParameterUnit_Indexed)
                    {
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 10.0f;
                        pData->param.data[i].hints |= PARAMETER_IS_INTEGER;
                    }
                    else
                    {
                        float range = max - min;
                        step = range/100.0f;
                        stepSmall = range/1000.0f;
                        stepLarge = range/10.0f;
                    }

                    pData->param.ranges[i].min = min;
                    pData->param.ranges[i].max = max;
                    pData->param.ranges[i].def = def;
                    pData->param.ranges[i].step = step;
                    pData->param.ranges[i].stepSmall = stepSmall;
                    pData->param.ranges[i].stepLarge = stepLarge;
                }
            }

            delete[] paramIds;
        }

        if (needsCtrlIn || hasMidiIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
#endif
        }

        if (needsCtrlOut || hasMidiOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // plugin hints
        pData->hints = 0x0;

        if (audioOuts > 0 && (audioIns == audioOuts || audioIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (audioOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (audioOuts >= 2 && audioOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginAU::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);

        AudioStreamBasicDescription streamFormat = {
            .mFormatID         = kAudioFormatLinearPCM,
            .mBitsPerChannel   = 32,
            .mBytesPerFrame    = sizeof(float),
            .mBytesPerPacket   = sizeof(float),
            .mFramesPerPacket  = 1,
            .mFormatFlags      = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved,
            .mChannelsPerFrame = 0,
            .mSampleRate       = pData->engine->getSampleRate(),
        };

        if (pData->audioIn.count != 0)
        {
            streamFormat.mChannelsPerFrame = pData->audioIn.count;
            CARLA_SAFE_ASSERT_RETURN(fFunctions.setProperty(fInterface,
                                                            kAudioUnitProperty_StreamFormat,
                                                            kAudioUnitScope_Input,
                                                            0, &streamFormat, sizeof(streamFormat)) == noErr,);
        }

        if (pData->audioOut.count != 0)
        {
            streamFormat.mChannelsPerFrame = pData->audioOut.count;
            CARLA_SAFE_ASSERT_RETURN(fFunctions.setProperty(fInterface,
                                                            kAudioUnitProperty_StreamFormat,
                                                            kAudioUnitScope_Output,
                                                            0, &streamFormat, sizeof(streamFormat)) == noErr,);
        }

        fFunctions.initialize(fInterface);
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);

        fFunctions.uninitialize(fInterface);
    }

    void process(const float* const* const audioIn,
                 float** const audioOut,
                 const float* const* const,
                 float** const,
                 const uint32_t frames) override
    {
        // ------------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Check buffers

        CARLA_SAFE_ASSERT_RETURN(frames > 0,);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr,);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr,);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            // TODO
        }

        // ------------------------------------------------------------------------------------------------------------
        // Event Input (main port)

        if (pData->event.portIn != nullptr)
        {
            // TODO
        }

        // ------------------------------------------------------------------------------------------------------------
        // Plugin processing

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        AudioUnitRenderActionFlags actionFlags = kAudioUnitRenderAction_DoNotCheckRenderArgs;
        AudioTimeStamp timeStamp = {};
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        timeStamp.mSampleTime = timeInfo.frame;
        const UInt32 inBusNumber = 0;

        {
            uint32_t i = 0;
            for (; i < pData->audioOut.count; ++i)
            {
                fAudioBufferData->mBuffers[i].mData = audioOut[i];
                fAudioBufferData->mBuffers[i].mDataByteSize = sizeof(float) * frames;

                if (audioOut[i] != audioIn[i])
                    std::memcpy(audioOut[i], audioIn[i], sizeof(float) * frames);
            }

            for (; i < pData->audioIn.count; ++i)
            {
                fAudioBufferData->mBuffers[i].mData = audioOut[i];
                fAudioBufferData->mBuffers[i].mDataByteSize = sizeof(float) * frames;
            }
        }

        fFunctions.render(fInterface, &actionFlags, &timeStamp, inBusNumber, frames, fAudioBufferData);

        // ------------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // ------------------------------------------------------------------------------------------------------------
        // Control Output

        // TODO
       #endif

        // ------------------------------------------------------------------------------------------------------------
        // Events/MIDI Output

        // TODO
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        carla_stdout("CarlaPluginAU::handlePluginUIClosed()");

        // TODO
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        // TODO
    }

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename,
              const char* const label,
              const char* const name,
              const uint options)
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
        // load bundle information

        if (! fBundleLoader.load(filename))
        {
            pData->engine->setLastError("Failed to load AU bundle executable");
            return false;
        }

        const CFTypeRef componentsRef = CFBundleGetValueForInfoDictionaryKey(fBundleLoader.getRef(), CFSTR("AudioComponents"));

        if (componentsRef == nullptr || CFGetTypeID(componentsRef) != CFArrayGetTypeID())
        {
            pData->engine->setLastError("Not an AU component");
            return false;
        }

        // ---------------------------------------------------------------
        // find binary matching requested label

        CFStringRef componentName;
        AudioComponentDescription desc = {};
        FactoryFn factoryFn;

        const CFArrayRef components = static_cast<CFArrayRef>(componentsRef);

        for (uint32_t c = 0, count = CFArrayGetCount(components); c < count; ++c)
        {
            // reset
            desc.componentType = 0;

            const CFTypeRef componentRef = CFArrayGetValueAtIndex(components, c);
            CARLA_SAFE_ASSERT_CONTINUE(componentRef != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(CFGetTypeID(componentRef) == CFDictionaryGetTypeID());

            const CFDictionaryRef component = static_cast<CFDictionaryRef>(componentRef);

            componentName = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("name"), (const void **)&componentName));

            CFStringRef componentFactoryFunction = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("factoryFunction"), (const void **)&componentFactoryFunction));

            CFStringRef componentType = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("type"), (const void **)&componentType));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentType) == 4);

            CFStringRef componentSubType = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("subtype"), (const void **)&componentSubType));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentSubType) == 4);

            CFStringRef componentManufacturer = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("manufacturer"), (const void **)&componentManufacturer));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentManufacturer) == 4);

            factoryFn = fBundleLoader.getSymbol<FactoryFn>(componentFactoryFunction);
            CARLA_SAFE_ASSERT_CONTINUE(factoryFn != nullptr);

            char clabel[15] = {};
            CFStringGetCString(componentType, clabel, 5, kCFStringEncodingASCII);
            CFStringGetCString(componentSubType, clabel + 5, 5, kCFStringEncodingASCII);
            CFStringGetCString(componentManufacturer, clabel + 10, 5, kCFStringEncodingASCII);

            desc.componentType = getFourCharCodeFromString(clabel);
            desc.componentSubType = getFourCharCodeFromString(clabel + 5);
            desc.componentManufacturer = getFourCharCodeFromString(clabel + 10);

            CARLA_SAFE_ASSERT_CONTINUE(desc.componentType != 0);
            CARLA_SAFE_ASSERT_CONTINUE(desc.componentSubType != 0);
            CARLA_SAFE_ASSERT_CONTINUE(desc.componentManufacturer != 0);

            clabel[4] = clabel[9] = ',';

            if (label == nullptr || label[0] == '\0' || std::strcmp(label, clabel) == 0)
                break;
        }

        if (desc.componentType == 0)
        {
            pData->engine->setLastError("Failed to find request plugin in Component bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // load binary

        fInterface = factoryFn(&desc);
        
        if (fInterface == nullptr)
        {
            pData->engine->setLastError("Component failed to create new interface");
            return false;
        }

        if (! fFunctions.init(fInterface))
        {
            pData->engine->setLastError("Component does not provide all necessary functions");
            fInterface = nullptr;
            return false;
        }

        if (fInterface->Open(fInterface, (AudioUnit)(void*)0x1) != noErr)
        {
            pData->engine->setLastError("Component failed to open");
            fInterface->Close(fInterface);
            fInterface = nullptr;
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        const CFIndex componentNameLen = CFStringGetLength(componentName);
        char* const nameBuffer = new char[componentNameLen + 1];

        if (CFStringGetCString(componentName, nameBuffer, componentNameLen + 1, kCFStringEncodingUTF8))
        {
            if (char* const sep = std::strstr(nameBuffer, ": "))
            {
                sep[0] = sep[1] = '\0';
                fName = sep + 2;
                fMaker = nameBuffer;
            }
            else
            {
                fName = nameBuffer;
                fMaker = nameBuffer + componentNameLen;
            }
        }

        fLabel = label;
        pData->name = pData->engine->getUniquePluginName(name != nullptr && name[0] != '\0' ? name : fName.buffer());
        pData->filename = carla_strdup(filename);

        delete[] nameBuffer;

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // init component

        {
            const UInt32 bufferSize = pData->engine->getBufferSize();

            if (fFunctions.setProperty(fInterface,
                                       kAudioUnitProperty_MaximumFramesPerSlice,
                                       kAudioUnitScope_Global,
                                       0, &bufferSize, sizeof(bufferSize)) != noErr)
            {
                pData->engine->setLastError("Failed to set Component maximum frames per slice");
                return false;
            }
        }

        {
            const Float64 sampleRate = pData->engine->getSampleRate();

            // input scope
            UInt32 outDataSize = 0;
            Boolean outWritable = false;
            if (fFunctions.getPropertyInfo(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Input,
                                           0, &outDataSize, &outWritable) == noErr
                && outDataSize == sizeof(UInt32))
            {
                UInt32 outData = 0;
                if (fFunctions.getProperty(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Input,
                                           0, &outData, &outDataSize) == noErr
                    && outData != 0)
                {
                    if (fFunctions.setProperty(fInterface,
                                               kAudioUnitProperty_SampleRate,
                                               kAudioUnitScope_Input,
                                               0, &sampleRate, sizeof(sampleRate)) != noErr)
                    {
                        pData->engine->setLastError("Failed to set Component input sample rate");
                        return false;
                    }
                }
            }

            // output scope
            outDataSize = 0;
            outWritable = false;
            if (fFunctions.getPropertyInfo(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Output,
                                           0, &outDataSize, &outWritable) == noErr
                && outDataSize == sizeof(UInt32))
            {
                UInt32 outData = 0;
                if (fFunctions.getProperty(fInterface,
                                           kAudioUnitProperty_ElementCount,
                                           kAudioUnitScope_Output,
                                           0, &outData, &outDataSize) == noErr
                    && outData != 0)
                {
                    if (fFunctions.setProperty(fInterface,
                                               kAudioUnitProperty_SampleRate,
                                               kAudioUnitScope_Output,
                                               0, &sampleRate, sizeof(sampleRate)) != noErr)
                    {
                        pData->engine->setLastError("Failed to set Component output sample rate");
                        return false;
                    }
                }
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // set default options

        pData->options = PLUGIN_OPTION_FIXED_BUFFERS;

        return true;
    }

private:
    BundleLoader fBundleLoader;
    AudioComponentPlugInInterface* fInterface;
    AudioBufferList* fAudioBufferData;
    String fName;
    String fLabel;
    String fMaker;

    struct Functions {
        InitializeFn initialize;
        UninitializeFn uninitialize;
        GetPropertyInfoFn getPropertyInfo;
        GetPropertyFn getProperty;
        SetPropertyFn setProperty;
        GetParameterFn getParameter;
        SetParameterFn setParameter;
        ScheduleParametersFn scheduleParameters;
        ResetFn reset;
        RenderFn render;
        MIDIEventFn midiEvent;

        Functions()
            : initialize(nullptr),
              uninitialize(nullptr),
              getPropertyInfo(nullptr),
              getProperty(nullptr),
              setProperty(nullptr),
              getParameter(nullptr),
              setParameter(nullptr),
              scheduleParameters(nullptr),
              reset(nullptr),
              render(nullptr),
              midiEvent(nullptr) {}

        bool init(AudioComponentPlugInInterface* const interface)
        {
            initialize = (InitializeFn)interface->Lookup(kAudioUnitInitializeSelect);
            uninitialize = (UninitializeFn)interface->Lookup(kAudioUnitUninitializeSelect);
            getPropertyInfo = (GetPropertyInfoFn)interface->Lookup(kAudioUnitGetPropertyInfoSelect);
            getProperty = (GetPropertyFn)interface->Lookup(kAudioUnitGetPropertySelect);
            setProperty = (SetPropertyFn)interface->Lookup(kAudioUnitSetPropertySelect);
            getParameter = (GetParameterFn)interface->Lookup(kAudioUnitGetParameterSelect);
            setParameter = (SetParameterFn)interface->Lookup(kAudioUnitSetParameterSelect);
            scheduleParameters = (ScheduleParametersFn)interface->Lookup(kAudioUnitScheduleParametersSelect);
            reset = (ResetFn)interface->Lookup(kAudioUnitResetSelect);
            render = (RenderFn)interface->Lookup(kAudioUnitRenderSelect);
            midiEvent = (MIDIEventFn)interface->Lookup(kMusicDeviceMIDIEventSelect);

            return initialize != nullptr
                && uninitialize != nullptr
                && getPropertyInfo != nullptr
                && getProperty != nullptr
                && setProperty != nullptr
                && getParameter != nullptr
                && setParameter != nullptr
                && scheduleParameters != nullptr
                && render != nullptr;
        }
    } fFunctions;
};
#endif

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newAU(const Initializer& init)
{
    carla_stdout("CarlaPlugin::newAU({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.label, init.name, init.uniqueId);

   #ifdef CARLA_OS_MAC
    std::shared_ptr<CarlaPluginAU> plugin(new CarlaPluginAU(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.label, init.name, init.options))
        return nullptr;

    return plugin;
   #else
    init.engine->setLastError("AU support not available");
    return nullptr;
   #endif
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
