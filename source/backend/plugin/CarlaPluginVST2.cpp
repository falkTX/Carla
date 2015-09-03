/*
 * Carla VST Plugin
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
# define USE_JUCE_FOR_VST
#endif

#ifndef USE_JUCE_FOR_VST

#include "CarlaVstUtils.hpp"

#include "CarlaMathUtils.hpp"
#include "CarlaPluginUI.hpp"

#include "juce_core.h"

#include <pthread.h>

#undef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 0

using juce::File;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

const uint PLUGIN_CAN_PROCESS_REPLACING = 0x1000;
const uint PLUGIN_HAS_COCKOS_EXTENSIONS = 0x2000;
const uint PLUGIN_USES_OLD_VSTSDK       = 0x4000;
const uint PLUGIN_WANTS_MIDI_INPUT      = 0x8000;

static const int32_t kVstMidiEventSize = static_cast<int32_t>(sizeof(VstMidiEvent));

// -----------------------------------------------------

class CarlaPluginVST2 : public CarlaPlugin,
                        private CarlaPluginUI::CloseCallback
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
          fIsProcessing(false),
          fMainThread(pthread_self()),
#ifdef PTW32_DLLPORT
          fProcThread({nullptr, 0}),
#else
          fProcThread(0),
#endif
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
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        CARLA_ASSERT(! fIsProcessing);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fEffect != nullptr)
        {
            dispatcher(effClose, 0, 0, nullptr, 0.0f);
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

        const intptr_t category(dispatcher(effGetPlugCategory, 0, 0, nullptr, 0.0f));

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
            const intptr_t ret = dispatcher(effGetChunk, 0 /* bank */, 0, dataPtr, 0.0f);
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

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fEffect->flags & effFlagsProgramChunks)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        if (getMidiInCount() == 0)
        {
            options |= PLUGIN_OPTION_FIXED_BUFFERS;
        }
        else
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
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fEffect->getParameter(fEffect, static_cast<int32_t>(parameterId));
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        strBuf[0] = '\0';
        dispatcher(effGetProductString, 0, 0, strBuf, 0.0f);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        strBuf[0] = '\0';
        dispatcher(effGetVendorString, 0, 0, strBuf, 0.0f);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        strBuf[0] = '\0';
        dispatcher(effGetEffectName, 0, 0, strBuf, 0.0f);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        strBuf[0] = '\0';
        dispatcher(effGetParamName, static_cast<int32_t>(parameterId), 0, strBuf, 0.0f);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        strBuf[0] = '\0';
        dispatcher(effGetParamDisplay, static_cast<int32_t>(parameterId), 0, strBuf, 0.0f);

        if (strBuf[0] == '\0')
            std::snprintf(strBuf, STR_MAX, "%f", getParameterValue(parameterId));
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        strBuf[0] = '\0';
        dispatcher(effGetParamLabel, static_cast<int32_t>(parameterId), 0, strBuf, 0.0f);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fUI.window != nullptr)
        {
            CarlaString guiTitle(pData->name);
            guiTitle += " (GUI)";
            fUI.window->setTitle(guiTitle.buffer());
        }
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

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        if (fLastChunk != nullptr)
            std::free(fLastChunk);

        fLastChunk = std::malloc(dataSize);
        CARLA_SAFE_ASSERT_RETURN(fLastChunk != nullptr,);

        std::memcpy(fLastChunk, data, dataSize);

        {
            const ScopedSingleProcessLocker spl(this, true);
            dispatcher(effSetChunk, 0 /* bank */, static_cast<intptr_t>(dataSize), fLastChunk, 0.0f);
        }

        // simulate an updateDisplay callback
        handleAudioMasterCallback(audioMasterUpdateDisplay, 0, 0, nullptr, 0.0f);

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        const bool sendOsc(pData->engine->isOscControlRegistered());
#else
        const bool sendOsc(false);
#endif
        pData->updateParameterValues(this, sendOsc, true, false);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

        if (index >= 0)
        {
            try {
                dispatcher(effBeginSetProgram, 0, 0, nullptr, 0.0f);
            } CARLA_SAFE_EXCEPTION_RETURN("effBeginSetProgram",);

            {
                const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

                try {
                    dispatcher(effSetProgram, 0, index, nullptr, 0.0f);
                } CARLA_SAFE_EXCEPTION("effSetProgram");
            }

            try {
                dispatcher(effEndSetProgram, 0, 0, nullptr, 0.0f);
            } CARLA_SAFE_EXCEPTION("effEndSetProgram");
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (fUI.isVisible == yesNo)
            return;

        if (yesNo)
        {
            CarlaString uiTitle(pData->name);
            uiTitle += " (GUI)";

            intptr_t value  = 0;
            void*    vstPtr = nullptr;
            ERect*  vstRect = nullptr;

            if (fUI.window == nullptr)
            {
                const char* msg = nullptr;
                const uintptr_t frontendWinId(pData->engine->getOptions().frontendWinId);

#if defined(CARLA_OS_LINUX)
# ifdef HAVE_X11
                fUI.window = CarlaPluginUI::newX11(this, frontendWinId, false);
# else
                msg = "UI is only for systems with X11";
                (void)frontendWinId; // unused
# endif
#elif defined(CARLA_OS_MAC)
# ifdef __LP64__
                fUI.window = CarlaPluginUI::newCocoa(this, frontendWinId);
# endif
#elif defined(CARLA_OS_WIN)
                fUI.window = CarlaPluginUI::newWindows(this, frontendWinId);
#else
                msg = "Unknown UI type";
#endif

                if (fUI.window == nullptr)
                    return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, msg);

                fUI.window->setTitle(uiTitle.buffer());
            }

            vstPtr = fUI.window->getPtr();

            dispatcher(effEditGetRect, 0, 0, &vstRect, 0.0f);

#ifdef HAVE_X11
            value = (intptr_t)fUI.window->getDisplay();
#endif

            if (dispatcher(effEditOpen, 0, value, vstPtr, 0.0f) != 0)
            {
                if (vstRect == nullptr || vstRect->right - vstRect->left < 2)
                    dispatcher(effEditGetRect, 0, 0, &vstRect, 0.0f);

                if (vstRect != nullptr)
                {
                    const int width(vstRect->right - vstRect->left);
                    const int height(vstRect->bottom - vstRect->top);

                    CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                    if (width > 1 && height > 1)
                        fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), false);
                }

                fUI.window->show();
                fUI.isVisible = true;
            }
            else
            {
                delete fUI.window;
                fUI.window = nullptr;

                return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, "Plugin refused to open its own UI");
            }
        }
        else
        {
            fUI.isVisible = false;

            CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
            fUI.window->hide();

            dispatcher(effEditClose, 0, 0, nullptr, 0.0f);
        }
    }

    void idle() override
    {
        if (fNeedIdle)
            dispatcher(effIdle, 0, 0, nullptr, 0.0f);

        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        if (fUI.window != nullptr)
        {
            fUI.window->idle();

            if (fUI.isVisible)
                dispatcher(effEditIdle, 0, 0, nullptr, 0.0f);
        }

        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        carla_debug("CarlaPluginVST2::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

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

            if (aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
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
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(j);

            float min, max, def, step, stepSmall, stepLarge;

            VstParameterProperties prop;
            carla_zeroStruct(prop);

            if (pData->hints & PLUGIN_HAS_COCKOS_EXTENSIONS)
            {
                double vrange[2] = { 0.0, 1.0 };
                bool   isInteger = false;

                if (static_cast<uintptr_t>(dispatcher(effVendorSpecific, static_cast<int32_t>(0xdeadbef0), static_cast<int32_t>(j), vrange, 0.0f)) >= 0xbeef)
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
                        isInteger = dispatcher(effVendorSpecific, kVstParameterUsesIntStep, static_cast<int32_t>(j), nullptr, 0.0f) >= 0xbeef;
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
            else if (dispatcher(effGetParameterProperties, static_cast<int32_t>(j), 0, &prop, 0) == 1)
            {
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
                {
                    min = 0.0f;
                    max = 1.0f;
                }

                if (prop.flags & kVstParameterIsSwitch)
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
                    pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
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
#ifndef BUILD_BRIDGE
            pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
#endif

            if ((pData->hints & PLUGIN_USES_OLD_VSTSDK) != 0 || dispatcher(effCanBeAutomated, static_cast<int32_t>(j), 0, nullptr, 0.0f) == 1)
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            // no such thing as VST default parameters
            def = fEffect->getParameter(fEffect, static_cast<int32_t>(j));

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
        const intptr_t vstCategory = dispatcher(effGetPlugCategory, 0, 0, nullptr, 0.0f);

        pData->hints = 0x0;

        if (vstCategory == kPlugCategSynth || vstCategory == kPlugCategGenerator)
           pData->hints |= PLUGIN_IS_SYNTH;

        if (fEffect->flags & effFlagsHasEditor)
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
            pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
        }

        if (dispatcher(effGetVstVersion, 0, 0, nullptr, 0.0f) < kVstVersion)
            pData->hints |= PLUGIN_USES_OLD_VSTSDK;

        if ((fEffect->flags & effFlagsCanReplacing) != 0 && fEffect->processReplacing != fEffect->process)
            pData->hints |= PLUGIN_CAN_PROCESS_REPLACING;

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, const_cast<char*>("hasCockosExtensions"), 0.0f)) == 0xbeef0000)
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

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        // dummy pre-start to get latency and wantEvents() on old plugins
        {
            activate();
            deactivate();
        }

#if 0 // TODO
        // check latency
        if (pData->hints & PLUGIN_CAN_DRYWET)
        {
#ifdef VESTIGE_HEADER
            char* const empty3Ptr = &fEffect->empty3[0];
            int32_t initialDelay = *(int32_t*)empty3Ptr;
            pData->latency = (initialDelay > 0) ? static_cast<uint32_t>(initialDelay) : 0;
#else
            pData->latency = (fEffect->initialDelay > 0) ? static_cast<uint32_t>(fEffect->initialDelay) : 0;
#endif

            pData->client->setLatency(pData->latency);
#ifndef BUILD_BRIDGE
            pData->recreateLatencyBuffers();
#endif
        }
#endif

        //bufferSizeChanged(pData->engine->getBufferSize());
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
                if (dispatcher(effGetProgramNameIndexed, i, 0, strBuf, 0.0f) != 1)
                {
                    // program will be [re-]changed later
                    dispatcher(effSetProgram, 0, i, nullptr, 0.0f);
                    dispatcher(effGetProgramName, 0, 0, strBuf, 0.0f);
                }
                pData->prog.names[i] = carla_strdup(strBuf);
            }
        }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        // Update OSC Names
        if (pData->engine->isOscControlRegistered() && pData->id < pData->engine->getCurrentPluginCount())
        {
            pData->engine->oscSend_control_set_program_count(pData->id, newCount);

            for (uint32_t i=0; i < newCount; ++i)
                pData->engine->oscSend_control_set_program_name(pData->id, i, pData->prog.names[i]);
        }
#endif

        if (doInit)
        {
            if (newCount > 0)
                setProgram(0, false, false, false);
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
                setProgram(pData->prog.current, true, true, true);
            }
            else
            {
                // Program was changed during update, re-set it
                if (pData->prog.current >= 0)
                    dispatcher(effSetProgram, 0, pData->prog.current, nullptr, 0.0f);
            }

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        try {
            dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
        } catch(...) {}

        try {
            dispatcher(effStartProcess, 0, 0, nullptr, 0.0f);
        } catch(...) {}
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);

        try {
            dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
        } catch(...) {}

        try {
            dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
        } catch(...) {}
    }

    void process(const float** const audioIn, float** const audioOut, const float** const, float** const, const uint32_t frames) override
    {
        fProcThread = pthread_self();

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
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

                for (uint8_t i=0, k=MAX_MIDI_CHANNELS; i < MAX_MIDI_CHANNELS; ++i)
                {
                    fMidiEvents[k].type = kVstMidiType;
                    fMidiEvents[k].byteSize    = kVstMidiEventSize;
                    fMidiEvents[k].midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiEvents[k].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[k+i].type = kVstMidiType;
                    fMidiEvents[k+i].byteSize    = kVstMidiEventSize;
                    fMidiEvents[k+i].midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiEvents[k+i].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
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

#ifndef BUILD_BRIDGE
#if 0 // TODO
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latencyBuffers[i], static_cast<int>(pData->latency));
            }
#endif
#endif

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        fTimeInfo.flags = kVstTransportChanged;

        if (timeInfo.playing)
            fTimeInfo.flags |= kVstTransportPlaying;

        fTimeInfo.samplePos  = double(timeInfo.frame);
        fTimeInfo.sampleRate = pData->engine->getSampleRate();

        if (timeInfo.usecs != 0)
        {
            fTimeInfo.nanoSeconds = double(timeInfo.usecs)/1000.0;
            fTimeInfo.flags |= kVstNanosValid;
        }

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
        {
            double ppqBar  = double(timeInfo.bbt.bar - 1) * timeInfo.bbt.beatsPerBar;
            double ppqBeat = double(timeInfo.bbt.beat - 1);
            double ppqTick = double(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat;

            // PPQ Pos
            fTimeInfo.ppqPos = ppqBar + ppqBeat + ppqTick;
            fTimeInfo.flags |= kVstPpqPosValid;

            // Tempo
            fTimeInfo.tempo  = timeInfo.bbt.beatsPerMinute;
            fTimeInfo.flags |= kVstTempoValid;

            // Bars
            fTimeInfo.barStartPos = ppqBar;
            fTimeInfo.flags |= kVstBarsValid;

            // Time Signature
            fTimeInfo.timeSigNumerator   = static_cast<int32_t>(timeInfo.bbt.beatsPerBar);
            fTimeInfo.timeSigDenominator = static_cast<int32_t>(timeInfo.bbt.beatType);
            fTimeInfo.flags |= kVstTimeSigValid;
        }
        else
        {
            // Tempo
            fTimeInfo.tempo = 120.0;
            fTimeInfo.flags |= kVstTempoValid;

            // Time Signature
            fTimeInfo.timeSigNumerator   = 4;
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
                ExternalMidiNote note = { 0, 0, 0 };

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

#ifndef BUILD_BRIDGE
            bool allNotesOffSent = false;
#endif
            bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

            for (uint32_t i=0, numEvents = pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, event.time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = event.time;

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
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

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

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                break;
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_CONTROL)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = char(ctrlEvent.param);
                            vstMidiEvent.midiData[2] = char(ctrlEvent.value*127.0f);
                        }

                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            if (ctrlEvent.param < pData->prog.count)
                            {
                                setProgram(ctrlEvent.param, false, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventProgramChange, ctrlEvent.param, 0, 0.0f);
                                break;
                            }
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
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);
                            vstMidiEvent.midiData[0] = char(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            vstMidiEvent.midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            VstMidiEvent& vstMidiEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(vstMidiEvent);

                            vstMidiEvent.type        = kVstMidiType;
                            vstMidiEvent.byteSize    = kVstMidiEventSize;
                            vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);
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
                    static_assert(3 <= EngineMidiEvent::kDataSize, "Incorrect data");

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

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
                    vstMidiEvent.deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);
                    vstMidiEvent.midiData[0] = char(status | (event.channel & MIDI_CHANNEL_BIT));
                    vstMidiEvent.midiData[1] = char(midiEvent.size >= 2 ? midiEvent.data[1] : 0);
                    vstMidiEvent.midiData[2] = char(midiEvent.size >= 3 ? midiEvent.data[2] : 0);

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, midiEvent.data[1], 0.0f);
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

                pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(vstMidiEvent.deltaFrames), 3, midiData);
            }

        } // End of MIDI Output
    }

    bool processSingle(const float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
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

        float* vstInBuffer[pData->audioIn.count];
        float* vstOutBuffer[pData->audioOut.count];

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            vstInBuffer[i] = const_cast<float*>(inBuffer[i]+timeOffset);
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            vstOutBuffer[i] = outBuffer[i]+timeOffset;

        // --------------------------------------------------------------------------------------------------------
        // Set MIDI events

        if (fMidiEventCount > 0)
        {
            fEvents.numEvents = static_cast<int32_t>(fMidiEventCount);
            fEvents.reserved  = 0;
            dispatcher(effProcessEvents, 0, 0, &fEvents, 0.0f);
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fIsProcessing = true;

        if (pData->hints & PLUGIN_CAN_PROCESS_REPLACING)
        {
            fEffect->processReplacing(fEffect, (pData->audioIn.count > 0) ? vstInBuffer : nullptr, (pData->audioOut.count > 0) ? vstOutBuffer : nullptr, static_cast<int32_t>(frames));
        }
        else
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(vstOutBuffer[i], static_cast<int>(frames));

#if ! VST_FORCE_DEPRECATED
            fEffect->process(fEffect, (pData->audioIn.count > 0) ? vstInBuffer : nullptr, (pData->audioOut.count > 0) ? vstOutBuffer : nullptr, static_cast<int32_t>(frames));
#endif
        }

        fIsProcessing = false;

        fTimeInfo.samplePos += frames;

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));

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
                        FloatVectorOperations::copy(oldBufLeft, outBuffer[i]+timeOffset, static_cast<int>(frames));
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
        carla_debug("CarlaPluginVST2::bufferSizeChanged(%i)", newBufferSize);

        if (pData->active)
            deactivate();

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, static_cast<int32_t>(newBufferSize), nullptr, static_cast<float>(pData->engine->getSampleRate()));
#endif
        dispatcher(effSetBlockSize, 0, static_cast<int32_t>(newBufferSize), nullptr, 0.0f);

        if (pData->active)
            activate();
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

    // nothing

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
        pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST2::handlePluginUIResized(%u, %u)", width, height);

        return; // unused
        (void)width; (void)height;
    }

    // -------------------------------------------------------------------

    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);
#ifdef DEBUG
        if (opcode != effIdle && opcode != effEditIdle && opcode != effProcessEvents)
            carla_debug("CarlaPluginVST2::dispatcher(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstEffectOpcode2str(opcode), index, value, ptr, opt);
#endif

        try {
            return fEffect->dispatcher(fEffect, opcode, index, value, ptr, opt);
        } CARLA_SAFE_EXCEPTION_RETURN("Vst dispatcher", 0);
    }

    intptr_t handleAudioMasterCallback(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != audioMasterGetTime)
            carla_debug("CarlaPluginVST2::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

        intptr_t ret = 0;

        switch (opcode)
        {
        case audioMasterAutomate: {
            CARLA_SAFE_ASSERT_BREAK(pData->enabled);

            // plugins should never do this:
            CARLA_SAFE_ASSERT_INT(index >= 0 && index < static_cast<int32_t>(pData->param.count), index);

            if (index < 0 || index >= static_cast<int32_t>(pData->param.count))
                break;

            const uint32_t uindex(static_cast<uint32_t>(index));
            const float fixedValue(pData->param.getFixedValue(uindex, opt));

            // Called from plugin process thread, nasty!
            if (pthread_equal(pthread_self(), fProcThread))
            {
                CARLA_SAFE_ASSERT(fIsProcessing);

                pData->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, fixedValue);
            }
            // Called from UI
            else if (fUI.isVisible)
            {
                CarlaPlugin::setParameterValue(uindex, fixedValue, false, true, true);
            }
            // Unknown
            // TODO - check if plugin or UI is initializing
            else
            {
                carla_stdout("audioMasterAutomate called from unknown source");

                setParameterValue(uindex, fixedValue, true, true, true);
                //pData->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, fixedValue);
            }
            break;
        }

        case audioMasterCurrentId:
            ret = fEffect->uniqueID;
            break;

        case audioMasterIdle:
            if (pthread_equal(pthread_self(), fMainThread))
            {
                pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

                if (pData->engine->getType() != kEngineTypePlugin)
                    pData->engine->idle();
            }
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

            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
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
            ret = carla_fixedValue<intptr_t>(0, static_cast<intptr_t>(pData->engine->getOptions().maxParameters), fEffect->numParams);
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
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            }

            reload();

            if (m_active)
            {
                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
            }

            x_engine->callback(CALLBACK_RELOAD_ALL, m_id, 0, 0, 0.0, nullptr);

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
            CARLA_SAFE_ASSERT_BREAK(fUI.window != nullptr);
            CARLA_SAFE_ASSERT_BREAK(index > 0);
            CARLA_SAFE_ASSERT_BREAK(value > 0);
            fUI.window->setSize(static_cast<uint>(index), static_cast<uint>(value), true);
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
                ret = kVstProcessLevelUser;
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

        case audioMasterUpdateDisplay:
            // Idle UI if visible
            if (fUI.isVisible)
                dispatcher(effEditIdle, 0, 0, nullptr, 0.0f);

            // Update current program
            if (pData->prog.count > 0)
            {
                const int32_t current = static_cast<int32_t>(dispatcher(effGetProgram, 0, 0, nullptr, 0.0f));

                if (current >= 0 && current < static_cast<int32_t>(pData->prog.count))
                {
                    char strBuf[STR_MAX+1]  = { '\0' };
                    dispatcher(effGetProgramName, 0, 0, strBuf, 0.0f);

                    if (pData->prog.names[current] != nullptr)
                        delete[] pData->prog.names[current];

                    pData->prog.names[current] = carla_strdup(strBuf);

                    if (pData->prog.current != current)
                    {
                        pData->prog.current = current;
                        pData->engine->callback(ENGINE_CALLBACK_PROGRAM_CHANGED, pData->id, current, 0, 0.0f, nullptr);
                    }
                }
            }

            pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0.0f, nullptr);
            ret = 1;
            break;

        case audioMasterBeginEdit:
        case audioMasterEndEdit:
            // TODO
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
            carla_debug("CarlaPluginVST2::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f) UNDEF", opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
            break;
        }

        return ret;

        // unused
        (void)opt;
    }

    bool canDo(const char* const feature) const noexcept
    {
        try {
            return (fEffect->dispatcher(fEffect, effCanDo, 0, 0, const_cast<char*>(feature), 0.0f) == 1);
        } CARLA_SAFE_EXCEPTION_RETURN("vstPluginCanDo", false);
    }

    bool hasMidiInput() const noexcept
    {
        return (canDo("receiveVstEvents") || canDo("receiveVstMidiEvent") || (fEffect->flags & effFlagsIsSynth) > 0 || (pData->hints & PLUGIN_WANTS_MIDI_INPUT));
    }

    bool hasMidiOutput() const noexcept
    {
        return (canDo("sendVstEvents") || canDo("sendVstMidiEvent"));
    }

    // -------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fEffect;
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const filename, const char* const name, const int64_t uniqueId)
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

        if (! pData->libOpen(filename))
        {
            pData->engine->setLastError(pData->libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        VST_Function vstFn = pData->libSymbol<VST_Function>("VSTPluginMain");

        if (vstFn == nullptr)
        {
            vstFn = pData->libSymbol<VST_Function>("main");

            if (vstFn == nullptr)
            {
                pData->engine->setLastError("Could not find the VST main entry in the plugin library");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 1)

        sCurrentUniqueId     = uniqueId;
        sLastCarlaPluginVST2 = this;

        try {
            fEffect = vstFn(carla_vst_audioMasterCallback);
        } CARLA_SAFE_EXCEPTION_RETURN("Vst init", false);

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

#ifdef VESTIGE_HEADER
        fEffect->ptr1 = this;
#else
        fEffect->resvd1 = (intptr_t)this;
#endif

        dispatcher(effOpen, 0, 0, nullptr, 0.0f);

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
            dispatcher(effGetEffectName, 0, 0, strBuf, 0.0f);

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

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 2)

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, static_cast<int32_t>(pData->engine->getBufferSize()), nullptr, static_cast<float>(pData->engine->getSampleRate()));
#endif
        dispatcher(effSetSampleRate, 0, 0, nullptr, static_cast<float>(pData->engine->getSampleRate()));
        dispatcher(effSetBlockSize, 0, static_cast<int32_t>(pData->engine->getBufferSize()), nullptr, 0.0f);
        dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

        if (dispatcher(effGetVstVersion, 0, 0, nullptr, 0.0f) < kVstVersion)
            pData->hints |= PLUGIN_USES_OLD_VSTSDK;

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, const_cast<char*>("hasCockosExtensions"), 0.0f)) == 0xbeef0000)
            pData->hints |= PLUGIN_HAS_COCKOS_EXTENSIONS;

        // ---------------------------------------------------------------
        // set default options

        pData->options  = 0x0;

        if (fEffect->flags & effFlagsIsSynth)
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fEffect->flags & effFlagsProgramChunks)
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (hasMidiInput())
        {
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return true;

        // unused
        (void)uniqueId;
    }

private:
    int fUnique1;

    AEffect* fEffect;

    uint32_t     fMidiEventCount;
    VstMidiEvent fMidiEvents[kPluginMaxMidiEvents*2];
    VstTimeInfo  fTimeInfo;

    bool  fNeedIdle;
    void* fLastChunk;

    bool      fIsProcessing;
    pthread_t fMainThread;
    pthread_t fProcThread;

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

        CARLA_DECLARE_NON_COPY_STRUCT(FixedVstEvents);
    } fEvents;

    struct UI {
        bool isVisible;
        CarlaPluginUI* window;

        UI() noexcept
            : isVisible(false),
              window(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(! isVisible);

            if (window != nullptr)
            {
                delete window;
                window = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPY_STRUCT(UI);
    } fUI;

    int fUnique2;

    static intptr_t         sCurrentUniqueId;
    static CarlaPluginVST2* sLastCarlaPluginVST2;

    // -------------------------------------------------------------------

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

        // unimplemented
        carla_stderr("carla_vst_hostCanDo(\"%s\") - unknown feature", feature);
        return 0;
    }

    static intptr_t VSTCALLBACK carla_vst_audioMasterCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
#if defined(DEBUG) && ! defined(CARLA_OS_WIN)
        if (opcode != audioMasterGetTime && opcode != audioMasterProcessEvents && opcode != audioMasterGetCurrentProcessLevel && opcode != audioMasterGetOutputLatency)
            carla_debug("carla_vst_audioMasterCallback(%p, %02i:%s, %i, " P_INTPTR ", %p, %f)", effect, opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
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
            return 0x110; // 1.1.0

        case audioMasterCanDo:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            return carla_vst_hostCanDo((const char*)ptr);

        case audioMasterGetLanguage:
            return kVstLangEnglish;
        }

        // Check if 'resvd1' points to us, otherwise register ourselfs if possible
        CarlaPluginVST2* self = nullptr;

        if (effect != nullptr)
        {
#ifdef VESTIGE_HEADER
            if (effect->ptr1 != nullptr)
            {
                self = (CarlaPluginVST2*)effect->ptr1;
                if (self->fUnique1 != self->fUnique2)
                    self = nullptr;
            }
#else
            if (effect->resvd1 != 0)
            {
                self = (CarlaPluginVST2*)effect->resvd1;
                if (self->fUnique1 != self->fUnique2)
                    self = nullptr;
            }
#endif

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
#ifdef VESTIGE_HEADER
                effect->ptr1 = sLastCarlaPluginVST2;
#else
                effect->resvd1 = (intptr_t)sLastCarlaPluginVST2;
#endif
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

#endif // ! USE_JUCE_FOR_VST

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newVST2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST2({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.uniqueId);

#ifdef USE_JUCE_FOR_VST
    return newJuce(init, "VST");
#else
    CarlaPluginVST2* const plugin(new CarlaPluginVST2(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.uniqueId))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
