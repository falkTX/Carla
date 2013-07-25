/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/options.h"
#include "lv2/programs.h"
#include "lv2/state.h"
#include "lv2/urid.h"
#include "lv2/worker.h"

// #include "lv2/atom-util.h"
// #include "lv2/midi.h"

#include <map>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
# warning LV2 Synth still TODO
#endif
#if DISTRHO_PLUGIN_WANT_STATE
# warning LV2 State still TODO
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
# warning LV2 TimePos still TODO
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_STATE || DISTRHO_PLUGIN_WANT_TIMEPOS)
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_STATE)

typedef float* floatptr;
typedef std::map<d_string,d_string> StringMap;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class PluginLv2
{
public:
    PluginLv2(const LV2_URID_Map* const uridMap, const LV2_Worker_Schedule* const worker)
        : fPortControls(nullptr),
          fLastControlValues(nullptr),
          fUridMap(uridMap),
          fWorker(worker)
    {
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            fPortAudioIns[i] = nullptr;

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            fPortAudioOuts[i] = nullptr;

        {
            const uint32_t count(fPlugin.parameterCount());

            fPortControls = new floatptr[count];;
            fLastControlValues = new float[count];;

            for (uint32_t i=0; i < count; ++i)
            {
                fPortControls[i] = nullptr;
                fLastControlValues[i] = fPlugin.parameterValue(i);
            }
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

// #if DISTRHO_LV2_USE_EVENTS
//         uridIdAtomString   = 0;
// # if DISTRHO_PLUGIN_IS_SYNTH
//         uridIdMidiEvent    = 0;
// # endif
// # if DISTRHO_PLUGIN_WANT_STATE

// # endif
//
//         for (uint32_t i=0; features[i]; i++)
//         {
//             if (strcmp(features[i]->URI, LV2_URID__map) == 0)
//             {
//                 uridMap = (LV2_URID_Map*)features[i]->data;
//
//                 uridIdAtomString   = uridMap->map(uridMap->handle, LV2_ATOM__String);
// # if DISTRHO_PLUGIN_IS_SYNTH
//                 uridIdMidiEvent    = uridMap->map(uridMap->handle, LV2_MIDI__MidiEvent);
// # endif
// # if DISTRHO_PLUGIN_WANT_STATE
//                 uridIdPatchMessage = uridMap->map(uridMap->handle, LV2_PATCH__Message);
// # endif
//             }
//             else if (strcmp(features[i]->URI, LV2_WORKER__schedule) == 0)
//             {
//                 workerSchedule = (LV2_Worker_Schedule*)features[i]->data;
//             }
//         }
// #endif
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

// #if DISTRHO_PLUGIN_WANT_STATE
//         stateMap.clear();
// #endif
    }

    void lv2_activate()
    {
        fPlugin.activate();
    }

    void lv2_deactivate()
    {
        fPlugin.deactivate();
    }

    void lv2_connect_port(const uint32_t port, void* const dataLocation)
    {
        uint32_t index = 0;

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioIns[i] = (floatptr)dataLocation;
                return;
            }
        }

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioOuts[i] = (floatptr)dataLocation;
                return;
            }
        }

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
            fPortLatency = (floatptr)dataLocation;
            return;
        }
#endif

        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (port == index++)
            {
                fPortControls[i] = (floatptr)dataLocation;
                return;
            }
        }
    }

    void lv2_run(const uint32_t bufferSize)
    {
        if (bufferSize == 0)
        {
            updateParameterOutputs();
            return;
        }

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            assert(fPortControls[i] != nullptr);

            curValue = *fPortControls[i];

            if (fLastControlValues[i] != curValue && ! fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = curValue;
                fPlugin.setParameterValue(i, curValue);
            }
        }

#if DISTRHO_PLUGIN_IS_SYNTH
        // Get Events, xxx
        uint32_t midiEventCount = 0;

#if DISTRHO_LV2_USE_EVENTS
        LV2_ATOM_SEQUENCE_FOREACH(portEventsIn, iter)
        {
            const LV2_Atom_Event* event = /*(const LV2_Atom_Event*)*/iter;

            if (! event)
                continue;

# if DISTRHO_PLUGIN_IS_SYNTH
            if (event->body.type == uridIdMidiEvent)
            {
                if (event->time.frames >= bufferSize || midiEventCount >= MAX_MIDI_EVENTS)
                    break;

                if (event->body.size > 3)
                    continue;

                const uint8_t* data = (const uint8_t*)(event + 1);

                midiEvents[midiEventCount].frame = event->time.frames;
                memcpy(midiEvents[midiEventCount].buffer, data, event->body.size);

                midiEventCount += 1;
                continue;
            }
# endif
# if DISTRHO_PLUGIN_WANT_STATE
            if (event->body.type == uridIdPatchMessage)
            {
                // TODO
                //if (workerSchedule)
                //    workerSchedule->schedule_work(workerSchedule->handle, event->body.size, )
            }
# endif
        }
#endif

        fPlugin.run(fPortAudioIns, fPortAudioOuts, bufferSize, midiEventCount, midiEvents);
#else
        fPlugin.run(fPortAudioIns, fPortAudioOuts, bufferSize, 0, nullptr);
#endif

        updateParameterOutputs();
    }

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/)
    {
        // currently unused
        return LV2_OPTIONS_SUCCESS;
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
                }
                else
                    d_stderr("Host changed maxBlockLength but with wrong value type");
            }
            else if (options[i].key == fUridMap->map(fUridMap->handle, LV2_CORE__sampleRate))
            {
                if (options[i].type == fUridMap->map(fUridMap->handle, LV2_ATOM__Double))
                {
                    const double sampleRate(*(const double*)options[i].value);
                    fPlugin.setSampleRate(sampleRate);
                }
                else
                    d_stderr("Host changed sampleRate but with wrong value type");
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    const LV2_Program_Descriptor* lv2_get_program(const uint32_t index)
    {
        if (index >= fPlugin.programCount())
            return nullptr;

        static LV2_Program_Descriptor desc;

        desc.bank    = index / 128;
        desc.program = index % 128;
        desc.name    = fPlugin.programName(index);

        return &desc;
    }

    void lv2_select_program(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.programCount())
            return;

        fPlugin.setProgram(realProgram);

        // Update parameters
        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (! fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = fPlugin.parameterValue(i);

                if (fPortControls[i] != nullptr)
                    *fPortControls[i] = fLastControlValues[i];
            }
        }
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    LV2_State_Status lv2_save(LV2_State_Store_Function store, LV2_State_Handle handle, const uint32_t flags, const LV2_Feature* const* /*features*/)
    {
//         flags = LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE;

//         for (auto i = stateMap.begin(); i != stateMap.end(); i++)
//         {
//             const d_string& key   = i->first;
//             const d_string& value = i->second;
//
//             store(handle, uridMap->map(uridMap->handle, (const char*)key), (const char*)value, value.length(), uridIdAtomString, flags);
//         }

        return LV2_STATE_SUCCESS;
    }

    LV2_State_Status lv2_restore(LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, const uint32_t flags, const LV2_Feature* const* /*features*/)
    {
//         size_t   size;
//         uint32_t type;

//         for (uint32_t i=0; i < plugin.stateCount(); i++)
//         {
//             const d_string& key = plugin.stateKey(i);
//
//             const void* data = retrieve(handle, uridMap->map(uridMap->handle, (const char*)key), &size, &type, &flags);
//
//             if (size == 0 || ! data)
//                 continue;
//             if (type != uridIdAtomString)
//                 continue;
//
//             const char* value = (const char*)data;
//             setState(key, value);
//         }

        return LV2_STATE_SUCCESS;
    }

    LV2_Worker_Status lv2_work(LV2_Worker_Respond_Function /*respond*/, LV2_Worker_Respond_Handle /*handle*/, const uint32_t /*size*/, const void* /*data*/)
    {
        // TODO
        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status lv2_work_response(const uint32_t /*size*/, const void* /*body*/)
    {
        // TODO
        return LV2_WORKER_SUCCESS;
    }
#endif

private:
    PluginInternal fPlugin;

    // LV2 ports
    floatptr  fPortAudioIns[DISTRHO_PLUGIN_NUM_INPUTS];
    floatptr  fPortAudioOuts[DISTRHO_PLUGIN_NUM_OUTPUTS];
    floatptr* fPortControls;
#if DISTRHO_LV2_USE_EVENTS_IN
    LV2_Atom_Sequence* fPortEventsIn;
#endif
#if DISTRHO_LV2_USE_EVENTS_OUT
    LV2_Atom_Sequence* fPortEventsOut;
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
    floatptr fPortLatency;
#endif

    // Temporary data
    float* fLastControlValues;

// #if DISTRHO_PLUGIN_IS_SYNTH
//     MidiEvent fMidiEvents[MAX_MIDI_EVENTS];
// #endif

    // Feature data
    const LV2_URID_Map* const fUridMap;
    const LV2_Worker_Schedule* const fWorker;

    // xxx
// #if DISTRHO_LV2_USE_EVENTS
//
//     // URIDs
//     LV2_URID_Map* uridMap;
//     LV2_URID      uridIdAtomString;
// # if DISTRHO_PLUGIN_IS_SYNTH
//     LV2_URID      uridIdMidiEvent;
// # endif
// # if DISTRHO_PLUGIN_WANT_STATE
//     LV2_URID      uridIdPatchMessage;
//     LV2_Worker_Schedule* workerSchedule;
// # endif
// #endif

// #if DISTRHO_PLUGIN_WANT_STATE
//     stringMap stateMap;
//
//     void setState(const char* newKey, const char* newValue)
//     {
//         plugin.setState(newKey, newValue);
//
//         // check if key already exists
//         for (auto i = stateMap.begin(); i != stateMap.end(); i++)
//         {
//             const d_string& key = i->first;
//
//             if (key == newKey)
//             {
//                 i->second = newValue;
//                 return;
//             }
//         }
//
//         // add a new one then
//         stateMap[newKey] = newValue;
//     }
// #endif

    void updateParameterOutputs()
    {
        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = fPlugin.parameterValue(i);

                if (fPortControls[i] != nullptr)
                    *fPortControls[i] = fLastControlValues[i];
            }
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (fPortLatency != nullptr)
            *fPortLatency = fPlugin.latency();
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

    if (uridMap == nullptr)
    {
        d_stderr("URID Map feature missing, cannot continue!");
        return nullptr;
    }

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
        d_lastBufferSize = 512;

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

static uint32_t lv2_get_options(LV2_Handle instance, LV2_Options_Option* options)
{
    return instancePtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    return instancePtr->lv2_set_options(options);
}

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

#if DISTRHO_PLUGIN_WANT_STATE
static LV2_State_Status lv2_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    return instancePtr->lv2_save(store, handle, flags, features);
}

static LV2_State_Status lv2_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    return instancePtr->lv2_restore(retrieve, handle, flags, features);
}

LV2_Worker_Status lv2_work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    return instancePtr->lv2_work(respond, handle, size, data);
}

LV2_Worker_Status lv2_work_response(LV2_Handle instance, uint32_t size, const void* body)
{
    return instancePtr->lv2_work_response(size, body);
}
#endif

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
    static const LV2_Worker_Interface worker = { lv2_work, lv2_work_response, nullptr };

    if (std::strcmp(uri, LV2_STATE__interface) == 0)
        return &state;
    if (std::strcmp(uri, LV2_WORKER__interface) == 0)
        return &worker;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------

static LV2_Descriptor sLv2Descriptor = {
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
