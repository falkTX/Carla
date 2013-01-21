/*
 * DISTRHO PingPongPan Plugin, based on PingPongPan by Michael Gruhn
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoUIPingPongPan.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

DistrhoUIPingPongPan::DistrhoUIPingPongPan()
    : OpenGLExtUI()
{
    // background
    Image bgImage(DistrhoArtworkPingPongPan::backgroundData, DistrhoArtworkPingPongPan::backgroundWidth, DistrhoArtworkPingPongPan::backgroundHeight, GL_BGRA);
    setBackgroundImage(bgImage);

    // knobs
    Image knobImage(DistrhoArtworkPingPongPan::knobData, DistrhoArtworkPingPongPan::knobWidth, DistrhoArtworkPingPongPan::knobHeight);
    Point knobPos(136, 30);

    // knob Low-Mid
    knobFreq = new ImageKnob(knobImage, knobPos);
    knobFreq->setRange(0.0f, 100.0f);
    knobFreq->setValue(50.0f);
    addImageKnob(knobFreq);

    // knob Mid-High
    knobPos.setX(258);
    knobWidth = new ImageKnob(knobImage, knobPos);
    knobWidth->setRange(0.0f, 100.0f);
    knobWidth->setValue(75.0f);
    addImageKnob(knobWidth);

    // about button
    Image aboutImageNormal(DistrhoArtworkPingPongPan::aboutButtonNormalData, DistrhoArtworkPingPongPan::aboutButtonNormalWidth, DistrhoArtworkPingPongPan::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtworkPingPongPan::aboutButtonHoverData, DistrhoArtworkPingPongPan::aboutButtonHoverWidth, DistrhoArtworkPingPongPan::aboutButtonHoverHeight);
    Point aboutPos(25, 23);
    buttonAbout = new ImageButton(aboutImageNormal, aboutImageHover, aboutImageHover, aboutPos);
    addImageButton(buttonAbout);
}

DistrhoUIPingPongPan::~DistrhoUIPingPongPan()
{
    delete knobFreq;
    delete knobWidth;
    delete buttonAbout;
}

// -------------------------------------------------
// DSP Callbacks

void DistrhoUIPingPongPan::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case DistrhoPluginPingPongPan::paramFreq:
        knobFreq->setValue(value);
        break;
    case DistrhoPluginPingPongPan::paramWidth:
        knobWidth->setValue(value);
        break;
    }

    d_uiRepaint();
}

void DistrhoUIPingPongPan::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    knobFreq->setValue(50.0f);
    knobWidth->setValue(75.0f);

    d_uiRepaint();
}

// -------------------------------------------------
// Extended Callbacks

void DistrhoUIPingPongPan::imageButtonClicked(ImageButton* button)
{
    if (button != buttonAbout)
        return;

    Image imageAbout(DistrhoArtworkPingPongPan::aboutData, DistrhoArtworkPingPongPan::aboutWidth, DistrhoArtworkPingPongPan::aboutHeight, GL_BGRA);
    showImageModalDialog(imageAbout, "About");
}

void DistrhoUIPingPongPan::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == knobFreq)
        d_uiEditParameter(DistrhoPluginPingPongPan::paramFreq, true);
    else if (knob == knobWidth)
        d_uiEditParameter(DistrhoPluginPingPongPan::paramWidth, true);
}

void DistrhoUIPingPongPan::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == knobFreq)
        d_uiEditParameter(DistrhoPluginPingPongPan::paramFreq, false);
    else if (knob == knobWidth)
        d_uiEditParameter(DistrhoPluginPingPongPan::paramWidth, false);
}


void DistrhoUIPingPongPan::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == knobFreq)
        d_setParameterValue(DistrhoPluginPingPongPan::paramFreq, value);
    else if (knob == knobWidth)
        d_setParameterValue(DistrhoPluginPingPongPan::paramWidth, value);
}

// -------------------------------------------------

UI* createUI()
{
    return new DistrhoUIPingPongPan;
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
