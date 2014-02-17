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

#if defined(CARLA_OS_LINUX) || defined(VESTIGE_HEADER)
# define USE_JUCE_FOR_VST 0
#else
# define USE_JUCE_FOR_VST 1
#endif

#ifdef WANT_VST

#include "CarlaVstUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPluginUi.hpp"

#include <QtCore/QFile>

#undef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 0

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

// -----------------------------------------------------

const unsigned int PLUGIN_CAN_PROCESS_REPLACING = 0x1000;
const unsigned int PLUGIN_HAS_COCKOS_EXTENSIONS = 0x2000;
const unsigned int PLUGIN_USES_OLD_VSTSDK       = 0x4000;
const unsigned int PLUGIN_WANTS_MIDI_INPUT      = 0x8000;

// -----------------------------------------------------

class VstPlugin : public CarlaPlugin,
                         CarlaPluginUi::CloseCallback
{
public:
    VstPlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fUnique1(1),
          fEffect(nullptr),
          fLastChunk(nullptr),
          fMidiEventCount(0),
          fIsProcessing(false),
          fNeedIdle(false),
          fUnique2(2)
    {
        carla_debug("VstPlugin::VstPlugin(%p, %i)", engine, id);

        carla_zeroStruct<VstMidiEvent>(fMidiEvents, kPluginMaxMidiEvents*2);
        carla_zeroStruct<VstTimeInfo_R>(fTimeInfo);

        for (ushort i=0; i < kPluginMaxMidiEvents*2; ++i)
            fEvents.data[i] = (VstEvent*)&fMidiEvents[i];

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_VST_GUI);

        // make plugin valid
        srand(id);
        fUnique1 = fUnique2 = rand();
    }

    ~VstPlugin() override
    {
        carla_debug("VstPlugin::~VstPlugin()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            showCustomUI(false);

            if (fUi.isOsc)
                pData->osc.thread.stop(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
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
        return PLUGIN_VST;
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

    int32_t getChunkData(void** const dataPtr) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        int32_t ret = 0;

        try {
            ret = static_cast<int32_t>(dispatcher(effGetChunk, 0 /* bank */, 0, dataPtr, 0.0f));
        } catch(...) {}

        return ret;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (getMidiInCount() == 0)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (fEffect->flags & effFlagsProgramChunks)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        if (vstPluginCanDo(fEffect, "receiveVstEvents") || vstPluginCanDo(fEffect, "receiveVstMidiEvent") || (fEffect->flags & effFlagsIsSynth) > 0 || (pData->hints & PLUGIN_WANTS_MIDI_INPUT))
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

#if 0
    void getParameterText(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        strBuf[0] = '\0';
        dispatcher(effGetParamDisplay, parameterId, 0, strBuf, 0.0f);

        if (strBuf[0] == '\0')
            std::snprintf(strBuf, STR_MAX, "%f", getParameterValue(parameterId));
    }
#endif

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

        if (fUi.window != nullptr)
        {
            QString guiTitle(QString("%1 (GUI)").arg(pData->name));
            fUi.window->setTitle(guiTitle.toUtf8().constData());
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

    void setChunkData(const char* const stringData) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(stringData != nullptr,);

        if (fLastChunk != nullptr)
        {
            std::free(fLastChunk);
            fLastChunk = nullptr;
        }

        QByteArray chunk(QByteArray::fromBase64(stringData));

        CARLA_SAFE_ASSERT_RETURN(chunk.size() > 0,);

        fLastChunk = std::malloc(static_cast<size_t>(chunk.size()));
        CARLA_SAFE_ASSERT_RETURN(fLastChunk != nullptr,);

        std::memcpy(fLastChunk, chunk.constData(), static_cast<size_t>(chunk.size()));

        {
            const ScopedSingleProcessLocker spl(this, true);
            dispatcher(effSetChunk, 0 /* bank */, chunk.size(), fLastChunk, 0.0f);
        }

        // simulate an updateDisplay callback
        handleAudioMasterCallback(audioMasterUpdateDisplay, 0, 0, nullptr, 0.0f);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

        if (index >= 0)
        {
            try {
                dispatcher(effBeginSetProgram, 0, 0, nullptr, 0.0f);
            } catch (...) {
                return;
            }

            {
                const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));
                try {
                    dispatcher(effSetProgram, 0, index, nullptr, 0.0f);
                } catch(...) {}
            }

            try {
                dispatcher(effEndSetProgram, 0, 0, nullptr, 0.0f);
            } catch(...) {}
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (fUi.isOsc)
        {
            if (yesNo)
            {
                pData->osc.data.free();
                pData->osc.thread.start();
            }
            else
            {
                pData->transientTryCounter = 0;

                if (pData->osc.data.target != nullptr)
                {
                    osc_send_hide(pData->osc.data);
                    osc_send_quit(pData->osc.data);
                    pData->osc.data.free();
                }

                pData->osc.thread.stop(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
            }
            return;
        }

        if (fUi.isVisible == yesNo)
            return;

        if (yesNo)
        {
            if (fUi.window == nullptr)
            {
                const char* msg = nullptr;
                const uintptr_t frontendWinId(pData->engine->getOptions().frontendWinId);

#if defined(CARLA_OS_LINUX)
# ifdef HAVE_X11
                fUi.window = CarlaPluginUi::newX11(this, frontendWinId);
# else
                msg = "UI is only for systems with X11";
# endif
#elif defined(CARLA_OS_MAC)
# ifdef __LP64__
                fUi.window = CarlaPluginUi::newCocoa(this, frontendWinId);
# else
                fUi.window = CarlaPluginUi::newCarbon(this, frontendWinId);
# endif
#elif defined(CARLA_OS_WIN)
                fUi.window = CarlaPluginUi::newWindows(this, frontendWinId);
#else
                msg = "Unknown UI type";
#endif

                if (fUi.window == nullptr)
                    return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, msg);

                QString guiTitle(QString("%1 (GUI)").arg(pData->name));
                fUi.window->setTitle(guiTitle.toUtf8().constData());
            }

            if (dispatcher(effEditOpen, 0, 0, fUi.window->getPtr(), 0.0f) != 0)
            {
                ERect* vstRect = nullptr;

                dispatcher(effEditGetRect, 0, 0, &vstRect, 0.0f);

                if (vstRect != nullptr)
                {
                    const int width(vstRect->right  - vstRect->left);
                    const int height(vstRect->bottom - vstRect->top);

                    CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                    if (width > 1 && height > 1)
                        fUi.window->setSize(static_cast<uint>(width), static_cast<uint>(height), false);
                }

                fUi.window->show();
                fUi.isVisible = true;
            }
            else
            {
                delete fUi.window;
                fUi.window = nullptr;

                return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, "Plugin refused to open its own UI");
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(fUi.window != nullptr,);

            fUi.isVisible = false;
            fUi.window->hide();
            dispatcher(effEditClose, 0, 0, nullptr, 0.0f);
        }
    }

    void idle() override
    {
        if (fNeedIdle)
            dispatcher(effIdle, 0, 0, nullptr, 0.0f);

        if (fUi.window != nullptr)
        {
            fUi.window->idle();

            if (fUi.isVisible)
                dispatcher(effEditIdle, 0, 0, nullptr, 0.0f);
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        carla_debug("VstPlugin::reload() - start");

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

        if (vstPluginCanDo(fEffect, "receiveVstEvents") || vstPluginCanDo(fEffect, "receiveVstMidiEvent") || (fEffect->flags & effFlagsIsSynth) > 0 || (pData->hints & PLUGIN_WANTS_MIDI_INPUT))
        {
            mIns = 1;
            needsCtrlIn = true;
        }
        else
            mIns = 0;

        if (vstPluginCanDo(fEffect, "sendVstEvents") || vstPluginCanDo(fEffect, "sendVstMidiEvent"))
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

            if (aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = 0x0;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(j);
            pData->param.data[j].midiCC = -1;
            pData->param.data[j].midiChannel = 0;

            float min, max, def, step, stepSmall, stepLarge;

            VstParameterProperties prop;
            carla_zeroStruct<VstParameterProperties>(prop);

            if (pData->hints & PLUGIN_HAS_COCKOS_EXTENSIONS)
            {
                double vrange[2] = { 0.0, 1.0 };

                if (static_cast<uintptr_t>(dispatcher(effVendorSpecific, static_cast<int32_t>(0xdeadbef0), static_cast<int32_t>(j), vrange, 0.0f)) >= 0xbeef)
                {
                    min = static_cast<float>(vrange[0]);
                    max = static_cast<float>(vrange[1]);

                    if (min > max)
                        max = min;

                    if (max - min == 0.0f)
                    {
                        carla_stderr2("WARNING - Broken plugin parameter: max - min == 0.0f (with cockos extensions)");
                        max = min + 0.1f;
                    }
                }
                else
                {
                    min = 0.0f;
                    max = 1.0f;
                }

                if (dispatcher(effVendorSpecific, kVstParameterUsesIntStep, static_cast<int32_t>(j), nullptr, 0.0f) >= 0xbeef)
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                }
                else
                {
                    float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }
            }
            else if (dispatcher(effGetParameterProperties, static_cast<int32_t>(j), 0, &prop, 0) == 1)
            {
                if (prop.flags & kVstParameterUsesIntegerMinMax)
                {
                    min = float(prop.minInteger);
                    max = float(prop.maxInteger);

                    if (min > max)
                        max = min;
                    else if (max < min)
                        min = max;

                    if (max - min == 0.0f)
                    {
                        carla_stderr2("WARNING - Broken plugin parameter: max - min == 0.0f");
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
                    step = float(prop.stepInteger);
                    stepSmall = float(prop.stepInteger)/10;
                    stepLarge = float(prop.largeStepInteger);
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
                    float range = max - min;
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
            //pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
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
        const intptr_t vstCategory = dispatcher(effGetPlugCategory, 0, 0, nullptr, 0.0f);

        pData->hints = 0x0;

        if (vstCategory == kPlugCategSynth || vstCategory == kPlugCategGenerator)
           pData->hints |= PLUGIN_IS_SYNTH;

        if (fEffect->flags & effFlagsHasEditor)
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

            if (! fUi.isOsc)
                pData->hints |= PLUGIN_NEEDS_SINGLE_THREAD;
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
            pData->recreateLatencyBuffers();
        }

        // special plugin fixes
        // 1. IL Harmless - disable threaded processing
        if (fEffect->uniqueID == 1229484653)
        {
            char strBuf[STR_MAX+1] = { '\0' };
            getLabel(strBuf);

            if (std::strcmp(strBuf, "IL Harmless") == 0)
            {
                // TODO - disable threaded processing
            }
        }

        //bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("VstPlugin::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("VstPlugin::reloadPrograms(%s)", bool2str(doInit));
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

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (pData->engine->isOscControlRegistered())
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

        fMidiEventCount = 0;
        carla_zeroStruct<VstMidiEvent>(fMidiEvents, kPluginMaxMidiEvents*2);

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
                    fMidiEvents[k].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                    fMidiEvents[k].midiData[0] = static_cast<char>(MIDI_STATUS_CONTROL_CHANGE + k);
                    fMidiEvents[k].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[k+i].type = kVstMidiType;
                    fMidiEvents[k+i].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                    fMidiEvents[k+i].midiData[0] = static_cast<char>(MIDI_STATUS_CONTROL_CHANGE + k);
                    fMidiEvents[k+i].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                fMidiEventCount = MAX_MIDI_NOTE;

                for (uint8_t i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    fMidiEvents[i].type = kVstMidiType;
                    fMidiEvents[i].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                    fMidiEvents[i].midiData[0] = static_cast<char>(MIDI_STATUS_NOTE_OFF + pData->ctrlChannel);
                    fMidiEvents[i].midiData[1] = static_cast<char>(i);
                }
            }

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
                for (; fMidiEventCount < kPluginMaxMidiEvents*2 && ! pData->extNotes.data.isEmpty();)
                {
                    const ExternalMidiNote& note(pData->extNotes.data.getFirst(true));

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    fMidiEvents[fMidiEventCount].type = kVstMidiType;
                    fMidiEvents[fMidiEventCount].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                    fMidiEvents[fMidiEventCount].midiData[0] = static_cast<char>(note.channel + (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF);
                    fMidiEvents[fMidiEventCount].midiData[1] = static_cast<char>(note.note);
                    fMidiEvents[fMidiEventCount].midiData[2] = static_cast<char>(note.velo);

                    ++fMidiEventCount;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t numEvents  = pData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(inBuffer, outBuffer, event.time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = event.time;

                        if (fMidiEventCount > 0)
                        {
                            carla_zeroStruct<VstMidiEvent>(fMidiEvents, fMidiEventCount);
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

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            carla_zeroStruct<VstMidiEvent>(fMidiEvents[fMidiEventCount]);

                            fMidiEvents[fMidiEventCount].type = kVstMidiType;
                            fMidiEvents[fMidiEventCount].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                            fMidiEvents[fMidiEventCount].midiData[0] = static_cast<char>(MIDI_STATUS_CONTROL_CHANGE + event.channel);
                            fMidiEvents[fMidiEventCount].midiData[1] = static_cast<char>(ctrlEvent.param);
                            fMidiEvents[fMidiEventCount].midiData[2] = char(ctrlEvent.value*127.0f);
                            fMidiEvents[fMidiEventCount].deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);

                            ++fMidiEventCount;
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

                            carla_zeroStruct<VstMidiEvent>(fMidiEvents[fMidiEventCount]);

                            fMidiEvents[fMidiEventCount].type = kVstMidiType;
                            fMidiEvents[fMidiEventCount].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                            fMidiEvents[fMidiEventCount].midiData[0] = static_cast<char>(MIDI_STATUS_CONTROL_CHANGE + event.channel);
                            fMidiEvents[fMidiEventCount].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            fMidiEvents[fMidiEventCount].deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);

                            ++fMidiEventCount;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            carla_zeroStruct<VstMidiEvent>(fMidiEvents[fMidiEventCount]);

                            fMidiEvents[fMidiEventCount].type = kVstMidiType;
                            fMidiEvents[fMidiEventCount].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                            fMidiEvents[fMidiEventCount].midiData[0] = static_cast<char>(MIDI_STATUS_CONTROL_CHANGE + event.channel);
                            fMidiEvents[fMidiEventCount].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            fMidiEvents[fMidiEventCount].deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);

                            ++fMidiEventCount;
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = static_cast<uint8_t>(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per VST spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    carla_zeroStruct<VstMidiEvent>(fMidiEvents[fMidiEventCount]);

                    fMidiEvents[fMidiEventCount].type = kVstMidiType;
                    fMidiEvents[fMidiEventCount].byteSize    = static_cast<int32_t>(sizeof(VstMidiEvent));
                    fMidiEvents[fMidiEventCount].midiData[0] = static_cast<char>(status + channel);
                    fMidiEvents[fMidiEventCount].midiData[1] = static_cast<char>(midiEvent.data[1]);
                    fMidiEvents[fMidiEventCount].midiData[2] = static_cast<char>(midiEvent.data[2]);
                    fMidiEvents[fMidiEventCount].deltaFrames = static_cast<int32_t>(isSampleAccurate ? startTime : event.time);

                    ++fMidiEventCount;

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                } // case kEngineEventTypeMidi
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(inBuffer, outBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(inBuffer, outBuffer, frames, 0);

        } // End of Plugin processing (no events)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            // reverse lookup MIDI events
            for (uint32_t k = (kPluginMaxMidiEvents*2)-1; k >= fMidiEventCount; --k)
            {
                if (fMidiEvents[k].type == 0)
                    break;

                CARLA_SAFE_ASSERT_CONTINUE(fMidiEvents[k].deltaFrames >= 0);
                CARLA_SAFE_ASSERT_CONTINUE(fMidiEvents[k].midiData[0] != 0);

                const uint8_t status(static_cast<uint8_t>(fMidiEvents[k].midiData[0]));
                const uint8_t channel(static_cast<uint8_t>(status < MIDI_STATUS_BIT ? status & MIDI_CHANNEL_BIT : 0));

                uint8_t midiData[3];
                midiData[0] = static_cast<uint8_t>(fMidiEvents[k].midiData[0]);
                midiData[1] = static_cast<uint8_t>(fMidiEvents[k].midiData[1]);
                midiData[2] = static_cast<uint8_t>(fMidiEvents[k].midiData[2]);

                pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(fMidiEvents[k].deltaFrames), channel, 0, 3, midiData);
            }

        } // End of MIDI Output
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

        float* vstInBuffer[pData->audioIn.count];
        float* vstOutBuffer[pData->audioOut.count];

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            vstInBuffer[i] = inBuffer[i]+timeOffset;
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
                FLOAT_CLEAR(vstOutBuffer[i], frames);

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
        carla_debug("VstPlugin::bufferSizeChanged(%i)", newBufferSize);

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
        carla_debug("VstPlugin::sampleRateChanged(%g)", newSampleRate);

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

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (! fUi.isOsc)
            return;
        if (pData->osc.data.target == nullptr)
            return;

        osc_send_control(pData->osc.data, pData->param.data[index].rindex, value);
    }

    void uiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->prog.count,);

        if (! fUi.isOsc)
            return;
        if (pData->osc.data.target == nullptr)
            return;

        osc_send_program(pData->osc.data, index);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        if (! fUi.isOsc)
            return;
        if (pData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = static_cast<uint8_t>(MIDI_STATUS_NOTE_ON + channel);
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(pData->osc.data, midiData);
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        if (! fUi.isOsc)
            return;
        if (pData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = static_cast<uint8_t>(MIDI_STATUS_NOTE_OFF + channel);
        midiData[2] = note;
        midiData[3] = 0;

        osc_send_midi(pData->osc.data, midiData);
    }

    // -------------------------------------------------------------------

protected:
    void handlePluginUiClosed() override
    {
        CARLA_SAFE_ASSERT_RETURN(! fUi.isOsc,);
        CARLA_SAFE_ASSERT_RETURN(fUi.window != nullptr,);
        carla_debug("Lv2Plugin::handleExternalUiClosed()");

        showCustomUI(false);
        pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) const
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);
#ifdef DEBUG
        if (opcode != effIdle && opcode != effEditIdle && opcode != effProcessEvents)
        carla_debug("VstPlugin::dispatcher(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstEffectOpcode2str(opcode), index, value, ptr, opt);
#endif

        return fEffect->dispatcher(fEffect, opcode, index, value, ptr, opt);
    }

    intptr_t handleAudioMasterCallback(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != audioMasterGetTime)
        carla_debug("VstPlugin::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstEffectOpcode2str(opcode), index, value, ptr, opt);
#endif

        intptr_t ret = 0;

        switch (opcode)
        {
#if 0
        case audioMasterAutomate:
            if (! pData->enabled)
                break;

            // plugins should never do this:
            CARLA_SAFE_ASSERT_INT(index < static_cast<int32_t>(pData->param.count), index);

            if (index < 0 || index >= static_cast<int32_t>(pData->param.count))
                break;

            if (fGui.isVisible && ! fIsProcessing)
            {
                // Called from GUI
                setParameterValue(index, opt, false, true, true);
            }
            else if (fIsProcessing)
            {
                // Called from engine
                const float fixedValue(pData->param.getFixedValue(index, opt));

                if (pData->engine->isOffline())
                {
                    CarlaPlugin::setParameterValue(index, fixedValue, true, true, true);
                }
                else
                {
                    CarlaPlugin::setParameterValue(index, fixedValue, false, false, false);
                    pData->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, fixedValue);
                }
            }
            else
            {
                carla_stdout("audioMasterAutomate called from unknown source");
            }
            break;
#endif

        case audioMasterCurrentId:
            // TODO
            // if using old sdk, return effect->uniqueID
            break;

        case audioMasterIdle:
            if (fUi.window != nullptr)
                fUi.window->idle();
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
            if (fEffect->numParams <= 0)
                ret = 0;
            else if (fEffect->numParams > static_cast<int32_t>(pData->engine->getOptions().maxParameters))
                ret = static_cast<intptr_t>(pData->engine->getOptions().maxParameters);
            else
                ret = fEffect->numParams;
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
                carla_stderr2("VstPlugin::handleAudioMasterIOChanged() - plugin asked IO change, but it's not supported in rack mode");
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
            CARLA_SAFE_ASSERT_BREAK(fUi.window != nullptr);
            CARLA_SAFE_ASSERT_BREAK(index > 0);
            CARLA_SAFE_ASSERT_BREAK(value > 0);
            fUi.window->setSize(static_cast<uint>(index), static_cast<uint>(value), true);
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
            if (pData->engine->isOffline())
                ret = kVstProcessLevelOffline;
            else if (fIsProcessing)
                ret = kVstProcessLevelRealtime;
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
            if (fUi.isVisible)
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
            carla_debug("VstPlugin::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f) UNDEF", opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
            break;
        }

        return ret;

        // unused
        (void)opt;
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

        VST_Function vstFn = (VST_Function)pData->libSymbol("VSTPluginMain");

        if (vstFn == nullptr)
        {
            vstFn = (VST_Function)pData->libSymbol("main");

            if (vstFn == nullptr)
            {
                pData->engine->setLastError("Could not find the VST main entry in the plugin library");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 1)

        sLastVstPlugin = this;
        fEffect = vstFn(carla_vst_audioMasterCallback);
        sLastVstPlugin = nullptr;

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
            carla_zeroChar(strBuf, STR_MAX+1);
            dispatcher(effGetEffectName, 0, 0, strBuf, 0.0f);

            if (strBuf[0] != '\0')
                pData->name = pData->engine->getUniquePluginName(strBuf);
            else if (const char* const shortname = std::strrchr(filename, OS_SEP))
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
        // gui stuff

        if ((fEffect->flags & effFlagsHasEditor) != 0 && (fEffect->flags & effFlagsProgramChunks) == 0 && pData->engine->getOptions().preferUiBridges)
        {
            CarlaString bridgeBinary(pData->engine->getOptions().binaryDir);

#if defined(CARLA_OS_LINUX)
            bridgeBinary += "carla-bridge-vst-x11";
#elif defined(CARLA_OS_MAC)
            bridgeBinary += "carla-bridge-vst-mac";
#elif defined(CARLA_OS_WIN)
            bridgeBinary += "carla-bridge-vst-hwnd.exe";
#else
            bridgeBinary = "";
#endif

            if (QFile(bridgeBinary.buffer()).exists())
            {
                pData->osc.thread.setOscData(bridgeBinary, nullptr);
                fUi.isOsc = true;
            }
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (getMidiInCount() > 0)
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (fEffect->flags & effFlagsProgramChunks)
                pData->options |= PLUGIN_OPTION_USE_CHUNKS;

            if (vstPluginCanDo(fEffect, "receiveVstEvents") || vstPluginCanDo(fEffect, "receiveVstMidiEvent") || (fEffect->flags & effFlagsIsSynth) > 0 || (pData->hints & PLUGIN_WANTS_MIDI_INPUT))
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // set identifier string
            CarlaString identifier("VST/");

            if (const char* const shortname = std::strrchr(filename, OS_SEP))
            {
                identifier += shortname+1;
                identifier += ",";
            }

            identifier += CarlaString(static_cast<long>(fEffect->uniqueID));
            pData->identifier = identifier.dup();

            // load settings
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());

            // ignore settings, we need this anyway
            if (getMidiInCount() > 0)
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        }

        return true;

        // unused
        (void)uniqueId;
    }

private:
    int fUnique1;
    AEffect* fEffect;

    void*         fLastChunk;
    uint32_t      fMidiEventCount;
    VstMidiEvent  fMidiEvents[kPluginMaxMidiEvents*2];
    VstTimeInfo_R fTimeInfo;

    struct FixedVstEvents {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[kPluginMaxMidiEvents*2];

        FixedVstEvents()
            : numEvents(0),
              reserved(0)
        {
            carla_fill<VstEvent*>(data, kPluginMaxMidiEvents*2, nullptr);
        }
    } fEvents;

    struct UI {
        bool isOsc;
        bool isVisible;
        CarlaPluginUi* window;

        UI()
            : isOsc(false),
              isVisible(false),
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
    } fUi;

    volatile bool fIsProcessing;

    bool fNeedIdle;
    int  fUnique2;

    static VstPlugin* sLastVstPlugin;

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
            return -1;
        if (std::strcmp(feature, "shellCategory") == 0)
            return -1;

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
        VstPlugin* self = nullptr;

        if (effect != nullptr)
        {
#ifdef VESTIGE_HEADER
            if (effect->ptr1 != nullptr)
            {
                self = (VstPlugin*)effect->ptr1;
                if (self->fUnique1 != self->fUnique2)
                    self = nullptr;
            }
#else
            if (effect->resvd1 != 0)
            {
                self = (VstPlugin*)effect->resvd1;
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
            else if (sLastVstPlugin != nullptr)
            {
#ifdef VESTIGE_HEADER
                effect->ptr1 = sLastVstPlugin;
#else
                effect->resvd1 = (intptr_t)sLastVstPlugin;
#endif
                self = sLastVstPlugin;
            }
        }

        return (self != nullptr) ? self->handleAudioMasterCallback(opcode, index, value, ptr, opt) : 0;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstPlugin)
};

VstPlugin* VstPlugin::sLastVstPlugin = nullptr;

CARLA_BACKEND_END_NAMESPACE

#endif // WANT_VST

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newVST(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.uniqueId);

#ifdef WANT_VST
# if defined(HAVE_JUCE) && USE_JUCE_FOR_VST
    return newJuce(init, "VST");
# else
    VstPlugin* const plugin(new VstPlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.uniqueId))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo VST plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
# endif
#else
    init.engine->setLastError("VST support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
