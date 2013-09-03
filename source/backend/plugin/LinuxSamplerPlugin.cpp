/*
 * Carla LinuxSampler Plugin
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPluginInternal.hpp"

#ifdef WANT_LINUXSAMPLER

// fix broken headers
#define old__cplusplus __cplusplus
#undef __cplusplus

#include "linuxsampler/EngineFactory.h"
#include <linuxsampler/Sampler.h>

#define __cplusplus old__cplusplus
#undef old__cplusplus

namespace LinuxSampler {

using CarlaBackend::CarlaEngine;
using CarlaBackend::CarlaPlugin;

// -----------------------------------------------------------------------
// LinuxSampler static values

static const float VOLUME_MAX = 3.16227766f; // +10 dB
static const float VOLUME_MIN = 0.0f;        // -inf dB

// -----------------------------------------------------------------------
// LinuxSampler AudioOutputDevice Plugin

class AudioOutputDevicePlugin : public AudioOutputDevice
{
public:
    AudioOutputDevicePlugin(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin)
        : AudioOutputDevice(std::map<String, DeviceCreationParameter*>()),
          fEngine(engine),
          fPlugin(plugin)
    {
        CARLA_ASSERT(engine != nullptr);
        CARLA_ASSERT(plugin != nullptr);
    }

    // -------------------------------------------------------------------
    // LinuxSampler virtual methods

    void Play() override
    {
    }

    bool IsPlaying() override
    {
        return (fEngine->isRunning() && fPlugin->isEnabled());
    }

    void Stop() override
    {
    }

    uint MaxSamplesPerCycle() override
    {
        return fEngine->getBufferSize();
    }

    uint SampleRate() override
    {
        return fEngine->getSampleRate();
    }

    String Driver() override
    {
        return "AudioOutputDevicePlugin";
    }

    AudioChannel* CreateChannel(uint channelNr) override
    {
        return new AudioChannel(channelNr, nullptr, 0);
    }

    // -------------------------------------------------------------------
    // Give public access to the RenderAudio call

    int Render(const uint samples)
    {
        return RenderAudio(samples);
    }

private:
    CarlaEngine* const fEngine;
    CarlaPlugin* const fPlugin;
};

// -----------------------------------------------------------------------
// LinuxSampler MidiInputDevice Plugin

class MidiInputDevicePlugin : public MidiInputDevice
{
public:
    MidiInputDevicePlugin(Sampler* const sampler)
        : MidiInputDevice(std::map<String, DeviceCreationParameter*>(), sampler)
    {
    }

    // -------------------------------------------------------------------
    // LinuxSampler virtual methods

    void Listen() override
    {
    }

    void StopListen() override
    {
    }

    String Driver() override
    {
        return "MidiInputDevicePlugin";
    }

    MidiInputPort* CreateMidiPort() override
    {
        return new MidiInputPortPlugin(this, Ports.size());
    }

    // -------------------------------------------------------------------
    // Properly delete port (destructor is protected)

    void DeleteMidiPort(MidiInputPort* const port)
    {
        delete (MidiInputPortPlugin*)port;
    }

    // -------------------------------------------------------------------
    // MIDI Port implementation for this plugin MIDI input driver
    // (Constructor and destructor are protected)

    class MidiInputPortPlugin : public MidiInputPort
    {
    protected:
        MidiInputPortPlugin(MidiInputDevicePlugin* const device, const int portNumber)
            : MidiInputPort(device, portNumber) {}
        friend class MidiInputDevicePlugin;
    };
};

} // namespace LinuxSampler

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

class LinuxSamplerPlugin : public CarlaPlugin
{
public:
    LinuxSamplerPlugin(CarlaEngine* const engine, const unsigned short id, const bool isGIG, const bool use16Outs)
        : CarlaPlugin(engine, id),
          kIsGIG(isGIG),
          kUses16Outs(use16Outs),
          fSampler(new LinuxSampler::Sampler()),
          fSamplerChannel(nullptr),
          fEngine(nullptr),
          fEngineChannel(nullptr),
          fAudioOutputDevice(new LinuxSampler::AudioOutputDevicePlugin(engine, this)),
          fMidiInputDevice(new LinuxSampler::MidiInputDevicePlugin(fSampler)),
          fMidiInputPort(fMidiInputDevice->CreateMidiPort()),
          fInstrument(nullptr)
    {
        carla_debug("LinuxSamplerPlugin::LinuxSamplerPlugin(%p, %i, %s)", engine, id, bool2str(isGIG));
    }

    ~LinuxSamplerPlugin() override
    {
        carla_debug("LinuxSamplerPlugin::~LinuxSamplerPlugin()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fEngine != nullptr)
        {
            if (fSamplerChannel != nullptr)
            {
                fMidiInputPort->Disconnect(fSamplerChannel->GetEngineChannel());
                fEngineChannel->DisconnectAudioOutputDevice();
                fSampler->RemoveSamplerChannel(fSamplerChannel);
            }

            LinuxSampler::EngineFactory::Destroy(fEngine);
        }

        // destructor is private
        fMidiInputDevice->DeleteMidiPort(fMidiInputPort);

        delete fMidiInputDevice;
        delete fAudioOutputDevice;
        delete fSampler;

        fInstrumentIds.clear();

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return kIsGIG ? PLUGIN_GIG : PLUGIN_SFZ;
    }

    PluginCategory getCategory() const override
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getAvailableOptions() const override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return options;
    }

    void getLabel(char* const strBuf) const override
    {
        std::strncpy(strBuf, (const char*)fLabel, STR_MAX);
    }

    void getMaker(char* const strBuf) const override
    {
        std::strncpy(strBuf, (const char*)fMaker, STR_MAX);
    }

    void getCopyright(char* const strBuf) const override
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf) const override
    {
        std::strncpy(strBuf, (const char*)fRealName, STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(pData->midiprog.count))
            return;

        if (pData->ctrlChannel < 0 || pData->ctrlChannel >= 16)
            return;

        if (index >= 0)
        {
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;
            const uint32_t rIndex  = bank*128 + program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            if (pData->engine->isOffline())
            {
                fEngineChannel->PrepareLoadInstrument((const char*)fFilename, rIndex);
                fEngineChannel->LoadInstrument();
            }
            else
            {
                fInstrument->LoadInstrumentInBackground(fInstrumentIds[rIndex], fEngineChannel);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("LinuxSamplerPlugin::reload() - start");
        CARLA_ASSERT(pData->engine != nullptr);
        CARLA_ASSERT(fInstrument != nullptr);

        if (pData->engine == nullptr)
            return;
        if (fInstrument == nullptr)
            return;

        const ProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t aOuts;
        aOuts = 2;

        pData->audioOut.createNew(aOuts);

        const int   portNameSize = pData->engine->getMaxPortNameSize();
        CarlaString portName;

        // ---------------------------------------
        // Audio Outputs

        {
            // out-left
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "out-left";
            portName.truncate(portNameSize);

            pData->audioOut.ports[0].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[0].rindex = 0;

            // out-right
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "out-right";
            portName.truncate(portNameSize);

            pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[1].rindex = 1;
        }

        // ---------------------------------------
        // Event Input

        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        // ---------------------------------------

        // plugin hints
        fHints  = 0x0;
        //fHints |= PLUGIN_IS_SYNTH;
        fHints |= PLUGIN_CAN_VOLUME;
        fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints  = 0x0;
        pData->extraHints |= PLUGIN_HINT_HAS_MIDI_IN;
        pData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("LinuxSamplerPlugin::reload() - end");
    }

    void reloadPrograms(bool init) override
    {
        carla_debug("LinuxSamplerPlugin::reloadPrograms(%s)", bool2str(init));

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t i, count = fInstrumentIds.size();

        // sound kits must always have at least 1 midi-program
        CARLA_ASSERT(count > 0);

        if (count == 0)
            return;

        pData->midiprog.createNew(count);

        LinuxSampler::InstrumentManager::instrument_info_t info;

        for (i=0; i < pData->midiprog.count; ++i)
        {
            pData->midiprog.data[i].bank    = i / 128;
            pData->midiprog.data[i].program = i % 128;

            try {
                info = fInstrument->GetInstrumentInfo(fInstrumentIds[i]);
            }
            catch (const LinuxSampler::InstrumentManagerException&)
            {
                continue;
            }

            pData->midiprog.data[i].name = carla_strdup(info.InstrumentName.c_str());
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (pData->engine->isOscControlRegistered())
        {
            pData->engine->oscSend_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                pData->engine->oscSend_control_set_midi_program_data(fId, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            setMidiProgram(0, false, false, false);
        }
        else
        {
            pData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        CARLA_ASSERT(fAudioOutputDevice != nullptr);

        fAudioOutputDevice->Play();
    }

    void deactivate() override
    {
        CARLA_ASSERT(fAudioOutputDevice != nullptr);

        fAudioOutputDevice->Stop();
    }

    void process(float** const, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; ++k)
                {
                    fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, k);
                    fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, k);
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (k=0; k < MAX_MIDI_NOTE; ++k)
                    fMidiInputPort->DispatchNoteOff(k, 0, pData->ctrlChannel);
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                while (! pData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(pData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    if (note.velo > 0)
                        fMidiInputPort->DispatchNoteOn(note.note, note.velo, note.channel, 0);
                    else
                        fMidiInputPort->DispatchNoteOff(note.note, note.velo, note.channel, 0);
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t time, nEvents = pData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId = 0;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    if (processSingle(outBuffer, time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = time;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;
                    }
                    else
                        startTime += timeOffset;
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
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
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
                            }
                        }
#endif

                        // Control plugin parameters
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

                            double value;

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
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            fMidiInputPort->DispatchControlChange(ctrlEvent.param, ctrlEvent.value*127.0f, event.channel, sampleAccurate ? startTime : time);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, event.channel, sampleAccurate ? startTime : time);
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, event.channel, sampleAccurate ? startTime : time);
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status -= 0x10;

                    int32_t fragmentPos = sampleAccurate ? startTime : time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        const uint8_t note = midiEvent.data[1];

                        fMidiInputPort->DispatchNoteOff(note, 0, channel, fragmentPos);

                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fMidiInputPort->DispatchNoteOn(note, velo, channel, fragmentPos);

                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) != 0)
                    {
                        //const uint8_t note     = midiEvent.data[1];
                        //const uint8_t pressure = midiEvent.data[2];

                        // unsupported
                    }
                    else if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0)
                    {
                        const uint8_t control = midiEvent.data[1];
                        const uint8_t value   = midiEvent.data[2];

                        fMidiInputPort->DispatchControlChange(control, value, channel, fragmentPos);
                    }
                    else if (MIDI_IS_STATUS_CHANNEL_PRESSURE(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) != 0)
                    {
                        //const uint8_t pressure = midiEvent.data[1];

                        // unsupported
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) != 0)
                    {
                        const uint8_t lsb = midiEvent.data[1];
                        const uint8_t msb = midiEvent.data[2];

                        fMidiInputPort->DispatchPitchbend(((msb << 7) | lsb) - 8192, channel, fragmentPos);
                    }

                    break;
                }
                }
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(outBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing
    }

    bool processSingle(float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_ASSERT(outBuffer != nullptr);
        CARLA_ASSERT(frames > 0);

        if (outBuffer == nullptr)
            return false;
        if (frames == 0)
            return false;

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (i=0; i < pData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fAudioOutputDevice->Channel(0)->SetBuffer(outBuffer[0] + timeOffset);
        fAudioOutputDevice->Channel(1)->SetBuffer(outBuffer[1] + timeOffset);
        // QUESTION: Need to clear it before?
        fAudioOutputDevice->Render(frames);

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) > 0 && pData->postProc.volume != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) > 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            float oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < pData->audioOut.count; ++i)
            {
                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        carla_copyFloat(oldBufLeft, outBuffer[i], frames);

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------

    const void* getExtraStuff() const noexcept override
    {
        return kUses16Outs ? (const void*)0x1 : nullptr;
    }

    bool init(const char* filename, const char* const name, const char* label)
    {
        CARLA_ASSERT(pData->engine != nullptr);
        CARLA_ASSERT(pData->client == nullptr);
        CARLA_ASSERT(filename != nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (pData->engine == nullptr)
        {
            return false;
        }

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr)
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr)
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // Check if file exists
        {
//             QFileInfo file(filename);
//
//             if (! (file.exists() && file.isFile() && file.isReadable()))
//             {
//                 pData->engine->setLastError("Requested file is not valid or does not exist");
//                 return false;
//             }
        }

        // ---------------------------------------------------------------
        // Create the LinuxSampler Engine

        const char* const stype = kIsGIG ? "gig" : "sfz";

        try {
            fEngine = LinuxSampler::EngineFactory::Create(stype);
        }
        catch (LinuxSampler::Exception& e)
        {
            pData->engine->setLastError(e.what());
            return false;
        }

        // ---------------------------------------------------------------
        // Get the Engine's Instrument Manager

        fInstrument = fEngine->GetInstrumentManager();

        if (fInstrument == nullptr)
        {
            pData->engine->setLastError("Failed to get LinuxSampler instrument manager");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            fEngine = nullptr;
            return false;
        }

        // ---------------------------------------------------------------
        // Load the Instrument via filename

        try {
            fInstrumentIds = fInstrument->GetInstrumentFileContent(filename);
        }
        catch (const LinuxSampler::InstrumentManagerException& e)
        {
            pData->engine->setLastError(e.what());
            LinuxSampler::EngineFactory::Destroy(fEngine);
            fEngine = nullptr;
            return false;
        }

        // ---------------------------------------------------------------
        // Get info

        if (fInstrumentIds.size() == 0)
        {
            pData->engine->setLastError("Failed to find any instruments");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            fEngine = nullptr;
            return false;
        }

        LinuxSampler::InstrumentManager::instrument_info_t info;

        try {
            info = fInstrument->GetInstrumentInfo(fInstrumentIds[0]);
        }
        catch (const LinuxSampler::InstrumentManagerException& e)
        {
            pData->engine->setLastError(e.what());
            LinuxSampler::EngineFactory::Destroy(fEngine);
            fEngine = nullptr;
            return false;
        }

        fRealName = info.InstrumentName.c_str();
        fLabel    = info.Product.c_str();
        fMaker    = info.Artists.c_str();
        fFilename = filename;

        if (kUses16Outs && ! fLabel.endsWith(" (16 outs)"))
            fLabel += " (16 outs)";

        if (name != nullptr)
            fName = pData->engine->getUniquePluginName(name);
        else
            fName = pData->engine->getUniquePluginName((const char*)fRealName);

        // ---------------------------------------------------------------
        // Register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            fEngine = nullptr;
            return false;
        }

        // ---------------------------------------------------------------
        // Init LinuxSampler stuff

        fSamplerChannel = fSampler->AddSamplerChannel();
        fSamplerChannel->SetEngineType(stype);
        fSamplerChannel->SetAudioOutputDevice(fAudioOutputDevice);

        fEngineChannel = fSamplerChannel->GetEngineChannel();
        fEngineChannel->Connect(fAudioOutputDevice);
        fEngineChannel->Volume(LinuxSampler::VOLUME_MAX);

        fMidiInputPort->Connect(fSamplerChannel->GetEngineChannel(), LinuxSampler::midi_chan_all);

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
            fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
            fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

            // load settings
            pData->idStr  = kIsGIG ? "GIG" : "SFZ";
            pData->idStr += "/";
            pData->idStr += label;
            fOptions = pData->loadSettings(fOptions, getAvailableOptions());
        }

        return true;
    }

    // -------------------------------------------------------------------

    static CarlaPlugin* newLinuxSampler(const Initializer& init, bool isGIG, const bool use16Outs);

private:
    const bool kIsGIG; // sfz if false
    const bool kUses16Outs;

    CarlaString fRealName;
    CarlaString fLabel;
    CarlaString fMaker;

    LinuxSampler::Sampler*        fSampler;
    LinuxSampler::SamplerChannel* fSamplerChannel;

    LinuxSampler::Engine*        fEngine;
    LinuxSampler::EngineChannel* fEngineChannel;

    LinuxSampler::AudioOutputDevicePlugin* fAudioOutputDevice;
    LinuxSampler::MidiInputDevicePlugin* fMidiInputDevice;
    LinuxSampler::MidiInputPort* fMidiInputPort;

    LinuxSampler::InstrumentManager* fInstrument;
    std::vector<LinuxSampler::InstrumentManager::instrument_id_t> fInstrumentIds;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinuxSamplerPlugin)
};

CarlaPlugin* LinuxSamplerPlugin::newLinuxSampler(const Initializer& init, const bool isGIG, const bool use16Outs)
{
    carla_debug("LinuxSamplerPlugin::newLinuxSampler({%p, \"%s\", \"%s\", \"%s\"}, %s, %s)", init.engine, init.filename, init.name, init.label, bool2str(isGIG), bool2str(use16Outs));

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && use16Outs)
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo modules, please choose the 2-channel only sample-library version");
        return nullptr;
    }

    LinuxSamplerPlugin* const plugin(new LinuxSamplerPlugin(init.engine, init.id, isGIG, use16Outs));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    return plugin;
}

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LINUXSAMPLER
# warning linuxsampler not available (no GIG and SFZ support)
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newGIG(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newGIG({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));
#ifdef WANT_LINUXSAMPLER
    return LinuxSamplerPlugin::newLinuxSampler(init, true, use16Outs);
#else
    init.engine->setLastError("linuxsampler support not available");
    return nullptr;
#endif
}

CarlaPlugin* CarlaPlugin::newSFZ(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newSFZ({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));
#ifdef WANT_LINUXSAMPLER
    return LinuxSamplerPlugin::newLinuxSampler(init, false, use16Outs);
#else
    init.engine->setLastError("linuxsampler support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
