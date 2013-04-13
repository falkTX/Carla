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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaPluginInternal.hpp"

#ifdef WANT_LINUXSAMPLER

#include "linuxsampler/EngineFactory.h"
#include <linuxsampler/Sampler.h>

#include <QtCore/QFileInfo>

// TODO - setMidiProgram()

namespace LinuxSampler {

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
          kEngine(engine),
          kPlugin(plugin)
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
        return (kEngine->isRunning() && kPlugin->enabled());
    }

    void Stop() override
    {
    }

    uint MaxSamplesPerCycle() override
    {
        return kEngine->getBufferSize();
    }

    uint SampleRate() override
    {
        return kEngine->getSampleRate();
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
    CarlaBackend::CarlaEngine* const kEngine;
    CarlaBackend::CarlaPlugin* const kPlugin;
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
          fSampler(nullptr),
          fSamplerChannel(nullptr),
          fEngine(nullptr),
          fEngineChannel(nullptr),
          fAudioOutputDevice(nullptr),
          fMidiInputDevice(nullptr),
          fMidiInputPort(nullptr),
          fInstrument(nullptr)
    {
        carla_debug("LinuxSamplerPlugin::LinuxSamplerPlugin(%p, %i, %s)", engine, id, bool2str(isGIG));

        fSampler = new LinuxSampler::Sampler;

        fAudioOutputDevice = new LinuxSampler::AudioOutputDevicePlugin(engine, this);
        fMidiInputDevice   = new LinuxSampler::MidiInputDevicePlugin(fSampler);
        fMidiInputPort     = fMidiInputDevice->CreateMidiPort();
    }

    ~LinuxSamplerPlugin() override
    {
        carla_debug("LinuxSamplerPlugin::~LinuxSamplerPlugin()");

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->active/*Before*/)
            fAudioOutputDevice->Stop();

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
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const override
    {
        return kIsGIG ? PLUGIN_GIG : PLUGIN_SFZ;
    }

    PluginCategory category() override
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions() override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return options;
    }

    void getLabel(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fLabel, STR_MAX);
    }

    void getMaker(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fMaker, STR_MAX);
    }

    void getCopyright(char* const strBuf) override
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fRealName, STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        if (kData->ctrlChannel < 0 || kData->ctrlChannel >= 16)
            return;

        if (index >= 0)
        {
            // TODO
            //const uint32_t bank    = kData->midiprog.data[index].bank;
            //const uint32_t program = kData->midiprog.data[index].program;

            //const ScopedProcessLocker spl(this);
            //fluid_synth_program_select(fSynth, kData->ctrlChannel, fSynthId, bank, program);
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("LinuxSamplerPlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fInstrument != nullptr);

        if (kData->engine == nullptr)
            return;
        if (fInstrument == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->active)
            deactivate();

        clearBuffers();

        uint32_t aOuts;
        aOuts = 2;

        kData->audioOut.createNew(aOuts);

        const int   portNameSize = kData->engine->maxPortNameSize();
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

            kData->audioOut.ports[0].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[0].rindex = 0;

            // out-right
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "out-right";
            portName.truncate(portNameSize);

            kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[1].rindex = 1;
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

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        // ---------------------------------------

        // plugin hints
        fHints  = 0x0;
        fHints |= PLUGIN_IS_RTSAFE;
        fHints |= PLUGIN_IS_SYNTH;
        fHints |= PLUGIN_CAN_VOLUME;
        fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        kData->extraHints  = 0x0;
        kData->extraHints |= PLUGIN_HINT_HAS_MIDI_IN;
        kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        if (kData->active)
            activate();

        carla_debug("LinuxSamplerPlugin::reload() - end");
    }

    void reloadPrograms(bool init) override
    {
        carla_debug("LinuxSamplerPlugin::reloadPrograms(%s)", bool2str(init));

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t i, count = 0; fInstrumentIds.size();

        // sound kits must always have at least 1 midi-program
        CARLA_SAFE_ASSERT(count > 0); // FIXME

        if (count == 0)
            return;

        kData->midiprog.createNew(count);

        // Update data
        for (i=0; i < kData->midiprog.count; ++i)
        {
            LinuxSampler::InstrumentManager::instrument_info_t info = fInstrument->GetInstrumentInfo(fInstrumentIds[i]);

            kData->midiprog.data[i].bank    = i / 128;
            kData->midiprog.data[i].program = i % 128;
            kData->midiprog.data[i].name    = carla_strdup(info.InstrumentName.c_str());
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            setMidiProgram(0, false, false, false);
        }
        else
        {
            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
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

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            // TODO
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; ++k)
                {
                    fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, k, 0);
                    fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, k, 0);
                }
            }
            else
            {
            }

            kData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    if (note.velo > 0)
                        fMidiInputPort->DispatchNoteOn(note.note, note.velo, note.channel, 0);
                    else
                        fMidiInputPort->DispatchNoteOff(note.note, note.velo, note.channel, 0);
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

            uint32_t nextBankId = 0;
            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event = kData->event.portIn->getEvent(i);

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

                        if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                            nextBankId = kData->midiprog.data[kData->midiprog.current].bank;
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
                        for (k=0; k < kData->param.count; ++k)
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
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[i].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < kData->midiprog.count; ++k)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                            {
                                sendMidiAllNotesOff();
                                allNotesOffSent = true;
                            }

                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 0.0f);
                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 1.0f);
                        }

                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                            fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, k, sampleAccurate ? startTime : time);

                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                            {
                                sendMidiAllNotesOff();
                                allNotesOffSent = true;
                            }
                        }

                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                            fMidiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, k, sampleAccurate ? startTime : time);

                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent = event.midi;

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

                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fMidiInputPort->DispatchNoteOn(note, velo, channel, fragmentPos);

                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
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
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) != 0)
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

            kData->postRtEvents.trySplice();

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

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
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

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) > 0 && kData->postProc.volume != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) > 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            float oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        carla_copyFloat(oldBufLeft, outBuffer[i], frames);

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

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
                        outBuffer[i][k+timeOffset] *= kData->postProc.volume;
                }
            }

        } // End of Post-processing

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    // -------------------------------------------------------------------

    bool init(const char* filename, const char* const name, const char* label)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(filename != nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr)
        {
            kData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr)
        {
            kData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // Check if file exists
        {
            QFileInfo file(filename);

            if (! (file.exists() && file.isFile() && file.isReadable()))
            {
                kData->engine->setLastError("Requested file is not valid or does not exist");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // Create the LinuxSampler Engine

        const char* const stype = kIsGIG ? "gig" : "sfz";

        try {
            fEngine = LinuxSampler::EngineFactory::Create(stype);
        }
        catch (LinuxSampler::Exception& e)
        {
            kData->engine->setLastError(e.what());
            return false;
        }

        // ---------------------------------------------------------------
        // Get the Engine's Instrument Manager

        fInstrument = fEngine->GetInstrumentManager();

        if (fInstrument == nullptr)
        {
            kData->engine->setLastError("Failed to get LinuxSampler instrument manager");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            return false;
        }

        // ---------------------------------------------------------------
        // Load the Instrument via filename

        try {
            fInstrumentIds = fInstrument->GetInstrumentFileContent(filename);
        }
        catch (const LinuxSampler::InstrumentManagerException& e)
        {
            kData->engine->setLastError(e.what());
            LinuxSampler::EngineFactory::Destroy(fEngine);
            return false;
        }

        // ---------------------------------------------------------------
        // Get info

        if (fInstrumentIds.size() == 0)
        {
            kData->engine->setLastError("Failed to find any instruments");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            return false;
        }

        LinuxSampler::InstrumentManager::instrument_info_t info;

        try {
            info = fInstrument->GetInstrumentInfo(fInstrumentIds[0]);
        }
        catch (const LinuxSampler::InstrumentManagerException& e)
        {
            kData->engine->setLastError(e.what());
            LinuxSampler::EngineFactory::Destroy(fEngine);
            return false;
        }

        fRealName = info.InstrumentName.c_str();
        fLabel    = info.Product.c_str();
        fMaker    = info.Artists.c_str();
        fFilename = filename;

        if (name != nullptr)
            fName = kData->engine->getNewUniquePluginName(name);
        else
            fName = kData->engine->getNewUniquePluginName((const char*)fRealName);

        // ---------------------------------------------------------------
        // Register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            LinuxSampler::EngineFactory::Destroy(fEngine);
            return false;
        }

        // ---------------------------------------------------------------
        // Init LinuxSampler stuff

        fSamplerChannel = fSampler->AddSamplerChannel();
        fSamplerChannel->SetEngineType(stype);
        fSamplerChannel->SetAudioOutputDevice(fAudioOutputDevice);

        fEngineChannel = fSamplerChannel->GetEngineChannel();
        fEngineChannel->Connect(fAudioOutputDevice);
        fEngineChannel->PrepareLoadInstrument(filename, 0); // todo - find instrument from label
        fEngineChannel->LoadInstrument();
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
            kData->idStr  = kIsGIG ? "GIG" : "SFZ";
            kData->idStr += "/";
            kData->idStr += label;
            fOptions = kData->loadSettings(fOptions, availableOptions());
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
