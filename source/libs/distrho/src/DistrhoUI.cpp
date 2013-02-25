/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoUIInternal.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Static data

double d_lastUiSampleRate = 0.0;

// -------------------------------------------------
// UI

UI::UI()
    : pData(new UIPrivateData)
{
#if (defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2))
    pData->parameterOffset = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
    pData->parameterOffset += 1; // latency
# endif
    pData->parameterOffset += 1; // sample-rate
#endif

#ifdef DISTRHO_UI_QT4
    pData->ui = (Qt4UI*)this;
#else
    pData->ui = (OpenGLUI*)this;
#endif
}

UI::~UI()
{
    delete pData;
}

// -------------------------------------------------
// Host DSP State

double UI::d_sampleRate() const
{
    return pData->sampleRate;
}

void UI::d_editParameter(uint32_t index, bool started)
{
    pData->editParamCallback(index, started);
}

void UI::d_setParameterValue(uint32_t index, float value)
{
    pData->setParamCallback(index + pData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::d_setState(const char* key, const char* value)
{
    pData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::d_sendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
{
    pData->sendNoteCallback(onOff, channel, note, velocity);
}
#endif

// -------------------------------------------------
// Host UI State

void UI::d_uiResize(unsigned int width, unsigned int height)
{
    pData->uiResizeCallback(width, height);
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
