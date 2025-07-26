// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifndef STATIC_PLUGIN_TARGET
# define HAVE_SFZ
#endif

#ifdef HAVE_SFZ

#include "CarlaBackendUtils.hpp"

#include "sfzero/SFZero.h"

#include "water/buffers/AudioSampleBuffer.h"
#include "water/files/File.h"
#include "water/midi/MidiMessage.h"

using water::AudioSampleBuffer;
using water::File;
using water::MidiMessage;

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };


static void loadingIdleCallbackFunction(void* ptr)
{
    ((CarlaEngine*)ptr)->callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
}

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginSFZero : public CarlaPlugin
{
public:
    CarlaPluginSFZero(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fSynth(),
          fNumVoices(0.0f),
          fLabel(nullptr),
          fRealName(nullptr)
    {
        carla_debug("CarlaPluginSFZero::CarlaPluginSFZero(%p, %i)", engine, id);
    }

    ~CarlaPluginSFZero() override
    {
        carla_debug("CarlaPluginSFZero::~CarlaPluginSFZero()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fLabel != nullptr)
        {
            delete[] fLabel;
            fLabel = nullptr;
        }

        if (fRealName != nullptr)
        {
            delete[] fRealName;
            fRealName = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_SFZ;
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

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId == 0, 0.0f);

        return fNumVoices;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        if (fLabel != nullptr)
        {
            std::strncpy(strBuf, fLabel, STR_MAX);
            return true;
        }

        return CarlaPlugin::getLabel(strBuf);
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "SFZero engine", STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "ISC", STR_MAX);
        return true;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        if (fRealName != nullptr)
        {
            std::strncpy(strBuf, fRealName, STR_MAX);
            return true;
        }

        return CarlaPlugin::getRealName(strBuf);
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId == 0, false);

        std::strncpy(strBuf, "Voice Count", STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set ui stuff

    // nothing

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_debug("CarlaPluginSFZero::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        pData->audioOut.createNew(2);
        pData->param.createNew(1, false);

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        String portName;

        // ---------------------------------------
        // Audio Outputs

        // out-left
        portName.clear();

        if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            portName  = pData->name;
            portName += ":";
        }

        portName += "out-left";
        portName.truncate(portNameSize);

        pData->audioOut.ports[0].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 0);
        pData->audioOut.ports[0].rindex = 0;

        // out-right
        portName.clear();

        if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            portName  = pData->name;
            portName += ":";
        }

        portName += "out-right";
        portName.truncate(portNameSize);

        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
        pData->audioOut.ports[1].rindex = 1;

        // ---------------------------------------
        // Event Input

        portName.clear();

        if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            portName  = pData->name;
            portName += ":";
        }

        portName += "events-in";
        portName.truncate(portNameSize);

        pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);

        // ---------------------------------------
        // Parameters

        pData->param.data[0].type   = PARAMETER_OUTPUT;
        pData->param.data[0].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMATABLE | PARAMETER_IS_INTEGER;
        pData->param.data[0].index  = 0;
        pData->param.data[0].rindex = 0;
        pData->param.ranges[0].min = 0.0f;
        pData->param.ranges[0].max = 128;
        pData->param.ranges[0].def = 0.0f;
        pData->param.ranges[0].step = 1.0f;
        pData->param.ranges[0].stepSmall = 1.0f;
        pData->param.ranges[0].stepLarge = 1.0f;

        // ---------------------------------------

        // plugin hints
        pData->hints  = 0x0;
        pData->hints |= PLUGIN_IS_SYNTH;
        pData->hints |= PLUGIN_CAN_VOLUME;
        pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints  = 0x0;
        pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginSFZero::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(const float* const* const, float** const audioOut, const float* const*, float**, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);

            fNumVoices = 0.0f;
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            fSynth.allNotesOff(0, false);
            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        {
            // ----------------------------------------------------------------------------------------------------
            // Setup audio buffer

            AudioSampleBuffer audioOutBuffer(audioOut, 2, frames);

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    if (note.velo > 0)
                        fSynth.noteOn(note.channel+1, note.note, static_cast<float>(note.velo)/127.0f);
                    else
                        fSynth.noteOff(note.channel+1, note.note, static_cast<float>(note.velo)/127.0f, true);
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif
            uint32_t timeOffset = 0;

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }
                else if (eventTime > timeOffset)
                {
                    if (processSingle(audioOutBuffer, eventTime - timeOffset, timeOffset))
                        timeOffset = eventTime;
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
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
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

                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].hints != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, eventTime, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            fSynth.handleController(event.channel+1, ctrlEvent.param, int(ctrlEvent.normalizedValue*127.0f + 0.5f));
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                    case kEngineControlEventTypeMidiProgram:
                    case kEngineControlEventTypeAllSoundOff:
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

                            fSynth.allNotesOff(event.channel+1, true);
                        }
                        break;
                    }
                    break;
                }

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > EngineMidiEvent::kDataSize)
                        continue;

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

                    // put back channel in data
                    uint8_t midiData2[EngineMidiEvent::kDataSize];
                    midiData2[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    std::memcpy(midiData2 + 1, midiEvent.data + 1, static_cast<std::size_t>(midiEvent.size - 1));

                    const MidiMessage midiMessage(midiData2, static_cast<int>(midiEvent.size), 0.0);

                    fSynth.handleMidiEvent(midiMessage);

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiEvent.data[1]);
                    }
                } break;
                }
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioOutBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Parameter outputs

        fNumVoices = static_cast<float>(fSynth.numVoicesUsed());
    }

    bool processSingle(AudioSampleBuffer& audioOutBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

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
            audioOutBuffer.clear(timeOffset, frames);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fSynth.renderVoices(audioOutBuffer, static_cast<int>(timeOffset), static_cast<int>(frames));

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = carla_isNotEqual(pData->postProc.volume, 1.0f);
            //const bool doBalance = carla_isNotEqual(pData->postProc.balanceLeft, -1.0f) || carla_isNotEqual(pData->postProc.balanceRight, 1.0f);

            float* outBufferL = audioOutBuffer.getWritePointer(0, timeOffset);
            float* outBufferR = audioOutBuffer.getWritePointer(1, timeOffset);

#if 0
            if (doBalance)
            {
                float* const oldBufLeft = pData->postProc.extraBuffer;

                // there was a loop here
                {
                    if (i % 2 == 0)
                        carla_copyFloats(oldBufLeft, outBuffer[i], frames);

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
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
            }
#endif

            if (doVolume)
            {
                const float volume = pData->postProc.volume;

                for (uint32_t k=0; k < frames; ++k)
                {
                    *outBufferL++ *= volume;
                    *outBufferR++ *= volume;
                }
            }

        } // End of Post-processing
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        fSynth.setCurrentPlaybackSampleRate(newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------

    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const label, const uint options)
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

        for (int i = 128; --i >=0;)
            fSynth.addVoice(new sfzero::Voice());

        // ---------------------------------------------------------------
        // Init SFZero stuff

        fSynth.setCurrentPlaybackSampleRate(pData->engine->getSampleRate());

        File file(filename);
        sfzero::Sound* const sound = new sfzero::Sound(file);

        sfzero::Sound::LoadingIdleCallback cb = {
            loadingIdleCallbackFunction,
            pData->engine,
        };

        sound->loadRegions();
        sound->loadSamples(cb);

        if (fSynth.addSound(sound) == nullptr)
        {
            pData->engine->setLastError("Failed to allocate SFZ sounds in memory");
            return false;
        }

        sound->dumpToConsole();

        // ---------------------------------------------------------------

        const water::String basename(File(filename).getFileNameWithoutExtension());

        String label2(label != nullptr ? label : basename.toRawUTF8());

        fLabel    = carla_strdup(label2);
        fRealName = carla_strdup(basename.toRawUTF8());

        pData->filename = carla_strdup(filename);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fRealName[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fRealName);
        else
            pData->name = pData->engine->getUniquePluginName(fLabel);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set options

        pData->options = 0x0;

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
        if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
            pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

        return true;
    }

    // -------------------------------------------------------------------

private:
    sfzero::Synth fSynth;
    float fNumVoices;

    const char* fLabel;
    const char* fRealName;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginSFZero)
};

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_SFZ

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newSFZero(const Initializer& init)
{
    carla_debug("CarlaPluginSFZero::newSFZero({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.label, init.uniqueId);

#ifdef HAVE_SFZ
    // -------------------------------------------------------------------
    // Check if file exists

    if (! water::File(init.filename).existsAsFile())
    {
        init.engine->setLastError("Requested file is not valid or does not exist");
        return nullptr;
    }

    std::shared_ptr<CarlaPluginSFZero> plugin(new CarlaPluginSFZero(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
#else
    init.engine->setLastError("SFZ support not available");
    return nullptr;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
