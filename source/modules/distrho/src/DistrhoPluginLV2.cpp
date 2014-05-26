/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginInternal.hpp"

#include "lv2/atom.h"
#include "lv2/atom-util.h"
#include "lv2/buf-size.h"
#include "lv2/data-access.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/urid.h"
#include "lv2/worker.h"
#include "lv2/lv2_programs.h"

#ifdef noexcept
# undef noexcept
#endif

#include <map>
#include <string>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#if DISTRHO_PLUGIN_WANT_STATE
# warning LV2 State still TODO (working but needs final testing)
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
# warning LV2 TimePos still TODO
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_TIMEPOS || (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI))
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)

typedef std::map<d_string,d_string> StringMap;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class PluginLv2
{
public:
    PluginLv2(const LV2_URID_Map* const uridMap, const LV2_Worker_Schedule* const worker)
        : fPortControls(nullptr),
          fLastControlValues(nullptr),
#if DISTRHO_LV2_USE_EVENTS_IN || DISTRHO_LV2_USE_EVENTS_OUT
# if DISTRHO_PLUGIN_WANT_TIMEPOS
          fLastTimeSpeed(0.0f),
# endif
          fURIDs(uridMap),
#endif
          fUridMap(uridMap),
          fWorker(worker)
    {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            fPortAudioIns[i] = nullptr;
#else
        fPortAudioIns = nullptr;
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            fPortAudioOuts[i] = nullptr;
#else
        fPortAudioOuts = nullptr;
#endif

        if (const uint32_t count = fPlugin.getParameterCount())
        {
            fPortControls      = new float*[count];
            fLastControlValues = new float[count];

            for (uint32_t i=0; i < count; ++i)
            {
                fPortControls[i] = nullptr;
                fLastControlValues[i] = fPlugin.getParameterValue(i);
            }
        }
        else
        {
            fPortControls      = nullptr;
            fLastControlValues = nullptr;
        }

#if DISTRHO_LV2_USE_EVENTS_IN
        fPortEventsIn = nullptr;
#endif
#if DISTRHO_LV2_USE_EVENTS_OUT
        fPortEventsOut = nullptr;
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
        fPortLatency = nullptr;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (const uint32_t count = fPlugin.getStateCount())
        {
            fNeededUiSends = new bool[count];

            for (uint32_t i=0; i < count; ++i)
                fNeededUiSends[i] = false;
        }
        else
        {
            fNeededUiSends = nullptr;
        }
#endif
    }

    ~PluginLv2()
    {
        if (fPortControls != nullptr)
        {
            delete[] fPortControls;
            fPortControls = nullptr;
        }

        if (fLastControlValues)
        {
            delete[] fLastControlValues;
            fLastControlValues = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_STATE
        if (fNeededUiSends != nullptr)
        {
            delete[] fNeededUiSends;
            fNeededUiSends = nullptr;
        }

        fStateMap.clear();
#endif
    }

    // -------------------------------------------------------------------

    void lv2_activate()
    {
        fPlugin.activate();
    }

    void lv2_deactivate()
    {
        fPlugin.deactivate();
    }

    // -------------------------------------------------------------------

    void lv2_connect_port(const uint32_t port, void* const dataLocation)
    {
        uint32_t index = 0;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioIns[i] = (const float*)dataLocation;
                return;
            }
        }
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioOuts[i] = (float*)dataLocation;
                return;
            }
        }
#endif

#if DISTRHO_LV2_USE_EVENTS_IN
        if (port == index++)
        {
            fPortEventsIn = (LV2_Atom_Sequence*)dataLocation;
            return;
        }
#endif

#if DISTRHO_LV2_USE_EVENTS_OUT
        if (port == index++)
        {
            fPortEventsOut = (LV2_Atom_Sequence*)dataLocation;
            return;
        }
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (port == index++)
        {
            fPortLatency = (float*)dataLocation;
            return;
        }
#endif

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (port == index++)
            {
                fPortControls[i] = (float*)dataLocation;
                return;
            }
        }
    }

    // -------------------------------------------------------------------

    void lv2_run(const uint32_t sampleCount)
    {
        // pre-roll
        if (sampleCount == 0)
            return updateParameterOutputs();

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPortControls[i] == nullptr)
                continue;

            curValue = *fPortControls[i];

            if (fLastControlValues[i] != curValue && ! fPlugin.isParameterOutput(i))
            {
                fLastControlValues[i] = curValue;
                fPlugin.setParameterValue(i, curValue);
            }
        }

#if DISTRHO_LV2_USE_EVENTS_IN
# if DISTRHO_PLUGIN_IS_SYNTH
        uint32_t midiEventCount = 0;
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
        bool needsFrameIncrement = true;
# endif
        LV2_ATOM_SEQUENCE_FOREACH(fPortEventsIn, event)
        {
            if (event == nullptr)
                break;

# if DISTRHO_PLUGIN_IS_SYNTH
            if (event->body.type == fURIDs.midiEvent)
            {
                if (event->body.size > 4 || midiEventCount >= kMaxMidiEvents)
                    continue;

                const uint8_t* const data((const uint8_t*)(event + 1));

                MidiEvent& midiEvent(fMidiEvents[midiEventCount]);

                midiEvent.frame = event->time.frames;
                midiEvent.size  = event->body.size;

                uint8_t i;
                for (i=0; i < midiEvent.size; ++i)
                    midiEvent.buf[i] = data[i];
                for (; i < 4; ++i)
                  midiEvent.buf[i] = 0;

                ++midiEventCount;
                continue;
            }
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
            if (event->body.type == fURIDs.atomBlank)
            {
                const LV2_Atom_Object* const obj((const LV2_Atom_Object*)&event->body);

                if (obj->body.otype != fURIDs.timePosition)
                    continue;

                LV2_Atom* bar     = nullptr;
                LV2_Atom* barBeat = nullptr;
                LV2_Atom* beat     = nullptr;
                LV2_Atom* beatUnit = nullptr;
                LV2_Atom* beatsPerBar = nullptr;
                LV2_Atom* beatsPerMinute = nullptr;
                LV2_Atom* frame = nullptr;
                LV2_Atom* speed = nullptr;

                lv2_atom_object_get(obj,
                                    fURIDs.timeBar, &bar,
                                    fURIDs.timeBarBeat, &barBeat,
                                    fURIDs.timeBeat, &beat,
                                    fURIDs.timeBeatUnit, &beatUnit,
                                    fURIDs.timeBeatsPerBar, &beatsPerBar,
                                    fURIDs.timeBeatsPerMinute, &beatsPerMinute,
                                    fURIDs.timeFrame, &frame,
                                    fURIDs.timeSpeed, &speed,
                                    nullptr);

                // TODO:
                // - tick
                // - barStartTick
                // - ticksPerBeat

                if (bar != nullptr)
                {
                     if (bar->type == fURIDs.atomDouble)
                        fTimePos.bbt.bar = ((LV2_Atom_Double*)bar)->body + 1.0f;
                    else if (bar->type == fURIDs.atomFloat)
                        fTimePos.bbt.bar = ((LV2_Atom_Float*)bar)->body + 1.0f;
                    else if (bar->type == fURIDs.atomInt)
                        fTimePos.bbt.bar = ((LV2_Atom_Int*)bar)->body + 1;
                    else if (bar->type == fURIDs.atomLong)
                        fTimePos.bbt.bar = ((LV2_Atom_Long*)bar)->body + 1;
                }

                /*if (barBeat != nullptr && barBeat->type == fURIDs.atomFloat)
                {
                }*/

                if (beat != nullptr)
                {
                     if (beat->type == fURIDs.atomDouble)
                        fTimePos.bbt.beat = ((LV2_Atom_Double*)beat)->body + 1.0f;
                    else if (beat->type == fURIDs.atomFloat)
                        fTimePos.bbt.beat = ((LV2_Atom_Float*)beat)->body + 1.0f;
                    else if (beat->type == fURIDs.atomInt)
                        fTimePos.bbt.beat = ((LV2_Atom_Int*)beat)->body + 1;
                    else if (beat->type == fURIDs.atomLong)
                        fTimePos.bbt.beat = ((LV2_Atom_Long*)beat)->body + 1;
                }

                if (beatUnit != nullptr)
                {
                    if (beatUnit->type == fURIDs.atomDouble)
                        fTimePos.bbt.beatType = ((LV2_Atom_Double*)beatUnit)->body;
                    else if (beatUnit->type == fURIDs.atomFloat)
                        fTimePos.bbt.beatType = ((LV2_Atom_Float*)beatUnit)->body;
                    else if (beatUnit->type == fURIDs.atomInt)
                        fTimePos.bbt.beatType = ((LV2_Atom_Int*)beatUnit)->body;
                    else if (beatUnit->type == fURIDs.atomLong)
                        fTimePos.bbt.beatType = ((LV2_Atom_Long*)beatUnit)->body;
                }

                if (beatsPerBar != nullptr)
                {
                    if (beatsPerBar->type == fURIDs.atomDouble)
                        fTimePos.bbt.beatsPerBar = ((LV2_Atom_Double*)beatsPerBar)->body;
                    else if (beatsPerBar->type == fURIDs.atomFloat)
                        fTimePos.bbt.beatsPerBar = ((LV2_Atom_Float*)beatsPerBar)->body;
                    else if (beatsPerBar->type == fURIDs.atomInt)
                        fTimePos.bbt.beatsPerBar = ((LV2_Atom_Int*)beatsPerBar)->body;
                    else if (beatsPerBar->type == fURIDs.atomLong)
                        fTimePos.bbt.beatsPerBar = ((LV2_Atom_Long*)beatsPerBar)->body;
                }

                if (beatsPerMinute != nullptr)
                {
                    if (beatsPerMinute->type == fURIDs.atomDouble)
                        fTimePos.bbt.beatsPerMinute = ((LV2_Atom_Double*)beatsPerMinute)->body;
                    else if (beatsPerMinute->type == fURIDs.atomFloat)
                        fTimePos.bbt.beatsPerMinute = ((LV2_Atom_Float*)beatsPerMinute)->body;
                    else if (beatsPerMinute->type == fURIDs.atomInt)
                        fTimePos.bbt.beatsPerMinute = ((LV2_Atom_Int*)beatsPerMinute)->body;
                    else if (beatsPerMinute->type == fURIDs.atomLong)
                        fTimePos.bbt.beatsPerMinute = ((LV2_Atom_Long*)beatsPerMinute)->body;
                }

                if (frame != nullptr && frame->type == fURIDs.atomLong)
                {
                    fTimePos.frame = ((LV2_Atom_Long*)frame)->body;
                    needsFrameIncrement = false;
                }

                if (speed != nullptr && speed->type == fURIDs.atomFloat)
                {
                    fLastTimeSpeed = ((LV2_Atom_Float*)speed)->body;
                    fTimePos.playing = (fLastTimeSpeed == 1.0f);
                }

                if ((! fTimePos.bbt.valid) && beatsPerMinute != nullptr && beatsPerBar != nullptr && beatUnit != nullptr)
                    fTimePos.bbt.valid = true;

                continue;
            }
# endif
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            if (event->body.type == fURIDs.distrhoState && fWorker != nullptr)
            {
                const void* const data((const void*)(event + 1));

                // check if this is our special message
                if (std::strcmp((const char*)data, "__dpf_ui_data__") == 0)
                {
                    for (uint32_t i=0, count=fPlugin.getStateCount(); i < count; ++i)
                        fNeededUiSends[i] = true;
                }
                else
                // no, send to DSP as usual
                {
                    fWorker->schedule_work(fWorker->handle, event->body.size, data);
                }

                continue;
            }
# endif
        }
#endif

# if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (needsFrameIncrement && fLastTimeSpeed != 0.0f)
            fTimePos.frame += fLastTimeSpeed*sampleCount;
        fPlugin.setTimePos(fTimePos);
# endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fPlugin.run(fPortAudioIns, fPortAudioOuts, sampleCount, fMidiEvents, midiEventCount);
#else
        fPlugin.run(fPortAudioIns, fPortAudioOuts, sampleCount);
#endif

        updateParameterOutputs();

#if DISTRHO_LV2_USE_EVENTS_OUT
        const uint32_t capacity = fPortEventsOut->atom.size;

        bool needsInit = true;
        uint32_t size, offset = 0;
        LV2_Atom_Event* aev;

        for (uint32_t i=0, count=fPlugin.getStateCount(); i < count; ++i)
        {
            if (! fNeededUiSends[i])
                continue;

            const d_string& key = fPlugin.getStateKey(i);

            for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
            {
                const d_string& curKey = it->first;

                if (curKey != key)
                    continue;

                const d_string& value = it->second;

                // TODO - RT safe
                d_stdout("Got msg (from DSP to UI via host):\n%s\n%s", (const char*)key, (const char*)value);

                // join key and value
                std::string tmpStr;
                tmpStr += std::string(key);
                tmpStr += std::string("\0", 1);
                tmpStr += std::string(value);

                // get msg size
                const size_t msgSize(tmpStr.size()+1);

                if (sizeof(LV2_Atom_Event) + msgSize > capacity - offset)
                    return;

                if (needsInit)
                {
                    fPortEventsOut->atom.size = 0;
                    fPortEventsOut->atom.type = fURIDs.atomSequence;
                    fPortEventsOut->body.unit = 0;
                    fPortEventsOut->body.pad  = 0;
                    needsInit = false;
                }

                // reserve atom space
                const size_t atomSize(lv2_atom_pad_size(sizeof(LV2_Atom) + msgSize));
                char         atomBuf[atomSize];
                std::memset(atomBuf, 0, atomSize);

                // put data
                aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, fPortEventsOut) + offset);
                aev->time.frames = 0;
                aev->body.type   = fURIDs.distrhoState;
                aev->body.size   = msgSize;
                std::memcpy(LV2_ATOM_BODY(&aev->body), tmpStr.data(), msgSize-1);

                size    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + msgSize);
                offset += size;
                fPortEventsOut->atom.size += size;

                fNeededUiSends[i] = false;
                break;
            }
        }
#endif
    }

    // -------------------------------------------------------------------

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/)
    {
        // currently unused
        return LV2_OPTIONS_ERR_UNKNOWN;
    }

    uint32_t lv2_set_options(const LV2_Options_Option* const options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == fUridMap->map(fUridMap->handle, LV2_BUF_SIZE__maxBlockLength))
            {
                if (options[i].type == fUridMap->map(fUridMap->handle, LV2_ATOM__Int))
                {
                    const int bufferSize(*(const int*)options[i].value);
                    fPlugin.setBufferSize(bufferSize);
                    return LV2_OPTIONS_SUCCESS;
                }
                else
                {
                    d_stderr("Host changed maxBlockLength but with wrong value type");
                    return LV2_OPTIONS_ERR_BAD_VALUE;
                }
            }
            else if (options[i].key == fUridMap->map(fUridMap->handle, LV2_CORE__sampleRate))
            {
                if (options[i].type == fUridMap->map(fUridMap->handle, LV2_ATOM__Double))
                {
                    const double sampleRate(*(const double*)options[i].value);
                    fPlugin.setSampleRate(sampleRate);
                    return LV2_OPTIONS_SUCCESS;
                }
                else
                {
                    d_stderr("Host changed sampleRate but with wrong value type");
                    return LV2_OPTIONS_ERR_BAD_VALUE;
                }
            }
        }

        return LV2_OPTIONS_ERR_BAD_KEY;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    const LV2_Program_Descriptor* lv2_get_program(const uint32_t index)
    {
        if (index >= fPlugin.getProgramCount())
            return nullptr;

        static LV2_Program_Descriptor desc;

        desc.bank    = index / 128;
        desc.program = index % 128;
        desc.name    = fPlugin.getProgramName(index);

        return &desc;
    }

    void lv2_select_program(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.getProgramCount())
            return;

        fPlugin.setProgram(realProgram);

        // Update control inputs
        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
                continue;

            fLastControlValues[i] = fPlugin.getParameterValue(i);

            if (fPortControls[i] != nullptr)
                *fPortControls[i] = fLastControlValues[i];
        }
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_STATE
    LV2_State_Status lv2_save(const LV2_State_Store_Function store, const LV2_State_Handle handle, const uint32_t flags)
    {
        //flags = LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE;

        for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
        {
            const d_string& key   = it->first;
            const d_string& value = it->second;

            store(handle, fUridMap->map(fUridMap->handle, (const char*)key), (const char*)value, value.length()+1, fURIDs.atomString, flags);
        }

        return LV2_STATE_SUCCESS;
    }

    LV2_State_Status lv2_restore(const LV2_State_Retrieve_Function retrieve, const LV2_State_Handle handle)
    {
        size_t   size  = 0;
        uint32_t type  = 0;
        uint32_t flags = LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE;

        for (uint32_t i=0, count=fPlugin.getStateCount(); i < count; ++i)
        {
            const d_string& key = fPlugin.getStateKey(i);

            const void* data = retrieve(handle, fUridMap->map(fUridMap->handle, (const char*)key), &size, &type, &flags);

            if (size == 0)
                continue;
            if (data == nullptr)
                continue;
            if (type != fURIDs.atomString)
                continue;

            const char* const value((const char*)data);

            if (std::strlen(value) != size)
                continue;

            setState(key, value);

            d_stdout("Got state msg:\n%s\n%s", (const char*)key, value);

#if DISTRHO_LV2_USE_EVENTS_OUT
            // signal msg needed for UI
            fNeededUiSends[i] = true;
#endif
        }

        return LV2_STATE_SUCCESS;
    }

    // -------------------------------------------------------------------

    LV2_Worker_Status lv2_work(const void* const data)
    {
        const char* const stateKey((const char*)data);
        const char* const stateValue(stateKey+std::strlen(stateKey)+1);

        setState(stateKey, stateValue);

        return LV2_WORKER_SUCCESS;
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    void* lv2_get_instance_pointer()
    {
        return fPlugin.getInstancePointer();
    }
#endif

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    // LV2 ports
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    const float*  fPortAudioIns[DISTRHO_PLUGIN_NUM_INPUTS];
#else
    const float** fPortAudioIns;
#endif
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    float*  fPortAudioOuts[DISTRHO_PLUGIN_NUM_OUTPUTS];
#else
    float** fPortAudioOuts;
#endif
    float** fPortControls;
#if DISTRHO_LV2_USE_EVENTS_IN
    LV2_Atom_Sequence* fPortEventsIn;
#endif
#if DISTRHO_LV2_USE_EVENTS_OUT
    LV2_Atom_Sequence* fPortEventsOut;
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
    float* fPortLatency;
#endif

    // Temporary data
    float* fLastControlValues;
#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent fMidiEvents[kMaxMidiEvents];
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePos fTimePos;
    float   fLastTimeSpeed;
#endif

    // LV2 URIDs
#if DISTRHO_LV2_USE_EVENTS_IN || DISTRHO_LV2_USE_EVENTS_OUT
    struct URIDs {
        LV2_URID atomBlank;
        LV2_URID atomDouble;
        LV2_URID atomFloat;
        LV2_URID atomInt;
        LV2_URID atomLong;
        LV2_URID atomSequence;
        LV2_URID atomString;
        LV2_URID distrhoState;
        LV2_URID midiEvent;
        LV2_URID timePosition;
        LV2_URID timeBar;
        LV2_URID timeBarBeat;
        LV2_URID timeBeat;
        LV2_URID timeBeatUnit;
        LV2_URID timeBeatsPerBar;
        LV2_URID timeBeatsPerMinute;
        LV2_URID timeFrame;
        LV2_URID timeSpeed;

        URIDs(const LV2_URID_Map* const uridMap)
            : atomBlank(uridMap->map(uridMap->handle, LV2_ATOM__Blank)),
              atomDouble(uridMap->map(uridMap->handle, LV2_ATOM__Double)),
              atomFloat(uridMap->map(uridMap->handle, LV2_ATOM__Float)),
              atomInt(uridMap->map(uridMap->handle, LV2_ATOM__Int)),
              atomLong(uridMap->map(uridMap->handle, LV2_ATOM__Long)),
              atomSequence(uridMap->map(uridMap->handle, LV2_ATOM__Sequence)),
              atomString(uridMap->map(uridMap->handle, LV2_ATOM__String)),
              distrhoState(uridMap->map(uridMap->handle, "urn:distrho:keyValueState")),
              midiEvent(uridMap->map(uridMap->handle, LV2_MIDI__MidiEvent)),
              timePosition(uridMap->map(uridMap->handle, LV2_TIME__Position)),
              timeBar(uridMap->map(uridMap->handle, LV2_TIME__bar)),
              timeBarBeat(uridMap->map(uridMap->handle, LV2_TIME__barBeat)),
              timeBeat(uridMap->map(uridMap->handle, LV2_TIME__beat)),
              timeBeatUnit(uridMap->map(uridMap->handle, LV2_TIME__beatUnit)),
              timeBeatsPerBar(uridMap->map(uridMap->handle, LV2_TIME__beatsPerBar)),
              timeBeatsPerMinute(uridMap->map(uridMap->handle, LV2_TIME__beatsPerMinute)),
              timeFrame(uridMap->map(uridMap->handle, LV2_TIME__frame)),
              timeSpeed(uridMap->map(uridMap->handle, LV2_TIME__speed)) {}
    } fURIDs;
#endif

    // LV2 features
    const LV2_URID_Map* const fUridMap;
    const LV2_Worker_Schedule* const fWorker;

#if DISTRHO_PLUGIN_WANT_STATE
    StringMap fStateMap;
    bool* fNeededUiSends;

    void setState(const char* const key, const char* const newValue)
    {
        fPlugin.setState(key, newValue);

        // check if we want to save this key
        if (! fPlugin.wantStateKey(key))
            return;

        // check if key already exists
        for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
        {
            const d_string& d_key = it->first;

            if (d_key == key)
            {
                it->second = newValue;
                return;
            }
        }

        // nope, add a new one then
        d_string d_key(key);
        fStateMap[d_key] = newValue;
    }
#endif

    void updateParameterOutputs()
    {
        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (! fPlugin.isParameterOutput(i))
                continue;

            fLastControlValues[i] = fPlugin.getParameterValue(i);

            if (fPortControls[i] != nullptr)
                *fPortControls[i] = fLastControlValues[i];
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (fPortLatency != nullptr)
            *fPortLatency = fPlugin.getLatency();
#endif
    }
};

// -----------------------------------------------------------------------

static LV2_Handle lv2_instantiate(const LV2_Descriptor*, double sampleRate, const char*, const LV2_Feature* const* features)
{
    const LV2_Options_Option* options = nullptr;
    const LV2_URID_Map*       uridMap = nullptr;
    const LV2_Worker_Schedule* worker = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            uridMap = (const LV2_URID_Map*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_WORKER__schedule) == 0)
            worker = (const LV2_Worker_Schedule*)features[i]->data;
    }

    if (options == nullptr)
    {
        d_stderr("Options feature missing, cannot continue!");
        return nullptr;
    }

    if (uridMap == nullptr)
    {
        d_stderr("URID Map feature missing, cannot continue!");
        return nullptr;
    }

#if DISTRHO_PLUGIN_WANT_STATE
    if (worker == nullptr)
    {
        d_stderr("Worker feature missing, cannot continue!");
        return nullptr;
    }
#endif

    d_lastBufferSize = 0;

    for (int i=0; options[i].key != 0; ++i)
    {
        if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
        {
            if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
                d_lastBufferSize = *(const int*)options[i].value;
            else
                d_stderr("Host provides maxBlockLength but has wrong value type");

            break;
        }
    }

    if (d_lastBufferSize == 0)
    {
        d_stderr("Host does not provide maxBlockLength option");
        d_lastBufferSize = 2048;
    }

    d_lastSampleRate = sampleRate;

    return new PluginLv2(uridMap, worker);
}

#define instancePtr ((PluginLv2*)instance)

static void lv2_connect_port(LV2_Handle instance, uint32_t port, void* dataLocation)
{
    instancePtr->lv2_connect_port(port, dataLocation);
}

static void lv2_activate(LV2_Handle instance)
{
    instancePtr->lv2_activate();
}

static void lv2_run(LV2_Handle instance, uint32_t sampleCount)
{
    instancePtr->lv2_run(sampleCount);
}

static void lv2_deactivate(LV2_Handle instance)
{
    instancePtr->lv2_deactivate();
}

static void lv2_cleanup(LV2_Handle instance)
{
    delete instancePtr;
}

// -----------------------------------------------------------------------

static uint32_t lv2_get_options(LV2_Handle instance, LV2_Options_Option* options)
{
    return instancePtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    return instancePtr->lv2_set_options(options);
}

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
static const LV2_Program_Descriptor* lv2_get_program(LV2_Handle instance, uint32_t index)
{
    return instancePtr->lv2_get_program(index);
}

static void lv2_select_program(LV2_Handle instance, uint32_t bank, uint32_t program)
{
    instancePtr->lv2_select_program(bank, program);
}
#endif

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_STATE
static LV2_State_Status lv2_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const*)
{
    return instancePtr->lv2_save(store, handle, flags);
}

static LV2_State_Status lv2_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t, const LV2_Feature* const*)
{
    return instancePtr->lv2_restore(retrieve, handle);
}

LV2_Worker_Status lv2_work(LV2_Handle instance, LV2_Worker_Respond_Function, LV2_Worker_Respond_Handle, uint32_t, const void* data)
{
    return instancePtr->lv2_work(data);
}
#endif

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
static void* lv2_get_instance_pointer(LV2_Handle instance)
{
    return instancePtr->lv2_get_instance_pointer();
}
#endif

// -----------------------------------------------------------------------

static const void* lv2_extension_data(const char* uri)
{
    static const LV2_Options_Interface options = { lv2_get_options, lv2_set_options };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    static const LV2_Programs_Interface programs = { lv2_get_program, lv2_select_program };

    if (std::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
        return &programs;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    static const LV2_State_Interface state = { lv2_save, lv2_restore };
    static const LV2_Worker_Interface worker = { lv2_work, nullptr, nullptr };

    if (std::strcmp(uri, LV2_STATE__interface) == 0)
        return &state;
    if (std::strcmp(uri, LV2_WORKER__interface) == 0)
        return &worker;
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
# define DISTRHO_DIRECT_ACCESS_URI "urn:distrho:direct-access"

    struct LV2_DirectAccess_Interface {
        void* (*get_instance_pointer)(LV2_Handle handle);
    };

    static const LV2_DirectAccess_Interface directaccess = { lv2_get_instance_pointer };

    if (std::strcmp(uri, DISTRHO_DIRECT_ACCESS_URI) == 0)
        return &directaccess;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------

static const LV2_Descriptor sLv2Descriptor = {
    DISTRHO_PLUGIN_URI,
    lv2_instantiate,
    lv2_connect_port,
    lv2_activate,
    lv2_run,
    lv2_deactivate,
    lv2_cleanup,
    lv2_extension_data
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLv2Descriptor : nullptr;
}

// -----------------------------------------------------------------------
