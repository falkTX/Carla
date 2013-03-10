/*
 * Carla VST Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaPluginInternal.hpp"

#ifdef WANT_VST

#include "CarlaVstUtils.hpp"

#ifdef Q_WS_X11
# include <QtGui/QX11Info>
#endif

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup PluginHints Plugin Hints
 * @{
 */
const unsigned int PLUGIN_CAN_PROCESS_REPLACING = 0x1000; //!< VST Plugin cas use processReplacing()
const unsigned int PLUGIN_HAS_COCKOS_EXTENSIONS = 0x2000; //!< VST Plugin has Cockos extensions
const unsigned int PLUGIN_USES_OLD_VSTSDK       = 0x4000; //!< VST Plugin uses an old VST SDK
const unsigned int PLUGIN_WANTS_MIDI_INPUT      = 0x8000; //!< VST Plugin wants MIDI input
/**@}*/

class VstPlugin : public CarlaPlugin
{
public:
    VstPlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id),
          fUnique1(1),
          fEffect(nullptr),
          fIsProcessing(false),
          fNeedIdle(false),
          fUnique2(2)
    {
        carla_debug("VstPlugin::VstPlugin(%p, %i)", engine, id);

        carla_zeroMem(fMidiEvents, sizeof(VstMidiEvent)*MAX_MIDI_EVENTS*2);
        carla_zeroMem(&fTimeInfo, sizeof(VstTimeInfo_R));

        for (unsigned short i=0; i < MAX_MIDI_EVENTS*2; i++)
            fEvents.data[i] = (VstEvent*)&fMidiEvents[i];

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_VST_GUI);

        // make plugin valid
        srand(id);
        fUnique1 = fUnique2 = rand();
    }

    ~VstPlugin()
    {
        carla_debug("VstPlugin::~VstPlugin()");

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        // make plugin invalid
        fUnique2 += 1;

        if (fEffect == nullptr)
            return;

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            if (fGui.isOsc)
            {
                // Wait a bit first, then force kill
                if (kData->osc.thread.isRunning() && ! kData->osc.thread.stop(kData->engine->getOptions().oscUiTimeout))
                {
                    carla_stderr("VST GUI thread still running, forcing termination now");
                    kData->osc.thread.terminate();
                }
            }
            else
                dispatcher(effEditClose, 0, 0, nullptr, 0.0f);
        }

        dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
        dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
        dispatcher(effClose, 0, 0, nullptr, 0.0f);

        fEffect = nullptr;
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const
    {
        return PLUGIN_VST;
    }

    PluginCategory category()
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (fEffect != nullptr)
        {
            const intptr_t category = dispatcher(effGetPlugCategory, 0, 0, nullptr, 0.0f);

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
        }

        return getPluginCategoryFromName(fName);
    }

    long uniqueId() const
    {
        CARLA_ASSERT(fEffect != nullptr);

        return (fEffect != nullptr) ? fEffect->uniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        CARLA_ASSERT(fOptions & PLUGIN_OPTION_USE_CHUNKS);
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(dataPtr != nullptr);

        return (fEffect != nullptr) ? dispatcher(effGetChunk, 0 /* bank */, 0, dataPtr, 0.0f) : 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions()
    {
        CARLA_ASSERT(fEffect != nullptr);

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_FIXED_BUFFER;
        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        //if ((kData->audioIns.count() == 2 || kData->audioOuts.count() == 0) || (kData->audioIns.count() == 0 || kData->audioOuts.count() == 2))
        //    options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fEffect->flags & effFlagsProgramChunks)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        if (kData->extraHints & PLUGIN_HINT_HAS_MIDI_IN)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        return (fEffect != nullptr) ? fEffect->getParameter(fEffect, parameterId) : 0;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (fEffect != nullptr)
            dispatcher(effGetProductString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (fEffect != nullptr)
            dispatcher(effGetVendorString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (fEffect != nullptr)
            dispatcher(effGetVendorString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (fEffect != nullptr)
            dispatcher(effGetEffectName, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fEffect != nullptr)
            dispatcher(effGetParamName, parameterId, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fEffect != nullptr)
        {
            dispatcher(effGetParamDisplay, parameterId, 0, strBuf, 0.0f);

            if (*strBuf == 0)
                std::sprintf(strBuf, "%f", getParameterValue(parameterId));
        }
        else
            CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fEffect != nullptr)
            dispatcher(effGetParamLabel, parameterId, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue = kData->param.fixValue(parameterId, value);
        fEffect->setParameter(fEffect, parameterId, fixedValue);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setChunkData(const char* const stringData)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(fOptions & PLUGIN_OPTION_USE_CHUNKS);
        CARLA_ASSERT(stringData != nullptr);

        // FIXME
        fChunk = QByteArray::fromBase64(QByteArray(stringData));
        //fChunk.toBase64();

        const ScopedProcessLocker spl(this, true);
        dispatcher(effSetChunk, 0 /* bank */, fChunk.size(), fChunk.data(), 0.0f);
    }

    void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(fEffect != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->prog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->prog.count))
            return;

        if (fEffect != nullptr && index >= 0)
        {
            const ScopedProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            dispatcher(effBeginSetProgram, 0, 0, nullptr, 0.0f);
            dispatcher(effSetProgram, 0, index, nullptr, 0.0f);
            dispatcher(effEndSetProgram, 0, 0, nullptr, 0.0f);
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

#if 0
    // -------------------------------------------------------------------
    // Set gui stuff

    void setGuiContainer(GuiContainer* const container)
    {
        carla_debug("VstPlugin::setGuiContainer(%p)", container);
        CARLA_ASSERT(container);

        if (gui.type == GUI_EXTERNAL_OSC)
            return;

        int32_t value   = 0;
        void* const ptr = (void*)container->winId();
        ERect* vstRect  = nullptr;

#ifdef Q_WS_X11
        value = (intptr_t)QX11Info::display();
#endif

        // get UI size before opening UI, plugin may refuse this
        effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f);

        if (vstRect)
        {
            int width  = vstRect->right  - vstRect->left;
            int height = vstRect->bottom - vstRect->top;

            if (width > 0 || height > 0)
            {
                container->setFixedSize(width, height);
#ifdef BUILD_BRIDGE
                x_engine->callback(CALLBACK_RESIZE_GUI, m_id, width, height, 1.0, nullptr);
#endif
            }
        }

        // open UI
        if (effect->dispatcher(effect, effEditOpen, 0, value, ptr, 0.0f) == 1)
        {
            // get UI size again, can't fail now
            vstRect = nullptr;
            effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f);

            if (vstRect)
            {
                int width  = vstRect->right  - vstRect->left;
                int height = vstRect->bottom - vstRect->top;

                if (width <= 0 || height <= 0)
                {
                    carla_stderr2("VstPlugin::setGuiContainer(%p) - failed to get proper editor size", container);
                    return;
                }

                gui.width  = width;
                gui.height = height;

                container->setFixedSize(width, height);
                carla_debug("VstPlugin::setGuiContainer(%p) -> setFixedSize(%i, %i)", container, width, height);
            }
            else
                carla_stderr2("VstPlugin::setGuiContainer(%p) - failed to get plugin editor size", container);
        }
        else
        {
            // failed to open UI
            carla_stderr("VstPlugin::setGuiContainer(%p) - failed to open UI", container);

            m_hints &= ~PLUGIN_HAS_GUI;
            x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0, nullptr);

            effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
        }
    }
#endif

    void showGui(const bool yesNo)
    {
        if (fGui.isOsc)
        {
            if (yesNo)
            {
                kData->osc.thread.start();
            }
            else
            {
                if (kData->osc.data.target != nullptr)
                {
                    osc_send_hide(&kData->osc.data);
                    osc_send_quit(&kData->osc.data);
                    kData->osc.data.free();
                }

                if (kData->osc.thread.isRunning() && ! kData->osc.thread.stop(kData->engine->getOptions().oscUiTimeout))
                    kData->osc.thread.terminate();
            }
        }
        else
        {
            if (yesNo && fGui.width > 0 && fGui.height > 0 && kData->gui != nullptr)
                kData->gui->setFixedSize(fGui.width, fGui.height);
        }

        fGui.isVisible = yesNo;
    }

    void idleGui()
    {
#ifdef VESTIGE_HEADER
        if (fEffect != nullptr /*&& effect->ptr1*/)
#else
        if (fEffect != nullptr /*&& effect->resvd1*/)
#endif
        {
            if (fNeedIdle)
                dispatcher(effIdle, 0, 0, nullptr, 0.0f);

            if (! fGui.isOsc && fGui.isVisible)
                dispatcher(effEditIdle, 0, 0, nullptr, 0.0f);
        }

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        carla_debug("VstPlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fEffect != nullptr);

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        deleteBuffers();

        uint32_t aIns, aOuts, mIns, mOuts, params, j;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = fEffect->numInputs;
        aOuts  = fEffect->numOutputs;
        params = fEffect->numParams;

        if (vstPluginCanDo(fEffect, "receiveVstEvents") || vstPluginCanDo(fEffect, "receiveVstMidiEvent") || (fEffect->flags & effFlagsIsSynth) > 0 || (fHints & PLUGIN_WANTS_MIDI_INPUT))
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
            kData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (params > 0)
        {
            kData->param.createNew(params);
            needsCtrlIn = true;
        }

        const uint  portNameSize = kData->engine->maxPortNameSize();
        CarlaString portName;

        // Audio Ins
        for (j=0; j < aIns; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
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

            kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
            kData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (j=0; j < aOuts; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
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

            kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[j].rindex = j;
        }

        for (j=0; j < params; j++)
        {
            kData->param.data[j].type   = PARAMETER_INPUT;
            kData->param.data[j].index  = j;
            kData->param.data[j].rindex = j;
            kData->param.data[j].hints  = 0x0;
            kData->param.data[j].midiChannel = 0;
            kData->param.data[j].midiCC = -1;

            float min, max, def, step, stepSmall, stepLarge;

            VstParameterProperties prop;
            carla_zeroMem(&prop, sizeof(VstParameterProperties));

            if (fHints & PLUGIN_HAS_COCKOS_EXTENSIONS)
            {
                double range[2] = { 0.0, 1.0 };

                if (dispatcher(effVendorSpecific, 0xdeadbef0, j, range, 0.0f) >= 0xbeef)
                {
                    min = range[0];
                    max = range[1];

                    if (min > max)
                        max = min;
                    else if (max < min)
                        min = max;

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

                if (dispatcher(effVendorSpecific, kVstParameterUsesIntStep, j, nullptr, 0.0f) >= 0xbeef)
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
            else if (dispatcher(effGetParameterProperties, j, 0, &prop, 0) == 1)
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
                    kData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (prop.flags & kVstParameterUsesIntStep)
                {
                    step = float(prop.stepInteger);
                    stepSmall = float(prop.stepInteger)/10;
                    stepLarge = float(prop.largeStepInteger);
                    kData->param.data[j].hints |= PARAMETER_IS_INTEGER;
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
                    kData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;
            }
            else
            {
                min = 0.0f;
                max = 1.0f;
                step = 0.001f;
                stepSmall = 0.0001f;
                stepLarge = 0.1f;
            }

            kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
#ifndef BUILD_BRIDGE
            kData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
#endif

            if ((fHints & PLUGIN_USES_OLD_VSTSDK) != 0 || dispatcher(effCanBeAutomated, j, 0, nullptr, 0.0f) == 1)
                kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            // no such thing as VST default parameters
            def = fEffect->getParameter(fEffect, j);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            kData->param.ranges[j].min = min;
            kData->param.ranges[j].max = max;
            kData->param.ranges[j].def = def;
            kData->param.ranges[j].step = step;
            kData->param.ranges[j].stepSmall = stepSmall;
            kData->param.ranges[j].stepLarge = stepLarge;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin hints
        const intptr_t vstCategory = dispatcher(effGetPlugCategory, 0, 0, nullptr, 0.0f);

        fHints = 0x0;

        if (vstCategory == kPlugCategSynth || vstCategory == kPlugCategGenerator)
            fHints |= PLUGIN_IS_SYNTH;

        if (fEffect->flags & effFlagsHasEditor)
        {
            fHints |= PLUGIN_HAS_GUI;

            if (! fGui.isOsc)
                fHints |= PLUGIN_HAS_SINGLE_THREAD;
        }

        if (dispatcher(effGetVstVersion, 0, 0, nullptr, 0.0f) < kVstVersion)
            fHints |= PLUGIN_USES_OLD_VSTSDK;

        if ((fEffect->flags & effFlagsCanReplacing) != 0 && fEffect->processReplacing != fEffect->process)
            fHints |= PLUGIN_CAN_PROCESS_REPLACING;

        if (fEffect->flags & effFlagsHasEditor)
            fHints |= PLUGIN_HAS_GUI;

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, (void*)"hasCockosExtensions", 0.0f)) == 0xbeef0000)
            fHints |= PLUGIN_HAS_COCKOS_EXTENSIONS;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        kData->extraHints = 0x0;

        if (mIns > 0)
            kData->extraHints |= PLUGIN_HINT_HAS_MIDI_IN;

        if (mOuts > 0)
            kData->extraHints |= PLUGIN_HINT_HAS_MIDI_OUT;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        // plugin options
        fOptions = 0x0;

        fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fEffect->flags & effFlagsProgramChunks)
            fOptions |= PLUGIN_OPTION_USE_CHUNKS;

#ifdef CARLA_OS_WIN
        // Most Windows plugins have issues with this
        fOptions |= PLUGIN_OPTION_FIXED_BUFFER;
#endif

        if (mIns > 0)
        {
            fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
            fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        // dummy pre-start to catch latency and possible wantEvents() call on old plugins
        {
            dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
            dispatcher(effStartProcess, 0, 0, nullptr, 0.0f);
            dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
            dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
        }

        // check latency
        if (fHints & PLUGIN_CAN_DRYWET)
        {
#ifdef VESTIGE_HEADER
            char* const empty3Ptr = &fEffect->empty3[0];
            int32_t* initialDelayPtr = (int32_t*)empty3Ptr;
            kData->latency = *initialDelayPtr;
#else
            kData->latency = fEffect->initialDelay;
#endif

            kData->client->setLatency(kData->latency);
            recreateLatencyBuffers();
        }

        // special plugin fixes
        // 1. IL Harmless - disable threaded processing
        if (fEffect->uniqueID == 1229484653)
        {
            char strBuf[STR_MAX] = { 0 };
            getLabel(strBuf);

            if (std::strcmp(strBuf, "IL Harmless") == 0)
            {
                // TODO - disable threaded processing
            }
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("VstPlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        carla_debug("VstPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = kData->prog.count;
        const int32_t current = kData->prog.current;

        // Delete old programs
        kData->prog.clear();

        // Query new programs
        uint32_t count = static_cast<uint32_t>(fEffect->numPrograms);

        if (count > 0)
        {
            kData->prog.createNew(count);

            // Update names
            for (i=0; i < count; i++)
            {
                char strBuf[STR_MAX] = { 0 };
                if (dispatcher(effGetProgramNameIndexed, i, 0, strBuf, 0.0f) != 1)
                {
                    // program will be [re-]changed later
                    dispatcher(effSetProgram, 0, i, nullptr, 0.0f);
                    dispatcher(effGetProgramName, 0, 0, strBuf, 0.0f);
                }
                kData->prog.names[i] = strdup(strBuf);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_program_count(fId, count);

            for (i=0; i < count; i++)
                kData->engine->osc_send_control_set_program_name(fId, i, kData->prog.names[i]);
        }
#endif

        if (init)
        {
            if (count > 0)
                setProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (count == oldCount+1)
            {
                // one program added, probably created by user
                kData->prog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                kData->prog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                kData->prog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current program > count
                kData->prog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                kData->prog.current = current;
            }

            if (programChanged)
            {
                setProgram(kData->prog.current, true, true, true);
            }
            else
            {
                // Program was changed during update, re-set it
                if (kData->prog.current >= 0)
                    dispatcher(effSetProgram, 0, kData->prog.current, nullptr, 0.0f);
            }

            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

#if 0
        // reset MIDI
        events.numEvents = 0;
        midiEvents[0].type = 0;

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.portCin && m_active && m_activeBefore)
        {
            bool allNotesOffSent = false;

            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            for (i=0; i < nEvents; i++)
            {
                cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case CarlaEngineNullEvent:
                    break;

                case CarlaEngineParameterChangeEvent:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->parameter) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->parameter) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_BALANCE(cinEvent->parameter) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cinEvent->value/0.5 - 1.0;

                            if (value < 0.0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0.0)
                            {
                                left  = (value*2)-1.0;
                                right = 1.0;
                            }
                            else
                            {
                                left  = -1.0;
                                right = 1.0;
                            }

                            setBalanceLeft(left, false, false);
                            setBalanceRight(right, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midiChannel != cinEvent->channel)
                            continue;
                        if (param.data[k].midiCC != cinEvent->parameter)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cinEvent->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cinEvent->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeEvent(PluginPostEventParameterChange, k, 0, value);
                        }
                    }

                    break;
                }

                case CarlaEngineMidiBankChangeEvent:
                    break;

                case CarlaEngineMidiProgramChangeEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        uint32_t progId = rint(cinEvent->value);

                        if (progId < prog.count)
                        {
                            setProgram(progId, false, false, false, false);
                            postponeEvent(PluginPostEventProgramChange, progId, 0, 0.0);
                        }
                    }
                    break;

                case CarlaEngineAllSoundOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                        effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
                        effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                        effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineAllNotesOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        allNotesOffSent = true;
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input

        if (midi.portMin && m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    VstMidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->midiData[0] = uint8_t(extMidiNotes[i].velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) + extMidiNotes[i].channel;
                    midiEvent->midiData[1] = extMidiNotes[i].note;
                    midiEvent->midiData[2] = extMidiNotes[i].velo;

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

            {
                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = midi.portMin->getEventCount();

                for (i=0; i < nEvents && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    minEvent = midi.portMin->getEvent(i);

                    if (! minEvent)
                        continue;

                    time = minEvent->time;

                    if (time >= frames)
                        continue;

                    uint8_t status  = minEvent->data[0];
                    uint8_t channel = status & 0x0F;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                        status -= 0x10;

                    VstMidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->deltaFrames = minEvent->time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        uint8_t note = minEvent->data[1];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;

                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        uint8_t note = minEvent->data[1];
                        uint8_t velo = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;
                        midiEvent->midiData[2] = velo;

                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        uint8_t note     = minEvent->data[1];
                        uint8_t pressure = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;
                        midiEvent->midiData[2] = pressure;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                    {
                        uint8_t pressure = minEvent->data[1];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        uint8_t lsb = minEvent->data[1];
                        uint8_t msb = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = lsb;
                        midiEvent->midiData[2] = msb;
                    }
                    else
                        continue;

                    midiEventCount += 1;
                }
            } // End of MIDI Input (System)

        } // End of MIDI Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (midi.portMin)
                {
                    for (k=0; k < MAX_MIDI_CHANNELS; k++)
                    {
                        memset(&midiEvents[k], 0, sizeof(VstMidiEvent));
                        midiEvents[k].type = kVstMidiType;
                        midiEvents[k].byteSize = sizeof(VstMidiEvent);
                        midiEvents[k].midiData[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                        midiEvents[k].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        memset(&midiEvents[k*2], 0, sizeof(VstMidiEvent));
                        midiEvents[k*2].type = kVstMidiType;
                        midiEvents[k*2].byteSize = sizeof(VstMidiEvent);
                        midiEvents[k*2].midiData[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                        midiEvents[k*2].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    }

                    midiEventCount = MAX_MIDI_CHANNELS*2;
                }

                if (m_latency > 0)
                {
                    for (i=0; i < aIn.count; i++)
                        memset(m_latencyBuffers[i], 0, sizeof(float)*m_latency);
                }

                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
            }

            if (midiEventCount > 0)
            {
                events.numEvents = midiEventCount;
                events.reserved  = 0;
                effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
            }

            // FIXME - make this a global option
            // don't process if not needed
            //if ((effect->flags & effFlagsNoSoundInStop) > 0 && aInsPeak[0] == 0.0 && aInsPeak[1] == 0.0 && midiEventCount == 0 && ! midi.portMout)
            //{
            if (m_hints & PLUGIN_CAN_PROCESS_REPLACING)
            {
                isProcessing = true;
                effect->processReplacing(effect, inBuffer, outBuffer, frames);
                isProcessing = false;
            }
            else
            {
                for (i=0; i < aOut.count; i++)
                    carla_zeroF(outBuffer[i], frames);

#if ! VST_FORCE_DEPRECATED
                isProcessing = true;
                effect->process(effect, inBuffer, outBuffer, frames);
                isProcessing = false;
#endif
            }
            //}
        }
        else
        {
            if (m_activeBefore)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_dryWet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_volume != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_balanceLeft != -1.0 || x_balanceRight != 1.0);

            double bal_rangeL, bal_rangeR;
            float bufValue, oldBufLeft[do_balance ? frames : 1];

            for (i=0; i < aOut.count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (k < m_latency && m_latency < frames)
                            bufValue = (aIn.count == 1) ? m_latencyBuffers[0][k] : m_latencyBuffers[i][k];
                        else
                            bufValue = (aIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(bufValue*(1.0-x_dryWet));
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_balanceLeft+1.0)/2;
                    bal_rangeR = (x_balanceRight+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_volume;
                }

                // Output VU
                if (x_engine->getOptions().processMode != PROCESS_MODE_CONTINUOUS_RACK)
                {
                    for (k=0; i < 2 && k < frames; k++)
                    {
                        if (std::abs(outBuffer[i][k]) > aOutsPeak[i])
                            aOutsPeak[i] = std::abs(outBuffer[i][k]);
                    }
                }
            }

            // Latency, save values for next callback
            if (m_latency > 0 && m_latency < frames)
            {
                for (i=0; i < aIn.count; i++)
                    memcpy(m_latencyBuffers[i], inBuffer[i] + (frames - m_latency), sizeof(float)*m_latency);
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aOut.count; i++)
                carla_zeroF(outBuffer[i], frames);

            aOutsPeak[0] = 0.0;
            aOutsPeak[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (midi.portMout && m_active)
        {
            uint8_t data[3] = { 0 };

            for (int32_t i = midiEventCount; i < events.numEvents; i++)
            {
                data[0] = midiEvents[i].midiData[0];
                data[1] = midiEvents[i].midiData[1];
                data[2] = midiEvents[i].midiData[2];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(data[0]) && data[2] == 0)
                    data[0] -= 0x10;

                midi.portMout->writeEvent(midiEvents[i].deltaFrames, data, 3);
            }
        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, aInsPeak[0]);
        x_engine->setInputPeak(m_id, 1, aInsPeak[1]);
        x_engine->setOutputPeak(m_id, 0, aOutsPeak[0]);
        x_engine->setOutputPeak(m_id, 1, aOutsPeak[1]);

        m_activeBefore = m_active;
#endif
    }

    void bufferSizeChanged(uint32_t newBufferSize)
    {
        if (kData->active)
        {
            dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
            dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
        }

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, newBufferSize, nullptr, kData->engine->getSampleRate());
#endif
        dispatcher(effSetBlockSize, 0, newBufferSize, nullptr, 0.0f);

        if (kData->active)
        {
            dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
            dispatcher(effStartProcess, 0, 0, nullptr, 0.0f);
        }
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void uiParameterChange(const uint32_t index, const double value)
    {
        CARLA_ASSERT(index < kData->param.count);

        if (index >= kData->param.count)
            return;
        if (! fGui.isOsc)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_control(&kData->osc.data, kData->param.data[index].rindex, value);
    }

    void uiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < kData->prog.count);

        if (index >= kData->prog.count)
            return;
        if (! fGui.isOsc)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_program(&kData->osc.data, index);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);
        CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (velo >= MAX_MIDI_VALUE)
            return;
        if (! fGui.isOsc)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(&kData->osc.data, midiData);
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (! fGui.isOsc)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;

        osc_send_midi(&kData->osc.data, midiData);
    }

    // -------------------------------------------------------------------

#if 0
    intptr_t handleAudioMasterTempoAt()
    {
        const CarlaEngineTimeInfo* const timeInfo = x_engine->getTimeInfo();

        if (timeInfo->valid & CarlaEngineTimeBBT)
            return timeInfo->bbt.beats_per_minute * 10000;

        return 0;
    }

    intptr_t handleAudioMasterIOChanged()
    {
        carla_debug("VstPlugin::handleAudioMasterIOChanged()");
        CARLA_ASSERT(m_enabled);

        // TESTING

        if (! m_enabled)
            return 1;

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

        return 1;
    }

    intptr_t handleAudioMasterProcessEvents(const VstEvents* const vstEvents)
    {
        CARLA_ASSERT(m_enabled);
        CARLA_ASSERT(midi.portMout);
        CARLA_ASSERT(isProcessing);

        if (! m_enabled)
            return 0;

        if (! midi.portMout)
            return 0;

        if (! isProcessing)
        {
            carla_stderr2("VstPlugin::handleAudioMasterProcessEvents(%p) - received MIDI out events outside audio thread, ignoring", vstEvents);
            return 0;
        }

        for (int32_t i=0; i < vstEvents->numEvents && events.numEvents < MAX_MIDI_EVENTS*2; i++)
        {
            if (! vstEvents->events[i])
                break;

            const VstMidiEvent* const vstMidiEvent = (const VstMidiEvent*)vstEvents->events[i];

            if (vstMidiEvent->type == kVstMidiType)
                memcpy(&midiEvents[events.numEvents++], vstMidiEvent, sizeof(VstMidiEvent));
        }

        return 1;
    }

    intptr_t handleAdioMasterSizeWindow(int32_t width, int32_t height)
    {
        carla_debug("VstPlugin::handleAudioMasterSizeWindow(%i, %i)", width, height);

        gui.width  = width;
        gui.height = height;

        x_engine->callback(CALLBACK_RESIZE_GUI, m_id, width, height, 0.0, nullptr);

        return 1;
    }

    void handleAudioMasterUpdateDisplay()
    {
        carla_debug("VstPlugin::handleAudioMasterUpdateDisplay()");

        // Update current program name
        if (prog.count > 0 && prog.current >= 0)
        {
            const int32_t index = prog.current;
            char strBuf[STR_MAX] = { 0 };

            effect->dispatcher(effect, effGetProgramName, 0, 0, strBuf, 0.0f);

            if (! prog.names[index])
            {
                prog.names[index] = strdup(strBuf);
            }
            else if (strBuf[0] != 0 && std::strcmp(strBuf, prog.names[index]) != 0)
            {
                free((void*)prog.names[index]);
                prog.names[index] = strdup(strBuf);
            }
        }

        // Tell backend to update
        x_engine->callback(CALLBACK_UPDATE, m_id, 0, 0, 0.0, nullptr);
    }
#endif

protected:
    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        carla_debug("VstPlugin::dispatcher(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstEffectOpcode2str(opcode), index, value, ptr, opt);
        CARLA_ASSERT(fEffect != nullptr);

        return (fEffect != nullptr) ? fEffect->dispatcher(fEffect, opcode, index, value, ptr, opt) : 0;
    }

    intptr_t handleAudioMasterCallback(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#if 0
        // Cockos VST extensions
        if (/*effect &&*/ ptr && (uint32_t)opcode == 0xdeadbeef && (uint32_t)index == 0xdeadf00d)
        {
            const char* const func = (char*)ptr;

            if (std::strcmp(func, "GetPlayPosition") == 0)
                return 0;
            if (std::strcmp(func, "GetPlayPosition2") == 0)
                return 0;
            if (std::strcmp(func, "GetCursorPosition") == 0)
                return 0;
            if (std::strcmp(func, "GetPlayState") == 0)
                return 0;
            if (std::strcmp(func, "SetEditCurPos") == 0)
                return 0;
            if (std::strcmp(func, "GetSetRepeat") == 0)
                return 0;
            if (std::strcmp(func, "GetProjectPath") == 0)
                return 0;
            if (std::strcmp(func, "OnPlayButton") == 0)
                return 0;
            if (std::strcmp(func, "OnStopButton") == 0)
                return 0;
            if (std::strcmp(func, "OnPauseButton") == 0)
                return 0;
            if (std::strcmp(func, "IsInRealTimeAudio") == 0)
                return 0;
            if (std::strcmp(func, "Audio_IsRunning") == 0)
                return 0;
        }
#endif

        intptr_t ret = 0;

        switch (opcode)
        {
        case audioMasterAutomate:
            //CARLA_ASSERT(fEnabled);
            CARLA_ASSERT_INT(index < static_cast<int32_t>(kData->param.count), index);

            if (index < 0 || index >= static_cast<int32_t>(kData->param.count) /*|| ! fEnabled*/)
                break;

            if (fIsProcessing && ! kData->engine->isOffline())
            {
                setParameterValue(index, opt, false, false, false);
                postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, opt);
            }
            else
            {
                CARLA_ASSERT(fGui.isVisible); // FIXME - remove when offline is implemented
                setParameterValue(index, opt, fIsProcessing, true, true);
            }
            break;

        case audioMasterCurrentId:
            // TODO
            // if using old sdk, return effect->uniqueID
            break;

        case audioMasterIdle:
            dispatcher(effEditIdle, 0, 0, nullptr, 0.0f);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterPinConnected:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterWantMidi:
            // Deprecated in VST SDK 2.4
            fHints |= PLUGIN_WANTS_MIDI_INPUT;
            break;
#endif

        case audioMasterGetTime:
            CARLA_ASSERT(fIsProcessing);

#ifdef VESTIGE_HEADER
            ret = getAddressFromPointer(&fTimeInfo);
#else
            ret = ToVstPtr<VstTimeInfo_R>(&fTimeInfo);
#endif
            break;

#if 0
        case audioMasterProcessEvents:
            CARLA_ASSERT(self && ptr);
            if (self)
            {
                if (ptr)
                    ret = self->handleAudioMasterProcessEvents((const VstEvents*)ptr);
                else
                    carla_stderr("VstPlugin::hostCallback::audioMasterProcessEvents called with invalid pointer");
            }
            else
                carla_stderr("VstPlugin::hostCallback::audioMasterProcessEvents called without valid object");
            break;
#endif

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetTime:
            // Deprecated in VST SDK 2.4
            break;

#if 0
        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            CARLA_ASSERT(self);
            if (self)
                ret = self->handleAudioMasterTempoAt();
            else
                carla_stderr("VstPlugin::hostCallback::audioMasterTempoAt called without valid object");
            if (ret == 0)
                ret = 120 * 10000;
            break;
#endif

        case audioMasterGetNumAutomatableParameters:
            // Deprecated in VST SDK 2.4
            ret = carla_min<intptr_t>(0, fEffect->numParams, kData->engine->getOptions().maxParameters);
            break;

        case audioMasterGetParameterQuantization:
            // Deprecated in VST SDK 2.4
            ret = 1; // full single float precision
            break;
#endif

#if 0
        case audioMasterIOChanged:
            CARLA_ASSERT(self);
            if (self)
                ret = self->handleAudioMasterIOChanged();
            else
                carla_stderr("VstPlugin::hostCallback::audioMasterIOChanged called without valid object");
            break;
#endif

        case audioMasterNeedIdle:
            // Deprecated in VST SDK 2.4
            fNeedIdle = true;
            break;

#if 0
        case audioMasterSizeWindow:
            CARLA_ASSERT(self);
            if (self)
            {
                if (index > 0 && value > 0)
                    ret = self->handleAdioMasterSizeWindow(index, value);
                else
                    carla_stderr("VstPlugin::hostCallback::audioMasterSizeWindow called with invalid size");
            }
            else
                carla_stderr("VstPlugin::hostCallback::audioMasterSizeWindow called without valid object");
            break;
#endif

        case audioMasterGetSampleRate:
            ret = kData->engine->getSampleRate();
            break;

        case audioMasterGetBlockSize:
            ret = kData->engine->getBufferSize();
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
            if (kData->engine->isOffline())
                ret = kVstProcessLevelOffline;
            else if (fIsProcessing)
                ret = kVstProcessLevelRealtime;
            else
                ret = kVstProcessLevelUser;
            break;

        case audioMasterGetAutomationState:
            ret = kVstAutomationReadWrite;
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

        case audioMasterGetLanguage:
            ret = kVstLangEnglish;
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterOpenWindow:
        case audioMasterCloseWindow:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterGetDirectory:
            // TODO
            //if (ptr)
            //    std::strcpy((char*)ptr, "stuff");
            //else
            //    carla_stderr("VstPlugin::hostCallback::audioMasterGetDirectory called with invalid pointer");
            break;

#if 0
        case audioMasterUpdateDisplay:
            CARLA_ASSERT(effect);
            if (self)
                self->handleAudioMasterUpdateDisplay();
            if (effect)
                effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            ret = 1;
            break;
#endif

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
            carla_debug("VstPlugin::handleAudioMasterCallback(%02i:%s, %i, " P_INTPTR ", %p, %f)", opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
            break;
        }

        return ret;
    }

public:
    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(filename != nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(filename))
        {
            kData->engine->setLastError(libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        VST_Function vstFn = (VST_Function)libSymbol("VSTPluginMain");

        if (vstFn == nullptr)
        {
            vstFn = (VST_Function)libSymbol("main");

            if (vstFn == nullptr)
            {
                kData->engine->setLastError("Could not find the VST main entry in the plugin library");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 1)

        sLastVstPlugin = this;
        fEffect = vstFn(carla_vst_audioMasterCallback);
        sLastVstPlugin = nullptr;

        if (fEffect == nullptr || fEffect->magic != kEffectMagic)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

#ifdef VESTIGE_HEADER
        fEffect->ptr1 = this;
#else
        fEffect->resvd1 = getAddressFromPointer(this);
#endif

        dispatcher(effOpen, 0, 0, nullptr, 0.0f);

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
        {
            fName = kData->engine->getNewUniquePluginName(name);
        }
        else
        {
            char strBuf[STR_MAX] = { 0 };
            dispatcher(effGetEffectName, 0, 0, strBuf, 0.0f);

            if (strBuf[0] != 0)
                fName = kData->engine->getNewUniquePluginName(strBuf);
            else
                fName = kData->engine->getNewUniquePluginName(label);
        }

        fFilename = filename;

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin (part 2)

#if ! VST_FORCE_DEPRECATED
        dispatcher(effSetBlockSizeAndSampleRate, 0, kData->engine->getBufferSize(), nullptr, kData->engine->getSampleRate());
#endif
        dispatcher(effSetSampleRate, 0, 0, nullptr, kData->engine->getSampleRate());
        dispatcher(effSetBlockSize, 0, kData->engine->getBufferSize(), nullptr, 0.0f);
        dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

        dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
        dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);

        if (dispatcher(effGetVstVersion, 0, 0, nullptr, 0.0f) < kVstVersion)
            fHints |= PLUGIN_USES_OLD_VSTSDK;

        if (static_cast<uintptr_t>(dispatcher(effCanDo, 0, 0, (void*)"hasCockosExtensions", 0.0f)) == 0xbeef0000)
            fHints |= PLUGIN_HAS_COCKOS_EXTENSIONS;

        // ---------------------------------------------------------------
        // gui stuff

        if (fEffect->flags & effFlagsHasEditor)
        {
            const EngineOptions& engineOptions(kData->engine->getOptions());

            if (engineOptions.preferUiBridges && engineOptions.bridge_vstx11.isNotEmpty() && (fEffect->flags & effFlagsProgramChunks) == 0)
            {
                kData->osc.thread.setOscData(engineOptions.bridge_vstx11, label);
                fGui.isOsc = true;
            }
        }

        return true;
    }

private:
    int fUnique1;
    AEffect* fEffect;

    QByteArray    fChunk;
    VstMidiEvent  fMidiEvents[MAX_MIDI_EVENTS*2];
    VstTimeInfo_R fTimeInfo;

    struct FixedVstEvents {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_MIDI_EVENTS*2];

#ifndef QTCREATOR_TEST // missing proper C++11 support
        FixedVstEvents()
            : numEvents(0),
              reserved(0),
              data{0} {}
#endif
    } fEvents;

    struct GuiInfo {
        bool isOsc;
        bool isVisible;
        int width;
        int height;

        GuiInfo()
            : isOsc(false),
              isVisible(false),
              width(0),
              height(0) {}
    } fGui;

    bool fIsProcessing;
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
#ifdef DEBUG
        if (opcode != audioMasterGetTime && opcode != audioMasterProcessEvents && opcode != audioMasterGetCurrentProcessLevel && opcode != audioMasterGetOutputLatency)
            carla_debug("carla_vst_audioMasterCallback(%p, %02i:%s, %i, " P_INTPTR ", %p, %f)", effect, opcode, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

        switch (opcode)
        {
        case audioMasterVersion:
            return kVstVersion;

        case audioMasterGetVendorString:
            CARLA_ASSERT(ptr != nullptr);

            if (ptr != nullptr)
            {
                std::strcpy((char*)ptr, "falkTX");
                return 1;
            }
            else
            {
                carla_stderr("carla_vst_audioMasterCallback() - audioMasterGetVendorString called with invalid pointer");
                return 0;
            }

        case audioMasterGetProductString:
            CARLA_ASSERT(ptr != nullptr);

            if (ptr != nullptr)
            {
                std::strcpy((char*)ptr, "Carla");
                return 1;
            }
            else
            {
                carla_stderr("carla_vst_audioMasterCallback() - audioMasterGetProductString called with invalid pointer");
                return 0;
            }

        case audioMasterGetVendorVersion:
            return 0x1000; // 1.0.0

        case audioMasterCanDo:
            CARLA_ASSERT(ptr != nullptr);

            if (ptr != nullptr)
            {
                return carla_vst_hostCanDo((const char*)ptr);
            }
            else
            {
                carla_stderr("carla_vst_audioMasterCallback() - audioMasterCanDo called with invalid pointer");
                return 0;
            }
        }

        // Check if 'resvd1' points to us, otherwise register ourselfs if possible
        VstPlugin* self = nullptr;

        if (effect != nullptr)
        {
#ifdef VESTIGE_HEADER
            if (effect->ptr1 != nullptr)
            {
                self = (VstPlugin*)effect->ptr1;
#else
            if (effect->resvd1 != 0)
            {
                self = FromVstPtr<VstPlugin>(effect->resvd1);
#endif
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
            else if (sLastVstPlugin != nullptr)
            {
#ifdef VESTIGE_HEADER
                effect->ptr1 = sLastVstPlugin;
#else
                effect->resvd1 = ToVstPtr<VstPlugin>(sLastVstPlugin);
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

#else // WANT_VST
# warning Building without VST support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newVST(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef WANT_VST
    VstPlugin* const plugin = new VstPlugin(init.engine, init.id);

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo VST plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("VST support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
