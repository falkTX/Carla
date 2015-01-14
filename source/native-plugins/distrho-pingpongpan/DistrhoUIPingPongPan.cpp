/*
 * DISTRHO PingPongPan Plugin, based on PingPongPan by Michael Gruhn
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the LICENSE file.
 */

#include "DistrhoPluginPingPongPan.hpp"
#include "DistrhoUIPingPongPan.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoUIPingPongPan::DistrhoUIPingPongPan()
    : UI(),
      fAboutWindow(this)
{
    // set UI size
    setSize(DistrhoArtworkPingPongPan::backgroundWidth, DistrhoArtworkPingPongPan::backgroundHeight);

    // background
    fImgBackground = Image(DistrhoArtworkPingPongPan::backgroundData, DistrhoArtworkPingPongPan::backgroundWidth, DistrhoArtworkPingPongPan::backgroundHeight, GL_BGR);

    Image imageAbout(DistrhoArtworkPingPongPan::aboutData, DistrhoArtworkPingPongPan::aboutWidth, DistrhoArtworkPingPongPan::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // knobs
    Image knobImage(DistrhoArtworkPingPongPan::knobData, DistrhoArtworkPingPongPan::knobWidth, DistrhoArtworkPingPongPan::knobHeight);

    // knob Low-Mid
    fKnobFreq = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobFreq->setId(DistrhoPluginPingPongPan::paramFreq);
    fKnobFreq->setAbsolutePos(60, 58);
    fKnobFreq->setRange(0.0f, 100.0f);
    fKnobFreq->setDefault(50.0f);
    fKnobFreq->setRotationAngle(270);
    fKnobFreq->setCallback(this);

    // knob Mid-High
    fKnobWidth = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobWidth->setId(DistrhoPluginPingPongPan::paramWidth);
    fKnobWidth->setAbsolutePos(182, 58);
    fKnobWidth->setRange(0.0f, 100.0f);
    fKnobWidth->setDefault(75.0f);
    fKnobWidth->setRotationAngle(270);
    fKnobWidth->setCallback(this);

    // about button
    Image aboutImageNormal(DistrhoArtworkPingPongPan::aboutButtonNormalData, DistrhoArtworkPingPongPan::aboutButtonNormalWidth, DistrhoArtworkPingPongPan::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtworkPingPongPan::aboutButtonHoverData, DistrhoArtworkPingPongPan::aboutButtonHoverWidth, DistrhoArtworkPingPongPan::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(183, 8);
    fButtonAbout->setCallback(this);

    // set default values
    d_programChanged(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUIPingPongPan::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case DistrhoPluginPingPongPan::paramFreq:
        fKnobFreq->setValue(value);
        break;
    case DistrhoPluginPingPongPan::paramWidth:
        fKnobWidth->setValue(value);
        break;
    }
}

void DistrhoUIPingPongPan::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobFreq->setValue(50.0f);
    fKnobWidth->setValue(75.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUIPingPongPan::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void DistrhoUIPingPongPan::imageKnobDragStarted(ImageKnob* knob)
{
    d_editParameter(knob->getId(), true);
}

void DistrhoUIPingPongPan::imageKnobDragFinished(ImageKnob* knob)
{
    d_editParameter(knob->getId(), false);
}

void DistrhoUIPingPongPan::imageKnobValueChanged(ImageKnob* knob, float value)
{
    d_setParameterValue(knob->getId(), value);
}

void DistrhoUIPingPongPan::onDisplay()
{
    fImgBackground.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUIPingPongPan();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
