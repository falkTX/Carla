/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoUIInternal.h"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

double d_lastUiSampleRate = 0.0;

// -------------------------------------------------

UI::UI()
{
    data = new UIPrivateData;

#if (defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2))
    data->parameterOffset = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
    data->parameterOffset += 1;
# endif
    data->parameterOffset += 1; // sample-rate
#endif

#ifdef DISTRHO_UI_QT4
    data->widget = (Qt4UI*)this;
#endif
}

UI::~UI()
{
    delete data;
}

// -------------------------------------------------
// Host DSP State

double UI::d_sampleRate() const
{
    return data->sampleRate;
}

void UI::d_setParameterValue(uint32_t index, float value)
{
    data->setParamCallback(index + data->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::d_setState(const char* key, const char* value)
{
    data->setStateCallback(key, value);
}
#endif

// -------------------------------------------------
// Host UI State

void UI::d_uiEditParameter(uint32_t index, bool started)
{
    data->uiEditParamCallback(index, started);
}

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::d_uiSendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
{
    data->uiSendNoteCallback(onOff, channel, note, velocity);
}
#endif

void UI::d_uiResize(unsigned int width, unsigned int height)
{
    data->uiResizeCallback(width, height);
}

// -------------------------------------------------
// DSP Callbacks

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::d_uiNoteReceived(bool, uint8_t, uint8_t, uint8_t)
{
}
#endif

// -------------------------------------------------

END_NAMESPACE_DISTRHO
