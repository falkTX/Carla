/*
 * DISTRHO StereoEnhancer Plugin, based on StereoEnhancer by Michael Gruhn
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

#include "DistrhoUIStereoEnhancer.hpp"

#include "dgl/ImageAboutWindow.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

DistrhoUIStereoEnhancer::DistrhoUIStereoEnhancer()
    : OpenGLUI(),
      fAboutWindow(this)
{
    // background
    fImgBackground = Image(DistrhoArtworkStereoEnhancer::backgroundData, DistrhoArtworkStereoEnhancer::backgroundWidth, DistrhoArtworkStereoEnhancer::backgroundHeight, GL_BGR);

    Image imageAbout(DistrhoArtworkStereoEnhancer::aboutData, DistrhoArtworkStereoEnhancer::aboutWidth, DistrhoArtworkStereoEnhancer::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // knobs
    Image knobImage(DistrhoArtworkStereoEnhancer::knobData, DistrhoArtworkStereoEnhancer::knobWidth, DistrhoArtworkStereoEnhancer::knobHeight);

    fKnobWidthLows = new ImageKnob(this, knobImage);
    fKnobWidthLows->setPos(140, 35);
    fKnobWidthLows->setRange(0.0f, 200.0f);
    fKnobWidthLows->setValue(100.0f);
    fKnobWidthLows->setCallback(this);

    fKnobWidthHighs = new ImageKnob(this, knobImage);
    fKnobWidthHighs->setPos(362, 35);
    fKnobWidthHighs->setRange(0.0f, 200.0f);
    fKnobWidthHighs->setValue(100.0f);
    fKnobWidthHighs->setCallback(this);

    fKnobCrossover = new ImageKnob(this, knobImage);
    fKnobCrossover->setPos(253, 35);
    fKnobCrossover->setRange(0.0f, 100.0f);
    fKnobCrossover->setValue(27.51604f);
    fKnobCrossover->setCallback(this);

    // about button
    Image aboutImageNormal(DistrhoArtworkStereoEnhancer::aboutButtonNormalData, DistrhoArtworkStereoEnhancer::aboutButtonNormalWidth, DistrhoArtworkStereoEnhancer::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtworkStereoEnhancer::aboutButtonHoverData, DistrhoArtworkStereoEnhancer::aboutButtonHoverWidth, DistrhoArtworkStereoEnhancer::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(346, 3);
    fButtonAbout->setCallback(this);
}

DistrhoUIStereoEnhancer::~DistrhoUIStereoEnhancer()
{
    delete fKnobWidthLows;
    delete fKnobWidthHighs;
    delete fKnobCrossover;
    delete fButtonAbout;
}

// -------------------------------------------------
// DSP Callbacks

void DistrhoUIStereoEnhancer::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case DistrhoPluginStereoEnhancer::paramWidthLows:
        fKnobWidthLows->setValue(value);
        break;
    case DistrhoPluginStereoEnhancer::paramWidthHighs:
        fKnobWidthHighs->setValue(value);
        break;
    case DistrhoPluginStereoEnhancer::paramCrossover:
        fKnobCrossover->setValue(value);
        break;
    }
}

void DistrhoUIStereoEnhancer::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobWidthLows->setValue(100.0f);
    fKnobWidthHighs->setValue(100.0f);
    fKnobCrossover->setValue(27.51604f);
}

// -------------------------------------------------
// Widget Callbacks

void DistrhoUIStereoEnhancer::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void DistrhoUIStereoEnhancer::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobWidthLows)
        d_editParameter(DistrhoPluginStereoEnhancer::paramWidthLows, true);
    else if (knob == fKnobWidthHighs)
        d_editParameter(DistrhoPluginStereoEnhancer::paramWidthHighs, true);
    else if (knob == fKnobCrossover)
        d_editParameter(DistrhoPluginStereoEnhancer::paramCrossover, true);
}

void DistrhoUIStereoEnhancer::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobWidthLows)
        d_editParameter(DistrhoPluginStereoEnhancer::paramWidthLows, false);
    else if (knob == fKnobWidthHighs)
        d_editParameter(DistrhoPluginStereoEnhancer::paramWidthHighs, false);
    else if (knob == fKnobCrossover)
        d_editParameter(DistrhoPluginStereoEnhancer::paramCrossover, false);
}

void DistrhoUIStereoEnhancer::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobWidthLows)
        d_setParameterValue(DistrhoPluginStereoEnhancer::paramWidthLows, value);
    else if (knob == fKnobWidthHighs)
        d_setParameterValue(DistrhoPluginStereoEnhancer::paramWidthHighs, value);
    else if (knob == fKnobCrossover)
        d_setParameterValue(DistrhoPluginStereoEnhancer::paramCrossover, value);
}

void DistrhoUIStereoEnhancer::onDisplay()
{
    fImgBackground.draw();
}

// -------------------------------------------------

UI* createUI()
{
    return new DistrhoUIStereoEnhancer();
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
