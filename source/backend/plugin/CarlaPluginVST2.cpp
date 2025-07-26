// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaProcessUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaVst2Utils.hpp"

#include "CarlaPluginUI.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Foundation/Foundation.h>
#endif

#include <pthread.h>

#include "water/memory/ByteOrder.h"

#undef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 0

#undef kEffectMagic
#define kEffectMagic (CCONST( 'V', 's', 't', 'P' ))

using water::ByteOrder;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

const uint PLUGIN_CAN_PROCESS_REPLACING = 0x1000;
const uint PLUGIN_HAS_COCKOS_EXTENSIONS = 0x2000;
const uint PLUGIN_USES_OLD_VSTSDK       = 0x4000;
const uint PLUGIN_WANTS_MIDI_INPUT      = 0x8000;

static const int32_t kVstMidiEventSize = static_cast<int32_t>(sizeof(VstMidiEvent));

#ifdef PTW32_DLLPORT
static const pthread_t kNullThread = {nullptr, 0};
#else
static const pthread_t kNullThread = 0;
#endif

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginVST2 : public CarlaPlugin,
                        private CarlaPluginUI::Callback
{
public:
    CarlaPluginVST2(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fUnique1(1),
          fEffect(nullptr),
          fMidiEventCount(0),
          fTimeInfo(),
          fNeedIdle(false),
          fLastChunk(nullptr),
          fIsInitializing(true),
          fIsProcessing(false),
          fChangingValuesThread(kNullThread),
          fIdleThread(kNullThread),
          fMainThread(pthread_self()),
          fProcThread(kNullThread),
         #ifdef CARLA_OS_MAC
          fBundleLoader(),
         #endif
          fFirstActive(true),
          fBufferSize(engine->getBufferSize()),
          fAudioOutBuffers(nullptr),
          fLastTimeInfo(),
          fEvents(),
          fUI(),
          fUnique2(2)
    {
        carla_debug("CarlaPluginVST2::CarlaPluginVST2(%p, %i)", engine, id);

        carla_zeroStructs(fMidiEvents, kPluginMaxMidiEvents*2);
        carla_zeroStruct(fTimeInfo);

        for (ushort i=0; i < kPluginMaxMidiEvents*2; ++i)
            fEvents.data[i] = (VstEvent*)&fMidiEvents[i];

        // make plugin valid
        srand(id);
        fUnique1 = fUnique2 = rand();
    }

    ~CarlaPluginVST2() override
    {
        carla_debug("CarlaPluginVST2::~CarlaPluginVST2()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            showCustomUI(false);

            if (fUI.isOpen)
            {
                fUI.isOpen = false;
                dispatcher(effEditClose);
            }
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        CARLA_ASSERT(! fIsProcessing);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fEffect != nullptr)
        {
            dispatcher(effClose);
            fEffect = nullptr;
        }

        // make plugin invalid
        fUnique2 += 1;

        if (fLastChunk != nullptr)
        {
            std::free(fLastChunk);
            fLastChunk = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_VST2;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, CarlaPlugin::getCategory());

        const intptr_t category = dispatcher(effGetPlugCategory);

        switch (category)
        {
        case kPlugCategSynth:
            return PLUGIN_CATEGORY_SYNTH;
        case kPlugCategAnalysis:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategMastering:
            return PLUGIN_CATEGORY_DYNAMICS;
        case kPlugCategRoomFx:
            return PLUGIN_CATEGORY_DELAY;
        case kPlugCategRestoration:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategGenerator:
            return PLUGIN_CATEGORY_SYNTH;
        }

        if (fEffect->flags & effFlagsIsSynth)
            return PLUGIN_CATEGORY_SYNTH;

        return CarlaPlugin::getCategory();
    }

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);

        return static_cast<int64_t>(fEffect->uniqueID);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);

        const int latency = fEffect->initialDelay;
        CARLA_SAFE_ASSERT_RETURN(latency >= 0, 0);

        return static_cast<uint32_t>(latency);
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        *dataPtr = nullptr;

        try {
            const intptr_t ret = dispatcher(effGetChunk, 0 /* bank */, 0, dataPtr);
            CARLA_SAFE_ASSERT_RETURN(ret >= 0, 0);
            return static_cast<std::size_t>(ret);
        } CARLA_SAFE_EXCEPTION_RETURN("CarlaPluginVST2::getChunkData", 0);
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);

        uint options = 0x0;

        // can't disable fixed buffers if using latency or MIDI output
        if (pData->latency.frames == 0 && ! hasMidiOutput())
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (fEffect->numPrograms > 1)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fEffect->flags & effFlagsProgramChunks)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        if (hasMidiInput())
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
            options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fEffect->getParameter(fEffect, static_cast<int32_t>(parameterId));
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);

        strBuf[0] = '\0';
        dispatcher(effGetProductString, 0, 0, strBuf);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);

        strBuf[0] = '\0';
        dispatcher(effGetVendorString, 0, 0, strBuf);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        return getMaker(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);

        strBuf[0] = '\0';
        dispatcher(effGetEffectName, 0, 0, strBuf);
        return true;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        strBuf[0] = '\0';

        VstParameterProperties prop;
        carla_zeroStruct(prop);

        if (dispatcher(effGetParameterProperties, static_cast<int32_t>(parameterId), 0, &prop) == 1
            && prop.label[0] != '\0')
        {
            std::strncpy(strBuf, prop.label, VestigeMaxLabelLen);
            strBuf[VestigeMaxLabelLen] = '\0';
            return true;
        }

        strBuf[0] = '\0';
        dispatcher(effGetParamName, static_cast<int32_t>(parameterId), 0, strBuf);
        return true;
    }

    bool getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        strBuf[0] = '\0';
        dispatcher(effGetParamDisplay, static_cast<int32_t>(parameterId), 0, strBuf);

        if (strBuf[0] == '\0')
            std::snprintf(strBuf, STR_MAX, "%.12g", static_cast<double>(getParameterValue(parameterId)));

        return true;
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        strBuf[0] = '\0';
        dispatcher(effGetParamLabel, static_cast<int32_t>(parameterId), 0, strBuf);
        return true;
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        strBuf[0] = '\0';

        VstParameterProperties prop;
        carla_zeroStruct(prop);

        if (dispatcher(effGetParameterProperties, static_cast<int32_t>(parameterId), 0, &prop) == 1
            && prop.category != 0 && prop.categoryLabel[0] != '\0')
        {
            std::snprintf(strBuf, STR_MAX, "%d:%s", prop.category, prop.categoryLabel);
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fUI.window == nullptr || pData->uiTitle.isNotEmpty())
            return;

        String uiTitle(pData->name);
        uiTitle += " (GUI)";
        fUI.window->setTitle(uiTitle.buffer());
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fEffect->setParameter(fEffect, static_cast<int32_t>(parameterId), fixedValue);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fEffect->setParameter(fEffect, static_cast<int32_t>(parameterId), fixedValue);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        if (loadJuceSaveFormat(data, dataSize))
            return;

        if (fLastChunk != nullptr)
            std::free(fLastChunk);

        fLastChunk = std::malloc(dataSize);
        CARLA_SAFE_ASSERT_RETURN(fLastChunk != nullptr,);

        std::memcpy(fLastChunk, data, dataSize);

        {
            const ScopedSingleProcessLocker spl(this, true);
            const CarlaScopedValueSetter<pthread_t> svs(fChangingValuesThread, pthread_self(), kNullThread);

            dispatcher(effSetChunk, 0 /* bank */, static_cast<intptr_t>(dataSize), fLastChunk);
        }

        // simulate an updateDisplay callback
        handleAudioMasterCallback(audioMasterUpdateDisplay, 0, 0, nullptr, 0.0f);

        pData->updateParameterValues(this, true, true, false);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        if (index >= 0)
        {
            try {
                dispatcher(effBeginSetProgram);
            } CARLA_SAFE_EXCEPTION_RETURN("effBeginSetProgram",);

            {
                const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));
                const CarlaScopedValueSetter<pthread_t> svs(fChangingValuesThread, pthread_self(), kNullThread);

                try {
                    dispatcher(effSetProgram, 0, index);
                } CARLA_SAFE_EXCEPTION("effSetProgram");
            }

            try {
                dispatcher(effEndSetProgram);
            } CARLA_SAFE_EXCEPTION("effEndSetProgram");
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(uindex < pData->prog.count,);

        try {
            dispatcher(effBeginSetProgram);
        } CARLA_SAFE_EXCEPTION_RETURN("effBeginSetProgram",);

        try {
            dispatcher(effSetProgram, 0, static_cast<intptr_t>(uindex));
        } CARLA_SAFE_EXCEPTION("effSetProgram");

        try {
            dispatcher(effEndSetProgram);
        } CARLA_SAFE_EXCEPTION("effEndSetProgram");

        CarlaPlugin::setProgramRT(uindex, sendCallbackLater);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void setCustomUITitle(const char* const title) noexcept override
    {
        if (fUI.window != nullptr)
        {
            try {
                fUI.window->setTitle(title);
            } CARLA_SAFE_EXCEPTION("set custom ui title");
        }

        CarlaPlugin::setCustomUITitle(title);
    }

    void showCustomUI(const bool yesNo) override
    {
        if (fUI.isVisible == yesNo)
            return;

        if (yesNo)
        {
            String uiTitle;

            if (pData->uiTitle.isNotEmpty())
            {
                uiTitle = pData->uiTitle;
            }
            else
            {
                uiTitle  = pData->name;
                uiTitle += " (GUI)";
            }

            intptr_t value = 0;

            if (fUI.window == nullptr)
            {
                const char* msg = nullptr;
                const EngineOptions& opts(pData->engine->getOptions());

#if defined(CARLA_OS_MAC)
                fUI.window = CarlaPluginUI::newCocoa(this, opts.frontendWinId, opts.pluginsAreStandalone, false);
#elif defined(CARLA_OS_WIN)
                fUI.window = CarlaPluginUI::newWindows(this, opts.frontendWinId, opts.pluginsAreStandalone, false);
#elif defined(HAVE_X11)
                fUI.window = CarlaPluginUI::newX11(this, opts.frontendWinId, opts.pluginsAreStandalone, false, false);
#else
                msg = "Unsupported UI type";
#endif

                if (fUI.window == nullptr)
                    return pData->engine->callback(true, true,
                                                   ENGINE_CALLBACK_UI_STATE_CHANGED,
                                                   pData->id,
                                                   -1,
                                                   0, 0, 0.0f,
                                                   msg);

                fUI.window->setTitle(uiTitle.buffer());

#ifdef HAVE_X11
                value = (intptr_t)fUI.window->getDisplay();
#endif

#ifndef CARLA_OS_MAC
                // inform plugin of what UI scale we use
                dispatcher(effVendorSpecific,
                           CCONST('P', 'r', 'e', 'S'),
                           CCONST('A', 'e', 'C', 's'),
                           nullptr,
                           opts.uiScale);
#endif

                // NOTE: there are far too many broken VST2 plugins, don't bother checking return value
                if (dispatcher(effEditOpen, 0, value, fUI.window->getPtr()) != 0 || true)
                {
                    fUI.isOpen = true;

                    ERect* vstRect = nullptr;
                    dispatcher(effEditGetRect, 0, 0, &vstRect);

                    if (vstRect != nullptr)
                    {
                        const int width(vstRect->right - vstRect->left);
                        const int height(vstRect->bottom - vstRect->top);

                        CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                        if (width > 1 && height > 1)
                            fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), true, true);
                    }
                }
                else
                {
                    delete fUI.window;
                    fUI.window = nullptr;

                    carla_stderr2("Plugin refused to open its own UI");
                    return pData->engine->callback(true, true,
                                                   ENGINE_CALLBACK_UI_STATE_CHANGED,
                                                   pData->id,
                                                   -1,
                                                   0, 0, 0.0f,
                                                   "Plugin refused to open its own UI");
                }
            }

            fUI.window->show();
            fUI.isVisible = true;
        }
        else
        {
            fUI.isVisible = false;

            if (fUI.window != nullptr)
                fUI.window->hide();

            if (fUI.isEmbed)
            {
                fUI.isEmbed = false;
                fUI.isOpen = false;
                dispatcher(effEditClose);
            }
        }
    }

    void* embedCustomUI(void* const ptr) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window == nullptr, nullptr);

        fUI.isEmbed = true;
        fUI.isOpen = true;
        fUI.isVisible = true;

       #ifndef CARLA_OS_MAC
        // inform plugin of what UI scale we use
        dispatcher(effVendorSpecific,
                   CCONST('P', 'r', 'e', 'S'),
                   CCONST('A', 'e', 'C', 's'),
                   nullptr,
                   pData->engine->getOptions().uiScale);
       #endif

        dispatcher(effEditOpen, 0, 0, ptr);

        ERect* vstRect = nullptr;
        dispatcher(effEditGetRect, 0, 0, &vstRect);

        if (vstRect != nullptr)
        {
            const int width = vstRect->right - vstRect->left;
            const int height = vstRect->bottom - vstRect->top;

            CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

            if (width > 1 && height > 1)
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                        pData->id, width, height,
                                        0, 0.0f, nullptr);
        }

        return nullptr;
    }

    void idle() override
    {
        if (fNeedIdle)
        {
            const CarlaScopedValueSetter<pthread_t> svs(fIdleThread, pthread_self(), kNullThread);
            dispatcher(effIdle);
        }

        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        if (fUI.isVisible)
            dispatcher(effEditIdle);

        if (fUI.window != nullptr)
            fUI.window->idle();

        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        carla_debug("CarlaPluginVST2::reload() - start");

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);
        const CarlaScopedValueSetter<bool> svs(fIsInitializing, fIsInitializing, false);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t aIns, aOuts, mIns, mOuts, params;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = (fEffect->numInputs > 0)  ? static_cast<uint32_t>(fEffect->numInputs)  : 0;
        aOuts  = (fEffect->numOutputs > 0) ? static_cast<uint32_t>(fEffect->numOutputs) : 0;
        params = (fEffect->numParams > 0)  ? static_cast<uint32_t>(fEffect->numParams)  : 0;

        if (hasMidiInput())
        {
            mIns = 1;
            needsCtrlIn = true;
        }
        else
            mIns = 0;

        if (hasMidiOutput())
        {
            mOuts = 1;
            needsCtrlOut = true;
        }
        else
            mOuts = 0;

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        const EngineProcessMode processMode = pData->engine->getProccessMode();
        const uint portNameSize = pData->engine->getMaxPortNameSize();
        String portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += String(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
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

            if (aOuts > 1)
            {
                portName += "output_";
                portName += String(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            const int32_t ij = static_cast<int32_t>(j);
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].index  = ij;
            pData->param.data[j].rindex = ij;

            float min, max, def, step, stepSmall, stepLarge;

            VstParameterProperties prop;
            carla_zeroStruct(prop);

            if (pData->hints & PLUGIN_HAS_COCKOS_EXTENSIONS)
            {
                double vrange[2] = { 0.0, 1.0 };
                bool   isInteger = false;

                if (static_cast<uintptr_t>(dispatcher(effVendorSpecific, static_cast<int32_t>(0xdeadbef0), ij, vrange)) >= 0xbeef)
                {
                    min = static_cast<float>(vrange[0]);
                    max = static_cast<float>(vrange[1]);

                    if (min > max)
                    {
                        carla_stderr2("WARNING - Broken plugin parameter min > max (with cockos extensions)");
                        min = max - 0.1f;
                    }
                    else if (carla_isEqual(min, max))
                    {
                        carla_stderr2("WARNING - Broken plugin parameter min == max (with cockos extensions)");
                        max = min + 0.1f;
                    }

                    // only use values as integer if we have a proper range
                    if (max - min >= 1.0f)
                        isInteger = dispatcher(effVendorSpecific, kVstParameterUsesIntStep, ij) >= 0xbeef;
                }
                else
                {
                    min = 0.0f;
                    max = 1.0f;
                }

                if (isInteger)
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                }
                else
                {
                    const float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }
            }
            else if (dispatcher(effGetParameterProperties, ij, 0, &prop) == 1)
            {
#if 0
                if (prop.flags & kVstParameterUsesIntegerMinMax)
                {
                    min = static_cast<float>(prop.minInteger);
                    max = static_cast<float>(prop.maxInteger);

                    if (min > max)
                    {
                        carla_stderr2("WARNING - Broken plugin parameter min > max");
                        min = max - 0.1f;
                    }
                    else if (carla_isEqual(min, max))
                    {
                        carla_stderr2("WARNING - Broken plugin parameter min == max");
                        max = min + 0.1f;
                    }
                }
                else
#endif
                {
                    min = 0.0f;
                    max = 1.0f;
                }

                /**/ if (prop.flags & kVstParameterIsSwitch)
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (prop.flags & kVstParameterUsesIntStep)
                {
                    step = static_cast<float>(prop.stepInteger);
                    stepSmall = static_cast<float>(prop.stepInteger)/10.0f;
                    stepLarge = static_cast<float>(prop.largeStepInteger);
                }
                else if (prop.flags & kVstParameterUsesFloatStep)
                {
                    step = prop.stepFloat;
                    stepSmall = prop.smallStepFloat;
                    stepLarge = prop.largeStepFloat;
                }
                else
                {
                    const float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (prop.flags & kVstParameterCanRamp)
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;
            }
            else
            {
                min = 0.0f;
                max = 1.0f;
                step = 0.001f;
                stepSmall = 0.0001f;
                stepLarge = 0.1f;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
            pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            if ((pData->hints & PLUGIN_USES_OLD_VSTSDK) != 0 || dispatcher(effCanBeAutomated, ij) == 1)
            {
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;

                if ((prop.flags & (kVstParameterIsSwitch|kVstParameterUsesIntStep)) == 0x0)
                    pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
            }

            // no such thing as VST default parameters
            def = fEffect->getParameter(fEffect, ij);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
#endif
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // plugin hints
        const intptr_t vstCategory = dispatcher(effGetPlugCategory);

        pData->hints = 0x0;

        if (vstCategory == kPlugCategSynth || vstCategory == kPlugCategGenerator)
           pData->hints |= PLUGIN_IS_SYNTH;

        if (fEffect->flags & effFlagsHasEditor)
        {
#if defined(CARLA_OS_MAC) && ! defined(CARLA_OS_64BIT)
            if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, const_cast<char*>("hasCockosViewAsConfig")) & 0xffff0000) == 0xbeef0000)
#endif
            {
                pData->hints |= PLUGIN_HAS_CUSTOM_UI;
                pData->hints |= PLUGIN_HAS_CUSTOM_EMBED_UI;
            }

            pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
        }

        if (dispatcher(effGetVstVersion) < kVstVersion)
            pData->hints |= PLUGIN_USES_OLD_VSTSDK;

        if ((fEffect->flags & effFlagsCanReplacing) != 0 && fEffect->processReplacing != fEffect->process)
            pData->hints |= PLUGIN_CAN_PROCESS_REPLACING;

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, const_cast<char*>("hasCockosExtensions"))) == 0xbeef0000)
            pData->hints |= PLUGIN_HAS_COCKOS_EXTENSIONS;

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

        // dummy pre-start to get latency and wantEvents() on old plugins
        {
            activate();
            deactivate();
        }

        // check initial latency
        const uint32_t latency = (fEffect->initialDelay > 0) ? static_cast<uint32_t>(fEffect->initialDelay) : 0;

        if (latency != 0)
        {
            pData->client->setLatency(latency);
#ifndef BUILD_BRIDGE
            pData->latency.recreateBuffers(std::max(aIns, aOuts), latency);
#endif
        }

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginVST2::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginVST2::reloadPrograms(%s)", bool2str(doInit));
        const uint32_t oldCount = pData->prog.count;
        const int32_t  current  = pData->prog.current;

        // Delete old programs
        pData->prog.clear();

        // Query new programs
        uint32_t newCount = (fEffect->numPrograms > 0) ? static_cast<uint32_t>(fEffect->numPrograms) : 0;

        if (newCount > 0)
        {
            pData->prog.createNew(newCount);

            // Update names
            for (int32_t i=0; i < fEffect->numPrograms; ++i)
            {
                char strBuf[STR_MAX+1] = { '\0' };
                if (dispatcher(effGetProgramNameIndexed, i, 0, strBuf) != 1)
                {
                    // program will be [re-]changed later
                    dispatcher(effSetProgram, 0, i);
                    dispatcher(effGetProgramName, 0, 0, strBuf);
                }
                pData->prog.names[i] = carla_strdup(strBuf);
            }
        }

        if (doInit)
        {
            if (newCount > 0)
                setProgram(0, false, false, false, true);
            else
                dispatcher(effSetProgram, 0, 0);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (newCount == oldCount+1)
            {
                // one program added, probably created by user
                pData->prog.current = static_cast<int32_t>(oldCount);
                programChanged = true;
            }
            else if (current < 0 && newCount > 0)
            {
                // programs exist now, but not before
                pData->prog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && newCount == 0)
            {
                // programs existed before, but not anymore
                pData->prog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(newCount))
            {
                // current program > count
                pData->prog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                pData->prog.current = current;
            }

            if (programChanged)
            {
                setProgram(pData->prog.current, true, true, true, false);
            }
            else
            {
                // Program was changed during update, re-set it
                if (pData->prog.current >= 0)
                    dispatcher(effSetProgram, 0, pData->prog.current);
            }

            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        const int32_t iBufferSize = static_cast<int32_t>(fBufferSize);
        const float   fSampleRate = static_cast<float>(pData->engine->getSampleRate());

        dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
        dispatcher(effSetBlockSizeAndSampleRate, 0, iBufferSize, nullptr, fSampleRate);
        dispatcher(effSetSampleRate, 0, 0, nullptr, fSampleRate);
        dispatcher(effSetBlockSize, 0, iBufferSize);

        try {
            dispatcher(effMainsChanged, 0, 1);
        } CARLA_SAFE_EXCEPTION("effMainsChanged on");

        try {
            dispatcher(effStartProcess, 0, 0);
        } CARLA_SAFE_EXCEPTION("effStartProcess on");

        fFirstActive = true;
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        try {
            dispatcher(effStopProcess);
        } CARLA_SAFE_EXCEPTION("effStartProcess off");

        try {
            dispatcher(effMainsChanged);
        } CARLA_SAFE_EXCEPTION("effMainsChanged off");
    }

    void process(const float* const* const audioIn,
                 float** const audioOut,
                 const float* const* const cvIn,
                 float** const,
                 const uint32_t frames) override
    {
        const CarlaScopedValueSetter<pthread_t> svs(fProcThread, pthread_self(), kNullThread);

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        fMidiEventCount = 0;
        carla_zeroStructs(fMidiEvents, kPluginMaxMidiEvents*2);

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                fMidiEventCount = MAX_MIDI_CHANNELS*2;

                for (uint8_t i=0; i < MAX_MIDI_CHANNELS; ++i)
                {
                    fMidiEvents[i].type = kVstMidiType;
                    fMidiEvents[i].byteSize    = kVstMidiEventSize;
                    fMidiEvents[i].midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                    fMidiEvents[i].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[MAX_MIDI_CHANNELS + i].type = kVstMidiType;
                    fMidiEvents[MAX_MIDI_CHANNELS + i].byteSize    = kVstMidiEventSize;
                    fMidiEvents[MAX_MIDI_CHANNELS + i].midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                    fMidiEvents[MAX_MIDI_CHANNELS + i].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                fMidiEventCount = MAX_MIDI_NOTE;

                for (uint8_t i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    fMidiEvents[i].type = kVstMidiType;
                    fMidiEvents[i].byteSize    = kVstMidiEventSize;
                    fMidiEvents[i].midiData[0] = char(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT));
                    fMidiEvents[i].midiData[1] = char(i);
                }
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        fTimeInfo.flags = 0;

        if (fFirstActive || ! fLastTimeInfo.compareIgnoringRollingFrames(timeInfo, frames))
        {
            fTimeInfo.flags |= kVstTransportChanged;
            fLastTimeInfo = timeInfo;
        }

        if (timeInfo.playing)
            fTimeInfo.flags |= kVstTransportPlaying;

        fTimeInfo.samplePos  = double(timeInfo.frame);
        fTimeInfo.sampleRate = pData->engine->getSampleRate();

        if (timeInfo.usecs != 0)
        {
            fTimeInfo.nanoSeconds = double(timeInfo.usecs)/1000.0;
            fTimeInfo.flags |= kVstNanosValid;
        }

        if (timeInfo.bbt.valid)
        {
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.bar > 0, timeInfo.bbt.bar);
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.beat > 0, timeInfo.bbt.beat);

            const double ppqBar  = static_cast<double>(timeInfo.bbt.beatsPerBar) * (timeInfo.bbt.bar - 1);
            // const double ppqBeat = static_cast<double>(timeInfo.bbt.beat - 1);
            // const double ppqTick = timeInfo.bbt.tick / timeInfo.bbt.ticksPerBeat;

            // PPQ Pos
            fTimeInfo.ppqPos = fTimeInfo.samplePos / (fTimeInfo.sampleRate * 60 / timeInfo.bbt.beatsPerMinute);
            // fTimeInfo.ppqPos = ppqBar + ppqBeat + ppqTick;
            fTimeInfo.flags |= kVstPpqPosValid;

            // Tempo
            fTimeInfo.tempo  = timeInfo.bbt.beatsPerMinute;
            fTimeInfo.flags |= kVstTempoValid;

            // Bars
            fTimeInfo.barStartPos = ppqBar;
            fTimeInfo.flags |= kVstBarsValid;

            // Time Signature
            fTimeInfo.timeSigNumerator = static_cast<int32_t>(timeInfo.bbt.beatsPerBar + 0.5f);
            fTimeInfo.timeSigDenominator = static_cast<int32_t>(timeInfo.bbt.beatType + 0.5f);
            fTimeInfo.flags |= kVstTimeSigValid;
        }
        else
        {
            // Tempo
            fTimeInfo.tempo = 120.0;
            fTimeInfo.flags |= kVstTempoValid;

            // Time Signature
            fTimeInfo.timeSigNumerator = 4;
            fTimeInfo.timeSigDenominator = 4;
            fTimeInfo.flags |= kVstTimeSigValid;

            // Missing info
            fTimeInfo.ppqPos = 0.0;
            fTimeInfo.barStartPos = 0.0;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { -1, 0, 0 };

                for (; fMidiEventCount < kPluginMaxMidiEvents*2 && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);

                    vstMidiEvent.type        = kVstMidiType;
                    vstMidiEvent.byteSize    = kVstMidiEventSize;
                    vstMidiEvent.midiData[0] = char((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    vstMidiEvent.midiData[1] = char(note.note);
                    vstMidiEvent.midiData[2] = char(note.velo);
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif
            bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn, frames, isSampleAccurate, pData->event.portIn);
#endif

            for (uint32_t i=0, numEvents = pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                EngineEvent& event(pData->event.portIn->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }

                if (isSampleAccurate && eventTime > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, eventTime - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = eventTime;

                        if (fMidiEventCount > 0)
                        {
                            carla_zeroStructs(fMidiEvents, fMidiEventCount);
                            fMidiEventCount = 0;
                        }
                    }
                    else
                        startTime += timeOffset;
                }

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = char(ctrlEvent.param);
                            vstMidiEvent.midiData[2] = char(ctrlEvent.normalizedValue*127.0f + 0.5f);
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if ((pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) != 0)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent_MSB(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent_MSB);
                            vstMidiEvent_MSB.type = kVstMidiType;
                            vstMidiEvent_MSB.byteSize = kVstMidiEventSize;
                            vstMidiEvent_MSB.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);
                            vstMidiEvent_MSB.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent_MSB.midiData[1] = MIDI_CONTROL_BANK_SELECT;
                            vstMidiEvent_MSB.midiData[2] = 0;

                            VstMidiEvent& vstMidiEvent_LSB(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent_LSB);
                            vstMidiEvent_LSB.type        = kVstMidiType;
                            vstMidiEvent_LSB.byteSize    = kVstMidiEventSize;
                            vstMidiEvent_LSB.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                            vstMidiEvent_LSB.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent_LSB.midiData[1] = MIDI_CONTROL_BANK_SELECT__LSB;
                            vstMidiEvent_LSB.midiData[2] = char(ctrlEvent.param);
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            if (ctrlEvent.param < pData->prog.count)
                            {
                                setProgramRT(ctrlEvent.param, true);
                                break;
                            }
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = char(ctrlEvent.param);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > 3)
                        continue;
#ifdef CARLA_PROPER_CPP11_SUPPORT
                    static_assert(3 <= EngineMidiEvent::kDataSize, "Incorrect data");
#endif

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                    carla_zeroStruct(vstMidiEvent);

                    vstMidiEvent.type        = kVstMidiType;
                    vstMidiEvent.byteSize    = kVstMidiEventSize;
                    vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : eventTime);
                    vstMidiEvent.midiData[0] = char(status | (event.channel & MIDI_CHANNEL_BIT));
                    vstMidiEvent.midiData[1] = char(midiEvent.size >= 2 ? midiEvent.data[1] : 0);
                    vstMidiEvent.midiData[2] = char(midiEvent.size >= 3 ? midiEvent.data[2] : 0);

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiEvent.data[1]);
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, frames, 0);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            // reverse lookup MIDI events
            for (uint32_t k = (kPluginMaxMidiEvents*2)-1; k >= fMidiEventCount; --k)
            {
                if (fMidiEvents[k].type == 0)
                    break;

                const VstMidiEvent& vstMidiEvent(fMidiEvents[k]);

                CARLA_SAFE_ASSERT_CONTINUE(vstMidiEvent.deltaFrames >= 0);
                CARLA_SAFE_ASSERT_CONTINUE(vstMidiEvent.midiData[0] != 0);

                uint8_t midiData[3];
                midiData[0] = static_cast<uint8_t>(vstMidiEvent.midiData[0]);
                midiData[1] = static_cast<uint8_t>(vstMidiEvent.midiData[1]);
                midiData[2] = static_cast<uint8_t>(vstMidiEvent.midiData[2]);

                if (! pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(vstMidiEvent.deltaFrames), 3, midiData))
                    break;
            }

        } // End of MIDI Output

        fFirstActive = false;

        // --------------------------------------------------------------------------------------------------------

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return;

        // unused
        (void)cvIn;
#endif
    }

    bool processSingle(const float* const* const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
            CARLA_SAFE_ASSERT_RETURN(fAudioOutBuffers != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
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

        float* vstInBuffer[64]; // pData->audioIn.count

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            vstInBuffer[i] = const_cast<float*>(inBuffer[i]+timeOffset);

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            carla_zeroFloats(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Set MIDI events

        fIsProcessing = true;

        if (fMidiEventCount > 0)
        {
            fEvents.numEvents = static_cast<int32_t>(fMidiEventCount);
            fEvents.reserved  = 0;
            dispatcher(effProcessEvents, 0, 0, &fEvents, 0.0f);
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        if (pData->hints & PLUGIN_CAN_PROCESS_REPLACING)
        {
            fEffect->processReplacing(fEffect,
                                      (pData->audioIn.count > 0) ? vstInBuffer : nullptr,
                                      (pData->audioOut.count > 0) ? fAudioOutBuffers : nullptr,
                                      static_cast<int32_t>(frames));
        }
        else
        {
#if ! VST_FORCE_DEPRECATED
            fEffect->process(fEffect,
                             (pData->audioIn.count > 0) ? vstInBuffer : nullptr,
                             (pData->audioOut.count > 0) ? fAudioOutBuffers : nullptr,
                             static_cast<int32_t>(frames));
#endif
        }

        fIsProcessing = false;
        fTimeInfo.samplePos += frames;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue;
            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = inBuffer[c][k+timeOffset];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioOutBuffers[i][k]  = oldBufLeft[k]            * (1.0f - balRangeL);
                            fAudioOutBuffers[i][k] += fAudioOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioOutBuffers[i][k]  = fAudioOutBuffers[i][k] * balRangeR;
                            fAudioOutBuffers[i][k] += oldBufLeft[k]          * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

        } // End of Post-processing
#else // BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginVST2::bufferSizeChanged(%i)", newBufferSize);

        fBufferSize = pData->engine->getBufferSize();

        if (pData->active)
            deactivate();

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, static_cast<int32_t>(newBufferSize), nullptr, static_cast<float>(pData->engine->getSampleRate()));
#endif
        dispatcher(effSetBlockSize, 0, static_cast<int32_t>(newBufferSize), nullptr, 0.0f);

        if (pData->active)
            activate();

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginVST2::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
            deactivate();

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, static_cast<int32_t>(pData->engine->getBufferSize()), nullptr, static_cast<float>(newSampleRate));
#endif
        dispatcher(effSetSampleRate, 0, 0, nullptr, static_cast<float>(newSampleRate));

        if (pData->active)
            activate();
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginVST2::clearBuffers() - start");

        if (fAudioOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                if (fAudioOutBuffers[i] != nullptr)
                {
                    delete[] fAudioOutBuffers[i];
                    fAudioOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioOutBuffers;
            fAudioOutBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginVST2::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST2::handlePluginUIClosed()");

        showCustomUI(false);
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST2::handlePluginUIResized(%u, %u)", width, height);

        return; // unused
        (void)width; (void)height;
    }

    // -------------------------------------------------------------------

    intptr_t dispatcher(int32_t opcode, int32_t index = 0, intptr_t value = 0, void* ptr = nullptr, float opt = 0.0f) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);
#ifdef DEBUG
        if (opcode != effIdle && opcode != effEditIdle && opcode != effProcessEvents)
            carla_debug("CarlaPluginVST2::dispatcher(%02i:%s, %i, " P_INTPTR ", %p, %f)",
                        opcode, vstEffectOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));
#endif

        try {
            return fEffect->dispatcher(fEffect, opcode, index, value, ptr, opt);
        } CARLA_SAFE_EXCEPTION_RETURN("Vst dispatcher", 0);
    }

    intptr_t handleAudioMasterCallback(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != audioMasterGetTime)
            carla_debug("CarlaPluginVST2::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f)",
                        opcode, vstMasterOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));
#endif

        intptr_t ret = 0;

        switch (opcode)
        {
        case audioMasterAutomate: {
            if (fIsInitializing) {
                // some plugins can be stupid...
                if (pData->param.count == 0)
                    break;
            } else {
                CARLA_CUSTOM_SAFE_ASSERT_BREAK("audioMasterAutomate while disabled", pData->enabled);
            }

            // plugins should never do this:
            CARLA_SAFE_ASSERT_INT2_BREAK(index >= 0 && index < static_cast<int32_t>(pData->param.count),
                                         index,
                                         static_cast<int32_t>(pData->param.count));

            const uint32_t uindex(static_cast<uint32_t>(index));
            const float fixedValue(pData->param.getFixedValue(uindex, opt));

            const pthread_t thisThread = pthread_self();

            if (pthread_equal(thisThread, kNullThread))
            {
                carla_stderr("audioMasterAutomate called with null thread!?");
                setParameterValue(uindex, fixedValue, false, true, true);
            }
            // Called from plugin process thread, nasty! (likely MIDI learn)
            else if (pthread_equal(thisThread, fProcThread))
            {
                CARLA_SAFE_ASSERT(fIsProcessing);
                pData->postponeParameterChangeRtEvent(true, index, fixedValue);
            }
            // Called from effSetChunk or effSetProgram
            else if (pthread_equal(thisThread, fChangingValuesThread))
            {
                carla_debug("audioMasterAutomate called while setting state");
                pData->postponeParameterChangeRtEvent(true, index, fixedValue);
            }
            // Called from effIdle
            else if (pthread_equal(thisThread, fIdleThread))
            {
                carla_debug("audioMasterAutomate called from idle thread");
                pData->postponeParameterChangeRtEvent(true, index, fixedValue);
            }
            // Called from main thread, why?
            else if (pthread_equal(thisThread, fMainThread))
            {
                if (fFirstActive) {
                    carla_stdout("audioMasterAutomate called while loading, nasty!");
                } else {
                    carla_debug("audioMasterAutomate called from main thread");
                }
                CarlaPlugin::setParameterValue(uindex, fixedValue, false, true, true);
            }
            // Called from UI?
            else if (fUI.isVisible)
            {
                carla_debug("audioMasterAutomate called while UI visible");
                CarlaPlugin::setParameterValue(uindex, fixedValue, false, true, true);
            }
            // Unknown
            else
            {
                carla_stdout("audioMasterAutomate called from unknown source");
                CarlaPlugin::setParameterValue(uindex, fixedValue, false, true, true);
            }
            break;
        }

        case audioMasterCurrentId:
            if (fEffect != nullptr)
                ret = fEffect->uniqueID;
            break;

        case audioMasterIdle:
            CARLA_SAFE_ASSERT_BREAK(pthread_equal(pthread_self(), fMainThread));

            pData->engine->callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);

            if (pData->engine->getType() != kEngineTypePlugin)
                pData->engine->idle();
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterPinConnected:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterWantMidi:
            // Deprecated in VST SDK 2.4
            pData->hints |= PLUGIN_WANTS_MIDI_INPUT;
            break;
#endif

        case audioMasterGetTime:
            ret = (intptr_t)&fTimeInfo;
            break;

        case audioMasterProcessEvents:
            CARLA_SAFE_ASSERT_RETURN(pData->enabled, 0);
            CARLA_SAFE_ASSERT_RETURN(fIsProcessing, 0);
            CARLA_SAFE_ASSERT_RETURN(pData->event.portOut != nullptr, 0);

            if (fMidiEventCount >= kPluginMaxMidiEvents*2-1)
                return 0;

            if (const VstEvents* const vstEvents = (const VstEvents*)ptr)
            {
                for (int32_t i=0; i < vstEvents->numEvents && i < kPluginMaxMidiEvents*2; ++i)
                {
                    if (vstEvents->events[i] == nullptr)
                        break;

                    const VstMidiEvent* const vstMidiEvent((const VstMidiEvent*)vstEvents->events[i]);

                    if (vstMidiEvent->type != kVstMidiType)
                        continue;

                    // reverse-find first free event, and put it there
                    for (uint32_t j=(kPluginMaxMidiEvents*2)-1; j >= fMidiEventCount; --j)
                    {
                        if (fMidiEvents[j].type == 0)
                        {
                            std::memcpy(&fMidiEvents[j], vstMidiEvent, sizeof(VstMidiEvent));
                            break;
                        }
                    }
                }
            }
            ret = 1;
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetTime:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            ret = static_cast<intptr_t>(fTimeInfo.tempo * 10000);
            break;

        case audioMasterGetNumAutomatableParameters:
            // Deprecated in VST SDK 2.4
            ret = static_cast<intptr_t>(pData->engine->getOptions().maxParameters);
            ret = carla_minPositive<intptr_t>(ret, fEffect->numParams);
            break;

        case audioMasterGetParameterQuantization:
            // Deprecated in VST SDK 2.4
            ret = 1; // full single float precision
            break;
#endif

#if 0
        case audioMasterIOChanged:
            CARLA_ASSERT(pData->enabled);

            // TESTING

            if (! pData->enabled)
            {
                ret = 1;
                break;
            }

            if (x_engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                carla_stderr2("CarlaPluginVST2::handleAudioMasterIOChanged() - plugin asked IO change, but it's not supported in rack mode");
                return 0;
            }

            engineProcessLock();
            m_enabled = false;
            engineProcessUnlock();

            if (m_active)
            {
                effect->dispatcher(effect, effStopProcess);
                effect->dispatcher(effect, effMainsChanged, 0, 0);
            }

            reload();

            if (m_active)
            {
                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess);
            }

            x_engine->callback(CALLBACK_RELOAD_ALL, m_id, 0, 0, 0, 0.0, nullptr);

            ret = 1;
            break;
#endif

#if ! VST_FORCE_DEPRECATED
        case audioMasterNeedIdle:
            // Deprecated in VST SDK 2.4
            fNeedIdle = true;
            ret = 1;
            break;
#endif

        case audioMasterSizeWindow:
            CARLA_SAFE_ASSERT_BREAK(index > 0);
            CARLA_SAFE_ASSERT_BREAK(value > 0);

            if (fUI.isEmbed)
            {
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                        pData->id, index, static_cast<int>(value),
                                        0, 0.0f, nullptr);
            }
            else
            {
                CARLA_SAFE_ASSERT_BREAK(fUI.window != nullptr);
                fUI.window->setSize(static_cast<uint>(index), static_cast<uint>(value), true, false);
            }
            ret = 1;
            break;

        case audioMasterGetSampleRate:
            ret = static_cast<intptr_t>(pData->engine->getSampleRate());
            break;

        case audioMasterGetBlockSize:
            ret = static_cast<intptr_t>(pData->engine->getBufferSize());
            break;

        case audioMasterGetInputLatency:
            ret = 0;
            break;

        case audioMasterGetOutputLatency:
            ret = 0;
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterGetPreviousPlug:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetNextPlug:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterWillReplaceOrAccumulate:
            // Deprecated in VST SDK 2.4
            ret = 1; // replace
            break;
#endif

        case audioMasterGetCurrentProcessLevel:
            if (pthread_equal(pthread_self(), fProcThread))
            {
                CARLA_SAFE_ASSERT(fIsProcessing);

                if (pData->engine->isOffline())
                    ret = kVstProcessLevelOffline;
                else
                    ret = kVstProcessLevelRealtime;
            }
            else
            {
                ret = kVstProcessLevelUser;
            }
            break;

        case audioMasterGetAutomationState:
            ret = pData->active ? kVstAutomationReadWrite : kVstAutomationOff;
            break;

        case audioMasterOfflineStart:
        case audioMasterOfflineRead:
        case audioMasterOfflineWrite:
        case audioMasterOfflineGetCurrentPass:
        case audioMasterOfflineGetCurrentMetaPass:
            // TODO
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetOutputSampleRate:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetOutputSpeakerArrangement:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterVendorSpecific:
            // TODO - cockos extensions
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetIcon:
            // Deprecated in VST SDK 2.4
            break;
#endif

#if ! VST_FORCE_DEPRECATED
        case audioMasterOpenWindow:
        case audioMasterCloseWindow:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterGetDirectory:
            // TODO
            break;

        case audioMasterUpdateDisplay: {
            bool programNamesUpdated = false;

            // Update current program
            if (pData->prog.count > 1)
            {
                const int32_t current = static_cast<int32_t>(dispatcher(effGetProgram));

                if (current >= 0 && current < static_cast<int32_t>(pData->prog.count))
                {
                    char strBuf[STR_MAX+1]  = { '\0' };
                    dispatcher(effGetProgramName, 0, 0, strBuf);

                    if (pData->prog.names[current] != nullptr)
                        delete[] pData->prog.names[current];

                    pData->prog.names[current] = carla_strdup(strBuf);

                    if (pData->prog.current != current)
                    {
                        pData->prog.current = current;
                        pData->engine->callback(true, true,
                                                ENGINE_CALLBACK_PROGRAM_CHANGED,
                                                pData->id,
                                                current,
                                                0, 0, 0.0f, nullptr);
                    }
                }

                // Check for updated program names
                char strBuf[STR_MAX+1];
                for (int32_t i=0; i < fEffect->numPrograms && i < static_cast<int32_t>(pData->prog.count); ++i)
                {
                    carla_zeroStruct(strBuf);

                    if (dispatcher(effGetProgramNameIndexed, i, 0, strBuf) != 1)
                    {
                        // stop here if plugin does not support indexed names
                        break;
                    }

                    if (std::strcmp(pData->prog.names[i], strBuf) != 0)
                    {
                        const char* const old = pData->prog.names[i];
                        pData->prog.names[i] = carla_strdup(strBuf);
                        delete[] old;
                        programNamesUpdated = true;
                    }
                }
            }

            if (! fIsInitializing)
            {
                if (programNamesUpdated)
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_RELOAD_PROGRAMS,
                                            pData->id, 0, 0, 0, 0.0f, nullptr);

                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_RELOAD_PARAMETERS,
                                        pData->id, 0, 0, 0, 0.0f, nullptr);
            }

            ret = 1;
            break;
        }

        case audioMasterBeginEdit:
            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            pData->engine->touchPluginParameter(pData->id, static_cast<uint32_t>(index), true);
            break;

        case audioMasterEndEdit:
            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            pData->engine->touchPluginParameter(pData->id, static_cast<uint32_t>(index), false);
            break;

        case audioMasterOpenFileSelector:
        case audioMasterCloseFileSelector:
            // TODO
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterEditFile:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetChunkFile:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetInputSpeakerArrangement:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        default:
            carla_debug("CarlaPluginVST2::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f) UNDEF",
                        opcode, vstMasterOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));
            break;
        }

        return ret;

        // unused
        (void)opt;
    }

    bool canDo(const char* const feature) const noexcept
    {
        try {
            return (dispatcher(effCanDo, 0, 0, const_cast<char*>(feature)) == 1);
        } CARLA_SAFE_EXCEPTION_RETURN("vstPluginCanDo", false);
    }

    bool hasMidiInput() const noexcept
    {
        return pData->extraHints & PLUGIN_EXTRA_HINT_HAS_MIDI_IN ||
               pData->hints & PLUGIN_WANTS_MIDI_INPUT ||
               fEffect->flags & effFlagsIsSynth ||
               canDo("receiveVstEvents") ||
               canDo("receiveVstMidiEvent");
    }

    bool hasMidiOutput() const noexcept
    {
        return pData->extraHints & PLUGIN_EXTRA_HINT_HAS_MIDI_OUT ||
               canDo("sendVstEvents") ||
               canDo("sendVstMidiEvent");
    }

    // -------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fEffect;
    }

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const int64_t uniqueId, const uint options)
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

        VST_Function vstFn;

#ifdef CARLA_OS_MAC
        String filenameCheck(filename);
        filenameCheck.toLower();

        if (filenameCheck.endsWith(".vst") || filenameCheck.endsWith(".vst/"))
        {
            if (! fBundleLoader.load(filename))
            {
                pData->engine->setLastError("Failed to load VST2 bundle executable");
                return false;
            }

            vstFn = fBundleLoader.getSymbol<VST_Function>(CFSTR("VSTPluginMain"));

            if (vstFn == nullptr)
                vstFn = fBundleLoader.getSymbol<VST_Function>(CFSTR("main_macho"));

            if (vstFn == nullptr)
            {
                pData->engine->setLastError("Not a VST2 plugin");
                return false;
            }
        }
        else
#endif
        {
            // -----------------------------------------------------------
            // open DLL

            if (! pData->libOpen(filename))
            {
                pData->engine->setLastError(pData->libError(filename));
                return false;
            }

            // -----------------------------------------------------------
            // get DLL main entry

            vstFn = pData->libSymbol<VST_Function>("VSTPluginMain");

            if (vstFn == nullptr)
            {
                vstFn = pData->libSymbol<VST_Function>("main");

                if (vstFn == nullptr)
                {
                    pData->engine->setLastError("Could not find the VST2 main entry in the plugin library");
                    return false;
                }
            }
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 1)

        sCurrentUniqueId     = static_cast<intptr_t>(uniqueId);
        sLastCarlaPluginVST2 = this;

        bool wasTriggered, wasThrown = false;
        {
           #if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
            const ScopedAbortCatcher sac;
           #endif

            try {
                fEffect = vstFn(carla_vst_audioMasterCallback);
            } CARLA_CATCH_UNWIND catch(...) {
                wasThrown = true;
            }

           #if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
            wasTriggered = sac.wasTriggered();
           #else
            wasTriggered = false;
           #endif
        }

        // try again if plugin blows
        if (wasTriggered || wasThrown)
        {
           #if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
            const ScopedAbortCatcher sac;
           #endif

            try {
                fEffect = vstFn(carla_vst_audioMasterCallback);
            } CARLA_SAFE_EXCEPTION_RETURN("VST init 2nd attempt", false);
        }

        sLastCarlaPluginVST2 = nullptr;
        sCurrentUniqueId     = 0;

        if (fEffect == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        if (fEffect->magic != kEffectMagic)
        {
            pData->engine->setLastError("Plugin is not valid (wrong vst effect magic code)");
            return false;
        }

        fEffect->ptr1 = this;

        const int32_t iBufferSize = static_cast<int32_t>(fBufferSize);
        const float   fSampleRate = static_cast<float>(pData->engine->getSampleRate());

        dispatcher(effIdentify);
        dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
        dispatcher(effSetBlockSizeAndSampleRate, 0, iBufferSize, nullptr, fSampleRate);
        dispatcher(effSetSampleRate, 0, 0, nullptr, fSampleRate);
        dispatcher(effSetBlockSize, 0, iBufferSize);
        dispatcher(effOpen);

        const bool isShell = (dispatcher(effGetPlugCategory) == kPlugCategShell);

        if (uniqueId == 0 && isShell)
        {
            char strBuf[STR_MAX+1];
            carla_zeroChars(strBuf, STR_MAX+1);

            sCurrentUniqueId = dispatcher(effShellGetNextPlugin, 0, 0, strBuf);

            dispatcher(effClose);
            fEffect = nullptr;

            sLastCarlaPluginVST2 = this;

            try {
                fEffect = vstFn(carla_vst_audioMasterCallback);
            } CARLA_SAFE_EXCEPTION_RETURN("Vst init", false);

            sLastCarlaPluginVST2 = nullptr;
            sCurrentUniqueId     = 0;

            dispatcher(effIdentify);
            dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
            dispatcher(effSetBlockSizeAndSampleRate, 0, iBufferSize, nullptr, fSampleRate);
            dispatcher(effSetSampleRate, 0, 0, nullptr, fSampleRate);
            dispatcher(effSetBlockSize, 0, iBufferSize);
            dispatcher(effOpen);
        }

        if (fEffect->uniqueID == 0 && !isShell)
        {
            dispatcher(effClose);
            fEffect = nullptr;
            pData->engine->setLastError("Plugin is not valid (no unique ID after being open)");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
        {
            pData->name = pData->engine->getUniquePluginName(name);
        }
        else
        {
            char strBuf[STR_MAX+1];
            carla_zeroChars(strBuf, STR_MAX+1);
            dispatcher(effGetEffectName, 0, 0, strBuf);

            if (strBuf[0] != '\0')
                pData->name = pData->engine->getUniquePluginName(strBuf);
            else if (const char* const shortname = std::strrchr(filename, CARLA_OS_SEP))
                pData->name = pData->engine->getUniquePluginName(shortname+1);
            else
                pData->name = pData->engine->getUniquePluginName("unknown");
        }

        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 2)

        for (int i = fEffect->numInputs;  --i >= 0;) dispatcher(effConnectInput,  i, 1);
        for (int i = fEffect->numOutputs; --i >= 0;) dispatcher(effConnectOutput, i, 1);

        if (dispatcher(effGetVstVersion) < kVstVersion)
            pData->hints |= PLUGIN_USES_OLD_VSTSDK;

        static const char kHasCockosExtensions[] = "hasCockosExtensions";

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, const_cast<char*>(kHasCockosExtensions))) == 0xbeef0000)
            pData->hints |= PLUGIN_HAS_COCKOS_EXTENSIONS;

        // ---------------------------------------------------------------
        // set default options

        pData->options = 0x0;

        if (fEffect->initialDelay > 0 || hasMidiOutput() || isPluginOptionEnabled(options, PLUGIN_OPTION_FIXED_BUFFERS))
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (fEffect->flags & effFlagsProgramChunks)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
                pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (hasMidiInput())
        {
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
                pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
            if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        if (fEffect->numPrograms > 1 && (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) == 0)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return true;
    }

private:
    int fUnique1;

    AEffect* fEffect;

    uint32_t     fMidiEventCount;
    VstMidiEvent fMidiEvents[kPluginMaxMidiEvents*2];
    VstTimeInfo  fTimeInfo;

    bool  fNeedIdle;
    void* fLastChunk;

    bool      fIsInitializing;
    bool      fIsProcessing;
    pthread_t fChangingValuesThread;
    pthread_t fIdleThread;
    pthread_t fMainThread;
    pthread_t fProcThread;

   #ifdef CARLA_OS_MAC
    BundleLoader fBundleLoader;
   #endif

    bool fFirstActive; // first process() call after activate()
    uint32_t fBufferSize;
    float** fAudioOutBuffers;
    EngineTimeInfo fLastTimeInfo;

    struct FixedVstEvents {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[kPluginMaxMidiEvents*2];

        FixedVstEvents() noexcept
            : numEvents(0),
              reserved(0)
        {
            carla_zeroPointers(data, kPluginMaxMidiEvents*2);
        }

        CARLA_DECLARE_NON_COPYABLE(FixedVstEvents);
    } fEvents;

    struct UI {
        bool isEmbed;
        bool isOpen;
        bool isVisible;
        CarlaPluginUI* window;

        UI() noexcept
            : isEmbed(false),
              isOpen(false),
              isVisible(false),
              window(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(isEmbed || ! isVisible);

            if (window != nullptr)
            {
                delete window;
                window = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(UI);
    } fUI;

    int fUnique2;

    static intptr_t         sCurrentUniqueId;
    static CarlaPluginVST2* sLastCarlaPluginVST2;

    // -------------------------------------------------------------------

    static bool compareMagic(int32_t magic, const char* name) noexcept
    {
        return magic == (int32_t)ByteOrder::littleEndianInt (name)
            || magic == (int32_t)ByteOrder::bigEndianInt (name);
    }

    static int32_t fxbSwap(const int32_t x) noexcept
    {
        return (int32_t)ByteOrder::swapIfLittleEndian ((uint32_t) x);
    }

    bool loadJuceSaveFormat(const void* const data, const std::size_t dataSize)
    {
        if (dataSize < 28)
            return false;

        const int32_t* const set = (const int32_t*)data;

        if (set[1] != 0)
            return false;
        if (! compareMagic(set[0], "CcnK"))
            return false;
        if (! compareMagic(set[2], "FBCh") && ! compareMagic(set[2], "FJuc"))
            return false;
        if (fxbSwap(set[3]) > 1)
            return false;

        const int32_t chunkSize = fxbSwap(set[39]);
        CARLA_SAFE_ASSERT_RETURN(chunkSize > 0, false);

        if (static_cast<std::size_t>(chunkSize + 160) > dataSize)
            return false;

        carla_stdout("NOTE: Loading plugin state in VST2/JUCE compatibility mode");
        setChunkData(&set[40], static_cast<std::size_t>(chunkSize));
        return true;
    }

    static intptr_t carla_vst_hostCanDo(const char* const feature)
    {
        carla_debug("carla_vst_hostCanDo(\"%s\")", feature);

        if (std::strcmp(feature, "supplyIdle") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstEvents") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstMidiEvent") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstMidiEventFlagIsRealtime") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstTimeInfo") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstEvents") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstMidiEvent") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstTimeInfo") == 0)
            return -1;
        if (std::strcmp(feature, "reportConnectionChanges") == 0)
            return -1;
        if (std::strcmp(feature, "acceptIOChanges") == 0)
            return 1;
        if (std::strcmp(feature, "sizeWindow") == 0)
            return 1;
        if (std::strcmp(feature, "offline") == 0)
            return -1;
        if (std::strcmp(feature, "openFileSelector") == 0)
            return -1;
        if (std::strcmp(feature, "closeFileSelector") == 0)
            return -1;
        if (std::strcmp(feature, "startStopProcess") == 0)
            return 1;
        if (std::strcmp(feature, "supportShell") == 0)
            return 1;
        if (std::strcmp(feature, "shellCategory") == 0)
            return 1;
        if (std::strcmp(feature, "NIMKPIVendorSpecificCallbacks") == 0)
            return -1;

        // unimplemented
        carla_stderr("carla_vst_hostCanDo(\"%s\") - unknown feature", feature);
        return 0;
    }

    static intptr_t VSTCALLBACK carla_vst_audioMasterCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
#if defined(DEBUG) && ! defined(CARLA_OS_WIN)
        if (opcode != audioMasterGetTime && opcode != audioMasterProcessEvents && opcode != audioMasterGetCurrentProcessLevel && opcode != audioMasterGetOutputLatency)
            carla_debug("carla_vst_audioMasterCallback(%p, %02i:%s, %i, " P_INTPTR ", %p, %f)",
                        effect, opcode, vstMasterOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));
#endif

        switch (opcode)
        {
        case audioMasterVersion:
            return kVstVersion;

        case audioMasterCurrentId:
            if (sCurrentUniqueId != 0)
                return sCurrentUniqueId;
            break;

        case audioMasterGetVendorString:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            std::strcpy((char*)ptr, "falkTX");
            return 1;

        case audioMasterGetProductString:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            std::strcpy((char*)ptr, "Carla");
            return 1;

        case audioMasterGetVendorVersion:
            return CARLA_VERSION_HEX;

        case audioMasterCanDo:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            return carla_vst_hostCanDo((const char*)ptr);

        case audioMasterGetLanguage:
            return kVstLangEnglish;
        }

        // Check if 'resvd1' points to us, otherwise register ourselves if possible
        CarlaPluginVST2* self = nullptr;

        if (effect != nullptr)
        {
            if (effect->ptr1 != nullptr)
            {
                self = (CarlaPluginVST2*)effect->ptr1;
                if (self->fUnique1 != self->fUnique2)
                    self = nullptr;
            }

            if (self != nullptr)
            {
                if (self->fEffect == nullptr)
                    self->fEffect = effect;

                if (self->fEffect != effect)
                {
                    carla_stderr2("carla_vst_audioMasterCallback() - host pointer mismatch: %p != %p", self->fEffect, effect);
                    self = nullptr;
                }
            }
            else if (sLastCarlaPluginVST2 != nullptr)
            {
                effect->ptr1 = sLastCarlaPluginVST2;
                self = sLastCarlaPluginVST2;
            }
        }

        return (self != nullptr) ? self->handleAudioMasterCallback(opcode, index, value, ptr, opt) : 0;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginVST2)
};

intptr_t         CarlaPluginVST2::sCurrentUniqueId     = 0;
CarlaPluginVST2* CarlaPluginVST2::sLastCarlaPluginVST2 = nullptr;

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPluginPtr CarlaPlugin::newVST2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST2({%p, \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.uniqueId);

    std::shared_ptr<CarlaPluginVST2> plugin(new CarlaPluginVST2(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.uniqueId, init.options))
        return nullptr;

    return plugin;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
