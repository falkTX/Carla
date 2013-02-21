/*
 * Carla DSSI Plugin
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

#ifdef WANT_DSSI

#include "CarlaLadspaUtils.hpp"
#include "dssi/dssi.h"

CARLA_BACKEND_START_NAMESPACE

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id)
    {
        carla_debug("DssiPlugin::DssiPlugin(%p, %i)", engine, id);

        fHandle  = nullptr;
        fHandle2 = nullptr;
        fDescriptor = nullptr;
        fDssiDescriptor = nullptr;

        fAudioInBuffers  = nullptr;
        fAudioOutBuffers = nullptr;
        fParamBuffers    = nullptr;

        carla_zeroMem(fMidiEvents, sizeof(snd_seq_event_t)*MAX_MIDI_EVENTS);

        if (engine->getOptions().useDssiVstChunks)
            fOptions |= PLUGIN_OPTION_USE_CHUNKS;
    }

    ~DssiPlugin()
    {
        carla_debug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            // Wait a bit first, then force kill
            if (kData->osc.thread.isRunning() && ! kData->osc.thread.stop(kData->engine->getOptions().oscUiTimeout))
            {
                carla_stderr("DSSI GUI thread still running, forcing termination now");
                kData->osc.thread.terminate();
            }
        }

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->deactivate != nullptr && kData->activeBefore)
            {
                if (fHandle != nullptr)
                    fDescriptor->deactivate(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->deactivate(fHandle2);
            }

            if (fDescriptor->cleanup != nullptr)
            {
                if (fHandle != nullptr)
                    fDescriptor->cleanup(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->cleanup(fHandle2);
            }

            fHandle  = nullptr;
            fHandle2 = nullptr;
            fDescriptor = nullptr;
            fDssiDescriptor = nullptr;
        }

        deleteBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const
    {
        return PLUGIN_DSSI;
    }

    PluginCategory category() const
    {
        if (fHints & PLUGIN_IS_SYNTH)
            return PLUGIN_CATEGORY_SYNTH;

        return getPluginCategoryFromName(fName);
    }

    long uniqueId() const
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        return (fDescriptor != nullptr) ? fDescriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        CARLA_ASSERT(fHints & PLUGIN_USES_CHUNKS);
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fDssiDescriptor->get_custom_data != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(fHandle2 == nullptr);
        CARLA_ASSERT(dataPtr != nullptr);

        unsigned long dataSize = 0;

        if (fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->get_custom_data(fHandle, dataPtr, &dataSize))
            return dataSize;

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    float getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParamBuffers[parameterId];
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Label != nullptr)
            std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Maker != nullptr)
            std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Name != nullptr)
            std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex = kData->param.data[parameterId].rindex;

        if (fDescriptor != nullptr && rindex < static_cast<int32_t>(fDescriptor->PortCount))
            std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue = kData->param.fixValue(parameterId, value);
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (type == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_STRING) != 0)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        if (fDssiDescriptor->configure != nullptr)
        {
            fDssiDescriptor->configure(fHandle, key, value);

            if (fHandle2)
                fDssiDescriptor->configure(fHandle2, key, value);
        }

        if (sendGui && kData->osc.data.target != nullptr)
            osc_send_configure(&kData->osc.data, key, value);

        if (strcmp(key, "reloadprograms") == 0 || strcmp(key, "load") == 0 || strncmp(key, "patches", 7) == 0)
        {
            const ScopedDisabler m(this); // FIXME
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData)
    {
        CARLA_ASSERT(fHints & PLUGIN_USES_CHUNKS);
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fDssiDescriptor->set_custom_data != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(fHandle2 == nullptr);
        CARLA_ASSERT(stringData != nullptr);

        if (fDssiDescriptor->set_custom_data == nullptr)
            return;

        // FIXME
        fChunk = QByteArray::fromBase64(QByteArray(stringData));
        //fChunk.toBase64();

        if (kData->engine->isOffline())
        {
            //const CarlaEngine::ScopedLocker m(kData->engine);
            fDssiDescriptor->set_custom_data(fHandle, fChunk.data(), fChunk.size());
        }
        else
        {
            const CarlaPlugin::ScopedDisabler m(this);
            fDssiDescriptor->set_custom_data(fHandle, fChunk.data(), fChunk.size());
        }
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        // FIXME
        if (fDssiDescriptor != nullptr && fHandle != nullptr && index >= 0)
        {
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            if (kData->engine->isOffline())
            {
                //const CarlaEngine::ScopedLocker m(x_engine, block);
                fDssiDescriptor->select_program(fHandle, bank, program);

                if (fHandle2 != nullptr)
                    fDssiDescriptor->select_program(fHandle2, bank, program);
            }
            else
            {
                //const ScopedDisabler m(this, block);

                fDssiDescriptor->select_program(fHandle, bank, program);

                if (fHandle2 != nullptr)
                    fDssiDescriptor->select_program(fHandle2, bank, program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
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

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        carla_debug("DssiPlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->client->isActive())
            kData->client->deactivate();

        deleteBuffers();

        const double sampleRate = kData->engine->getSampleRate();
        const unsigned long portCount = fDescriptor->PortCount;

        uint32_t aIns, aOuts, mIns, params, j;
        aIns = aOuts = mIns = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (unsigned long i=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType = fDescriptor->PortDescriptors[i];

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                if (LADSPA_IS_PORT_INPUT(portType))
                    aIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                    aOuts += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
                params += 1;
        }

        if ((fOptions & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1))
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate);

            if (aIns == 1)
            {
                aIns = 2;
                forcedStereoIn = true;
            }

            if (aOuts == 1)
            {
                aOuts = 2;
                forcedStereoOut = true;
            }
        }

        if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
        {
            mIns = 1;
            needsCtrlIn = true;
        }

        if (aIns > 0)
        {
            kData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; i++)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; i++)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            kData->param.createNew(params);
            fParamBuffers = new float[params];
        }

        const int   portNameSize = kData->engine->maxPortNameSize();
        CarlaString portName;

        for (unsigned long i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += fDescriptor->PortNames[i];
                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    j = iAudioIn++;
                    kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                    kData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        kData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                        kData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    j = iAudioOut++;
                    kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                    kData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                        kData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                j = iCtrl++;
                kData->param.data[j].index  = j;
                kData->param.data[j].rindex = i;
                kData->param.data[j].hints  = 0x0;
                kData->param.data[j].midiChannel = 0;
                kData->param.data[j].midiCC = -1;

                float min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHints.HintDescriptor))
                    min = portRangeHints.LowerBound;
                else
                    min = 0.0f;

                // max value
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHints.HintDescriptor))
                    max = portRangeHints.UpperBound;
                else
                    max = 1.0f;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0f)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': max - min == 0.0f", fDescriptor->PortNames[i]);
                    max = min + 0.1f;
                }

                // default value
                def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    kData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portRangeHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    kData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portRangeHints.HintDescriptor))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    kData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    kData->param.data[j].type   = PARAMETER_INPUT;
                    kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                    kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needsCtrlIn = true;

                    // MIDI CC value
                    if (fDssiDescriptor->get_midi_controller_for_port != nullptr)
                    {
                        int controller = fDssiDescriptor->get_midi_controller_for_port(fHandle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                kData->param.data[j].midiCC = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    if (std::strcmp(fDescriptor->PortNames[i], "latency") == 0 || std::strcmp(fDescriptor->PortNames[i], "_latency") == 0)
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        kData->param.data[j].type  = PARAMETER_LATENCY;
                        kData->param.data[j].hints = 0;
                    }
                    else if (std::strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        kData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        kData->param.data[j].hints = 0;
                    }
                    else
                    {
                        kData->param.data[j].type   = PARAMETER_OUTPUT;
                        kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    kData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portRangeHints.HintDescriptor))
                    kData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                kData->param.ranges[j].min = min;
                kData->param.ranges[j].max = max;
                kData->param.ranges[j].def = def;
                kData->param.ranges[j].step = step;
                kData->param.ranges[j].stepSmall = stepSmall;
                kData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                fParamBuffers[j] = def;

                fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                carla_stderr2("ERROR - Got a broken Port (neither Audio or Control)");

                fDescriptor->connect_port(fHandle, i, nullptr);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, nullptr);
            }
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

        // plugin checks
        fHints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (mIns == 1 && aIns == 0 && aOuts > 0)
            fHints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            fHints |= PLUGIN_CAN_FORCE_STEREO;

        // plugin options
        kData->availOptions &= ~(PLUGIN_OPTION_FIXED_BUFFER | PLUGIN_OPTION_SELF_AUTOMATION | PLUGIN_OPTION_SEND_ALL_SOUND_OFF | PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH | PLUGIN_OPTION_SEND_PITCHBEND);

        // always available if needed
        kData->availOptions |= PLUGIN_OPTION_SELF_AUTOMATION;

        // dssi-vst can do chunks, but needs fixed buffers
        if (QString(fFilename).endsWith("dssi-vst.so", Qt::CaseInsensitive))
        {
            fOptions |= PLUGIN_OPTION_FIXED_BUFFER;

            if (fOptions & PLUGIN_OPTION_USE_CHUNKS)
            {
                if (fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->set_custom_data != nullptr)
                    fHints |= PLUGIN_USES_CHUNKS;
            }
        }
        else
        {
            kData->availOptions |= PLUGIN_OPTION_FIXED_BUFFER;
        }

        // only for plugins with midi input
        if (mIns > 0)
        {
            kData->availOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            kData->availOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            kData->availOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
        }

        // check latency
        if (fHints & PLUGIN_CAN_DRYWET)
        {
            for (uint32_t i=0; i < kData->param.count; i++)
            {
                if (kData->param.data[i].type != PARAMETER_LATENCY)
                    continue;

                // we need to pre-run the plugin so it can update its latency control-port

                float tmpIn[aIns][2];
                float tmpOut[aOuts][2];

                for (j=0; j < aIns; j++)
                {
                    tmpIn[j][0] = 0.0f;
                    tmpIn[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, kData->audioIn.ports[j].rindex, tmpIn[j]);
                }

                for (j=0; j < aOuts; j++)
                {
                    tmpOut[j][0] = 0.0f;
                    tmpOut[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, kData->audioOut.ports[j].rindex, tmpOut[j]);
                }

                if (fDescriptor->activate != nullptr)
                    fDescriptor->activate(fHandle);

                fDescriptor->run(fHandle, 2);

                if (fDescriptor->deactivate != nullptr)
                    fDescriptor->deactivate(fHandle);

                const uint32_t latency = std::rint(fParamBuffers[i]);

                if (kData->latency != latency)
                {
                    kData->latency = latency;
                    kData->client->setLatency(latency);
                    recreateLatencyBuffers();
                }

                break;
            }
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        kData->client->activate();

        carla_debug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        carla_debug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = kData->midiprog.count;

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            while (fDssiDescriptor->get_program(fHandle, count))
                count++;
        }

        if (count > 0)
            kData->midiprog.createNew(count);

        // Update data
        for (i=0; i < kData->midiprog.count; i++)
        {
            const DSSI_Program_Descriptor* const pdesc = fDssiDescriptor->get_program(fHandle, i);
            CARLA_ASSERT(pdesc != nullptr);
            CARLA_ASSERT(pdesc->Name != nullptr);

            kData->midiprog.data[i].bank    = pdesc->Bank;
            kData->midiprog.data[i].program = pdesc->Program;
            kData->midiprog.data[i].name    = carla_strdup(pdesc->Name);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, kData->midiprog.count);

            for (i=0; i < kData->midiprog.count; i++)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (kData->midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);

            // Check if current program is invalid
            bool programChanged = false;

            if (kData->midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                kData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (kData->midiprog.current >= static_cast<int32_t>(kData->midiprog.count))
            {
                // current midi program > count
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (kData->midiprog.current < 0 && kData->midiprog.count > 0)
            {
                // programs exist now, but not before
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (kData->midiprog.current >= 0 && kData->midiprog.count == 0)
            {
                // programs existed before, but not anymore
                kData->midiprog.current = -1;
                programChanged = true;
            }

            if (programChanged)
                setMidiProgram(kData->midiprog.current, true, true, true, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; i++)
                carla_zeroFloat(outBuffer[i], frames);

            if (kData->activeBefore)
            {
                if (fDescriptor->deactivate != nullptr)
                {
                    fDescriptor->deactivate(fHandle);

                    if (fHandle2 != nullptr)
                        fDescriptor->deactivate(fHandle2);
                }
            }

            kData->activeBefore = kData->active;
            return;
        }

        unsigned long midiEventCount = 0;

        // --------------------------------------------------------------------------------------------------------
        // Check if not active before

        if (! kData->activeBefore)
        {
            if (kData->event.portIn != nullptr)
            {
                for (k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; k++)
                {
                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[k]);
                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[k+i]);

                    fMidiEvents[k].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[k].data.control.channel = k;
                    fMidiEvents[k].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;

                    fMidiEvents[k+i].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[k+i].data.control.channel = k;
                    fMidiEvents[k+i].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;
                }

                midiEventCount = MAX_MIDI_CHANNELS*2;
            }

            if (kData->latency > 0)
            {
                for (i=0; i < kData->audioIn.count; i++)
                    carla_zeroFloat(kData->latencyBuffers[i], kData->latency);
            }

            if (fDescriptor->activate != nullptr)
            {
                fDescriptor->activate(fHandle);

                if (fHandle2 != nullptr)
                    fDescriptor->activate(fHandle2);
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (kData->event.portIn != nullptr && kData->activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (midiEventCount < MAX_MIDI_EVENTS && ! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note = kData->extNotes.data.getLast(true); // FIXME, should be first

                    CARLA_ASSERT(note.channel >= 0);

                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                    fMidiEvents[midiEventCount].type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[midiEventCount].data.note.channel  = note.channel;
                    fMidiEvents[midiEventCount].data.note.note     = note.note;
                    fMidiEvents[midiEventCount].data.note.velocity = note.velo;

                    midiEventCount += 1;
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fHints & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t timeOffset = 0;

            uint32_t nextBankId = 0;
            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; i++)
            {
                const EngineEvent& event = kData->event.portIn->getEvent(i);

                time = event.time - framesOffset;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset, midiEventCount);
                    midiEventCount = 0;
                    nextBankId = 0;
                    timeOffset = time;
                }

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
                        // Control backend stuff
                        if (event.channel == kData->ctrlChannel)
                        {
                            double value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127/100;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
                            {
                                double left, right;
                                value = ctrlEvent.value/0.5 - 1.0;

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
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                continue;
                            }
                        }

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; k++)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            double value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                // FIXME - ranges call for this
                                value = ctrlEvent.value * (kData->param.ranges[k].max - kData->param.ranges[k].min) + kData->param.ranges[k].min;

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, k, 0, value);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < kData->midiprog.count; k++)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false, false);
                                    postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                                sendMidiAllNotesOff();

                            if (fDescriptor->deactivate != nullptr)
                            {
                                fDescriptor->deactivate(fHandle);

                                if (fHandle2 != nullptr)
                                    fDescriptor->deactivate(fHandle2);
                            }

                            if (fDescriptor->activate != nullptr)
                            {
                                fDescriptor->activate(fHandle);

                                if (fHandle2 != nullptr)
                                    fDescriptor->activate(fHandle2);
                            }

                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);

                            allNotesOffSent = true;
                        }

                        if (midiEventCount >= MAX_MIDI_EVENTS)
                            continue;

                        carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                        if (! sampleAccurate)
                            fMidiEvents[midiEventCount].time.tick = time;

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                        fMidiEvents[midiEventCount].data.control.channel = event.channel;
                        fMidiEvents[midiEventCount].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;

                        midiEventCount += 1;

                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                                sendMidiAllNotesOff();

                            allNotesOffSent = true;
                        }

                        if (midiEventCount >= MAX_MIDI_EVENTS)
                            continue;

                        carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                        if (! sampleAccurate)
                            fMidiEvents[midiEventCount].time.tick = time;

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                        fMidiEvents[midiEventCount].data.control.channel = event.channel;
                        fMidiEvents[midiEventCount].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                        midiEventCount += 1;

                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    if (midiEventCount >= MAX_MIDI_EVENTS)
                        continue;

                    const EngineMidiEvent& midiEvent = event.midi;

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status -= 0x10;

                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                    if (! sampleAccurate)
                        fMidiEvents[midiEventCount].time.tick = time - timeOffset;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        const uint8_t note = midiEvent.data[1];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_NOTEOFF;
                        fMidiEvents[midiEventCount].data.note.channel = channel;
                        fMidiEvents[midiEventCount].data.note.note    = note;

                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_NOTEON;
                        fMidiEvents[midiEventCount].data.note.channel  = channel;
                        fMidiEvents[midiEventCount].data.note.note     = note;
                        fMidiEvents[midiEventCount].data.note.velocity = velo;

                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        const uint8_t note     = midiEvent.data[1];
                        const uint8_t pressure = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_KEYPRESS;
                        fMidiEvents[midiEventCount].data.note.channel  = channel;
                        fMidiEvents[midiEventCount].data.note.note     = note;
                        fMidiEvents[midiEventCount].data.note.velocity = pressure;
                    }
                    else if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fHints & PLUGIN_OPTION_SELF_AUTOMATION) != 0)
                    {
                        const uint8_t control = midiEvent.data[1];
                        const uint8_t value   = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.param   = control;
                        fMidiEvents[midiEventCount].data.control.value   = value;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                    {
                        const uint8_t pressure = midiEvent.data[1];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CHANPRESS;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.value   = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        const uint8_t lsb = midiEvent.data[1];
                        const uint8_t msb = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_PITCHBEND;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.value   = ((msb << 7) | lsb) - 8192;
                    }
                    else
                        continue;

                    midiEventCount += 1;

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(inBuffer, outBuffer, frames - timeOffset, timeOffset, midiEventCount);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(inBuffer, outBuffer, frames, 0, midiEventCount);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        CARLA_PROCESS_CONTINUE_CHECK;

        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) > 0 && kData->postProc.dryWet != 1.0f;
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) > 0 && kData->postProc.volume != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) > 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; i++)
            {
                // Dry/Wet
                if (doDryWet)
                {
#if 0
                    for (k=0; k < frames; k++)
                    {
                        if (k < m_latency && m_latency < frames)
                            bufValue = (aIn.count == 1) ? m_latencyBuffers[0][k] : m_latencyBuffers[i][k];
                        else
                            bufValue = (aIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(bufValue*(1.0-x_dryWet));
                    }
#endif
                    for (k=0; k < frames; k++)
                    {
                        // TODO
                        //if (k < kData->latency && kData->latency < frames)
                        //    bufValue = (kData->audioIn.count == 1) ? kData->latencyBuffers[0][k] : kData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (kData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = inBuffer[ (kData->audioIn.count == 1) ? 0 : i ][k];

                        outBuffer[i][k] = (outBuffer[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        std::memcpy(oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; k++)
                    {
                        if (i % 2 == 0)
                        {
                            // left
                            outBuffer[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            outBuffer[i][k] += outBuffer[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k] * balRangeR;
                            outBuffer[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume
                if (doVolume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= kData->postProc.volume;
                }
            }

#if 0
            // Latency, save values for next callback, TODO
            if (kData->latency > 0 && kData->latency < frames)
            {
                for (i=0; i < kData->audioIn.count; i++)
                    std::memcpy(kData->latencyBuffers[i], inBuffer[i] + (frames - kData->latency), sizeof(float)*kData->latency);
            }
#endif
        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (kData->event.portOut != nullptr)
        {
            float value;

            for (k=0; k < kData->param.count; k++)
            {
                if (kData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                kData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (kData->param.data[k].midiCC > 0)
                {
                    value = kData->param.ranges[k].normalizeValue(fParamBuffers[k]);
                    kData->event.portOut->writeControlEvent(framesOffset, kData->param.data[k].midiChannel, kEngineControlEventTypeParameter, kData->param.data[k].midiCC, value);
                }
            }

        } // End of Control Output

        // --------------------------------------------------------------------------------------------------------

        kData->activeBefore = kData->active;
    }

    void processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset, const uint32_t midiEventCount)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
            std::memcpy(fAudioInBuffers[i], inBuffer[i]+timeOffset, sizeof(float)*frames);
        for (uint32_t i=0; i < kData->audioOut.count; i++)
            carla_zeroFloat(fAudioOutBuffers[i], frames);

        if (fDssiDescriptor->run_synth != nullptr)
        {
            fDssiDescriptor->run_synth(fHandle, frames, fMidiEvents, midiEventCount);

            if (fHandle2 != nullptr)
                fDssiDescriptor->run_synth(fHandle2, frames, fMidiEvents, midiEventCount);
        }
        else if (fDssiDescriptor->run_multiple_synths != nullptr)
        {
            unsigned long instances = (fHandle2 != nullptr) ? 2 : 1;
            LADSPA_Handle handlePtr[2] = { fHandle, fHandle2 };
            snd_seq_event_t* midiEventsPtr[2] = { fMidiEvents, fMidiEvents };
            unsigned long midiEventCountPtr[2] = { midiEventCount, midiEventCount };
            fDssiDescriptor->run_multiple_synths(instances, handlePtr, frames, midiEventsPtr, midiEventCountPtr);
        }
        else
        {
            fDescriptor->run(fHandle, frames);

            if (fHandle2 != nullptr)
                fDescriptor->run(fHandle2, frames);
        }

        for (uint32_t i=0, k; i < kData->audioOut.count; i++)
        {
            for (k=0; k < frames; k++)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
    }

    void bufferSizeChanged(const uint32_t newBufferSize)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; i++)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; i++)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < kData->audioOut.count; i++)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (kData->audioIn.count > 0)
            {
                CARLA_ASSERT(kData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (kData->audioOut.count > 0)
            {
                CARLA_ASSERT(kData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void uiParameterChange(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < kData->param.count);

        if (index >= kData->param.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_control(&kData->osc.data, kData->param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < kData->midiprog.count);

        if (index >= kData->midiprog.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_program(&kData->osc.data, kData->midiprog.data[index].bank, kData->midiprog.data[index].program);
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
        if (kData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;

        osc_send_midi(&kData->osc.data, midiData);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        carla_debug("DssiPlugin::deleteBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; i++)
            {
                if (fAudioInBuffers[i] != nullptr)
                {
                    delete[] fAudioInBuffers[i];
                    fAudioInBuffers[i] = nullptr;
                }
            }

            delete[] fAudioInBuffers;
            fAudioInBuffers = nullptr;
        }

        if (fAudioOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioOut.count; i++)
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

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::deleteBuffers();

        carla_debug("DssiPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const char* const guiFilename)
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

        const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)libSymbol("dssi_descriptor");

        if (descFn == nullptr)
        {
            kData->engine->setLastError("Could not find the DSSI Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((fDssiDescriptor = descFn(i++)) != nullptr)
        {
            fDescriptor = fDssiDescriptor->LADSPA_Plugin;
            if (fDescriptor != nullptr && fDescriptor->Label != nullptr && std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr || fDssiDescriptor == nullptr)
        {
            kData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = kData->engine->getNewUniquePluginName(name);
        else if (fDescriptor->Name != nullptr)
            fName = kData->engine->getNewUniquePluginName(fDescriptor->Name);
        else
            fName = kData->engine->getNewUniquePluginName(fDescriptor->Label);

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
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, kData->engine->getSampleRate());

        if (fHandle == nullptr)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (guiFilename != nullptr)
        {
            kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
            kData->osc.thread.setOscData(guiFilename, fDescriptor->Label);

            fHints |= PLUGIN_HAS_GUI;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor* fDescriptor;
    const DSSI_Descriptor*   fDssiDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;
    snd_seq_event_t fMidiEvents[MAX_MIDI_EVENTS];
    QByteArray      fChunk;
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_DSSI
# warning Building without DSSI support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newDSSI(const Initializer& init, const char* const guiFilename)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\"}, \"%s\")", init.engine, init.filename, init.name, init.label, guiFilename);

#ifdef WANT_DSSI
    DssiPlugin* const plugin = new DssiPlugin(init.engine, init.id);

    if (! plugin->init(init.filename, init.name, init.label, guiFilename))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && (plugin->hints() & PLUGIN_CAN_FORCE_STEREO) == 0)
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo DSSI plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    plugin->registerToOscClient();

    return plugin;
#else
    init.engine->setLastError("DSSI support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
