// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/* This plugin code is based on MOD Devices' midi-to-cv-mono by Bram Giesen and Jarno Verheesen
 */

#include "CarlaNative.h"
#include "CarlaMIDI.h"

#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------

#define NUM_NOTESBUFFER 8

typedef enum {
    PARAM_OCTAVE = 0,
    PARAM_SEMITONE,
    PARAM_CENT,
    PARAM_RETRIGGER,
    PARAM_COUNT
} Midi2CvParams;

typedef struct {
    // keep track of active notes
    uint8_t activeNotesList[NUM_NOTESBUFFER];
    uint8_t reTriggerBuffer[NUM_NOTESBUFFER];
    uint8_t triggerIndex;
    uint8_t activeNotes;
    uint8_t activeVelocity;
    uint8_t reTriggered;
    size_t  notesIndex;
    bool activePorts;

    // other stuff
    bool triggerState;
    int notesPressed;
    float params[PARAM_COUNT];

} Midi2CvHandle;

static void panic(Midi2CvHandle* const handle)
{
    memset(handle->activeNotesList, 200, sizeof(uint8_t)*NUM_NOTESBUFFER);
    memset(handle->reTriggerBuffer, 0, sizeof(uint8_t)*NUM_NOTESBUFFER);
    handle->triggerIndex = 0;
    handle->reTriggered = 200;
    handle->activeNotes = 0;
    handle->activeVelocity = 0;
    handle->activePorts = false;
    handle->notesPressed = 0;
    handle->notesIndex = 0;
    handle->triggerState = false;
}

static void set_status(Midi2CvHandle* const handle, int status)
{
  handle->activePorts = status;
  handle->triggerState = status;
}

// -----------------------------------------------------------------------

static NativePluginHandle midi2cv_instantiate(const NativeHostDescriptor* host)
{
    Midi2CvHandle* const handle = (Midi2CvHandle*)malloc(sizeof(Midi2CvHandle));

    if (handle == NULL)
        return NULL;

    panic(handle);
    memset(handle->params, 0, sizeof(float)*PARAM_COUNT);

    return handle;

    // unused
    (void)host;
}

#define handlePtr ((Midi2CvHandle*)handle)

static void midi2cv_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midi2cv_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* midi2cv_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;
    param.unit  = NULL;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    switch (index)
    {
    case PARAM_OCTAVE:
        param.name = "Octave";
        param.hints |= NATIVE_PARAMETER_IS_INTEGER;
        param.ranges.def = 0.0f;
        param.ranges.min = -3.0f;
        param.ranges.max = 3.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_SEMITONE:
        param.name   = "Semitone";
        param.hints |= NATIVE_PARAMETER_IS_INTEGER;
        param.ranges.def = 0.0f;
        param.ranges.min = -12.0f;
        param.ranges.max = 12.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 6.0f;
        break;
    case PARAM_CENT:
        param.name   = "Cent";
        param.hints |= NATIVE_PARAMETER_IS_INTEGER;
        param.ranges.def = 0.0f;
        param.ranges.min = -100.0f;
        param.ranges.max = 100.0f;
        param.ranges.step = 10.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 50.0f;
        break;
    case PARAM_RETRIGGER:
        param.name   = "Retrigger";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 0.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float midi2cv_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    return handlePtr->params[index];
}

static void midi2cv_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    handlePtr->params[index] = value;
}

static const NativePortRange* midi2cv_get_buffer_port_range(NativePluginHandle handle, uint32_t index, bool isOutput)
{
    if (! isOutput)
        return NULL;

    static NativePortRange npr;

    switch (index)
    {
    case 0:
        npr.minimum = 0.0f;
        npr.maximum = 9.0f;
        return &npr;
    case 1:
        npr.minimum = 0.0f;
        npr.maximum = 10.5f;
        return &npr;
    case 2:
        npr.minimum = 0.0f;
        npr.maximum = 10.0f;
        return &npr;
    default:
        return NULL;
    }

    // unused
    (void)handle;
}

static const char* midi2cv_get_buffer_port_name(NativePluginHandle handle, uint32_t index, bool isOutput)
{
    if (! isOutput)
        return NULL;

    switch (index)
    {
    case 0:
        return "Pitch";
    case 1:
        return "Velocity";
    case 2:
        return "Gate";
    default:
        return NULL;
    }

    // unused
    (void)handle;
}

static void midi2cv_activate(NativePluginHandle handle)
{
    panic(handlePtr);
}

// FIXME for v3.0, use const for the input buffer
static void midi2cv_process(NativePluginHandle handle,
                                float** inBuffer, float** outBuffer, uint32_t frames,
                                const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    float* const pitch    = outBuffer[0];
    float* const velocity = outBuffer[1];
    float* const trigger  = outBuffer[2];

    const float oC = handlePtr->params[PARAM_OCTAVE];
    const float sC = handlePtr->params[PARAM_SEMITONE];
    const float cC = handlePtr->params[PARAM_CENT];
    const bool  rC = handlePtr->params[PARAM_RETRIGGER] > 0.5f;

    bool retrigger = true;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        if (midiEvent->size <= 1 || midiEvent->size > 3)
            continue;

        const uint8_t* const mdata = midiEvent->data;
        const uint8_t status = MIDI_GET_STATUS_FROM_DATA(mdata);

        int storeN = 0;
        bool emptySlot = false;
        int notesIndex = NUM_NOTESBUFFER - 1;
        bool noteFound = false;

        switch (status)
        {
        case MIDI_STATUS_NOTE_ON:
            while (!emptySlot && storeN < NUM_NOTESBUFFER)
            {
                if (handlePtr->activeNotesList[storeN] == 200)
                {
                    handlePtr->activeNotesList[storeN] = mdata[1];
                    emptySlot = true;
                }
                storeN++;
            }
            handlePtr->activeNotes = mdata[1];
            handlePtr->activeVelocity = mdata[2];
            handlePtr->triggerIndex = (handlePtr->triggerIndex + 1U) % 8U;
            handlePtr->reTriggerBuffer[handlePtr->triggerIndex] = 1U;
            handlePtr->reTriggered = mdata[1];
            break;

        case MIDI_STATUS_NOTE_OFF:
            handlePtr->notesPressed--;
            for (int n = 0; n < NUM_NOTESBUFFER; ++n)
                if (mdata[1] == handlePtr->activeNotesList[n])
                    handlePtr->activeNotesList[n] = 200;

            while (!noteFound && notesIndex >= 0)
            {
                if (handlePtr->activeNotesList[notesIndex] < 200)
                {
                    handlePtr->activeNotes = handlePtr->activeNotesList[notesIndex];
                    if(retrigger && handlePtr->activeNotes != handlePtr->reTriggered)
                    {
                        handlePtr->reTriggered = mdata[1];
                    }
                    noteFound = true;
                }
                notesIndex--;
            }
            break;

        case MIDI_STATUS_CONTROL_CHANGE:
            if (mdata[1] == MIDI_CONTROL_ALL_NOTES_OFF)
                panic(handlePtr);
            break;
        }
    }

    int checked_note = 0;
    bool active_notes_found = false;
    while (checked_note < NUM_NOTESBUFFER && ! active_notes_found)
    {
        if (handlePtr->activeNotesList[checked_note] != 200)
            active_notes_found = true;
        checked_note++;
    }

    if (active_notes_found)
    {
        set_status(handlePtr, 1);
    }
    else
    {
        set_status(handlePtr, 0);
        handlePtr->activeVelocity = 0;
    }

    for (uint32_t i=0; i<frames; ++i)
    {
        pitch[i]    = (0.0f + (float)((oC) + (sC/12.0f)+(cC/1200.0f)) + ((float)handlePtr->activeNotes * 1/12.0f));
        velocity[i] = (0.0f + ((float)handlePtr->activeVelocity * 1/12.0f));
        trigger[i]  = ((handlePtr->triggerState == true) ? 10.0f : 0.0f);

        if (handlePtr->reTriggerBuffer[handlePtr->triggerIndex] == 1 && rC)
        {
            handlePtr->reTriggerBuffer[handlePtr->triggerIndex] = 0;
            trigger[i] = 0.0f;
        }
    }

    return;

    // unused
    (void)inBuffer;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const NativePluginDescriptor midi2cvDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE|NATIVE_PLUGIN_USES_CONTROL_VOLTAGE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF,
    .audioIns  = 0,
    .audioOuts = 0,
    .cvIns     = 0,
    .cvOuts    = 3, // pitch, velocity, gate
    .midiIns   = 1,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT,
    .paramOuts = 0,
    .name      = "MIDI to CV",
    .label     = "midi2cv",
    .maker     = "falkTX, Bram Giesen, Jarno Verheesen",
    .copyright = "GNU GPL v2+",

    .instantiate = midi2cv_instantiate,
    .cleanup     = midi2cv_cleanup,

    .get_parameter_count = midi2cv_get_parameter_count,
    .get_parameter_info  = midi2cv_get_parameter_info,
    .get_parameter_value = midi2cv_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = midi2cv_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = midi2cv_activate,
    .deactivate = NULL,
    .process    = midi2cv_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL,

    .render_inline_display = NULL,

    .get_buffer_port_name = midi2cv_get_buffer_port_name,
    .get_buffer_port_range = midi2cv_get_buffer_port_range,
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midi2cv(void);

void carla_register_native_plugin_midi2cv(void)
{
    carla_register_native_plugin(&midi2cvDesc);
}

// -----------------------------------------------------------------------
