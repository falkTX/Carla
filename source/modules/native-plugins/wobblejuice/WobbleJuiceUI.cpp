/*
 * Wobble Juice Plugin
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "WobbleJuiceUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

WobbleJuiceUI::WobbleJuiceUI()
    : UI(),
      fAboutWindow(this)
{
    // background
    fImgBackground = Image(WobbleJuiceArtwork::backgroundData, WobbleJuiceArtwork::backgroundWidth, WobbleJuiceArtwork::backgroundHeight, GL_BGR);

    // about
    Image imageAbout(WobbleJuiceArtwork::aboutData, WobbleJuiceArtwork::aboutWidth, WobbleJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // knobs
    Image knobImage(WobbleJuiceArtwork::knobData, WobbleJuiceArtwork::knobWidth, WobbleJuiceArtwork::knobHeight);

    // knob Division
    fKnobDivision = new ImageKnob(this, knobImage);
    fKnobDivision->setPos(222, 74);
    fKnobDivision->setRange(1.0f, 16.0f);
    fKnobDivision->setStep(1.0f);
    fKnobDivision->setValue(4.0f);
    fKnobDivision->setRotationAngle(270);
    fKnobDivision->setCallback(this);

    // knob Resonance
    fKnobResonance = new ImageKnob(this, knobImage);
    fKnobResonance->setPos(222, 199);
    fKnobResonance->setRange(0.0f, 0.2f);
    fKnobResonance->setValue(0.1f);
    fKnobResonance->setRotationAngle(270);
    fKnobResonance->setCallback(this);

    // knob Range
    fKnobRange = new ImageKnob(this, knobImage);
    fKnobRange->setPos(77, 199);
    fKnobRange->setRange(500.0f, 16000.0f);
    fKnobRange->setValue(16000.0f);
    fKnobRange->setRotationAngle(270);
    fKnobRange->setCallback(this);

    // knob Wave
    fKnobWave = new ImageKnob(this, knobImage);
    fKnobWave->setPos(77, 74);
    fKnobWave->setRange(1.0f, 4.0f);
    fKnobWave->setValue(2.0f);
    fKnobWave->setRotationAngle(270);
    fKnobWave->setCallback(this);

    // knob Drive
    fKnobDrive = new ImageKnob(this, knobImage);
    fKnobDrive->setPos(362, 199);
    fKnobDrive->setRange(0.0f, 1.0f);
    fKnobDrive->setValue(0.5f);
    fKnobDrive->setRotationAngle(270);
    fKnobDrive->setCallback(this);

    // knob Phase
    fKnobPhase = new ImageKnob(this, knobImage);
    fKnobPhase->setPos(362, 74);
    fKnobPhase->setRange(-1.0f, 1.0f);
    fKnobPhase->setValue(0.0f);
    fKnobPhase->setRotationAngle(270);
    fKnobPhase->setCallback(this);

    // about button
    Image aboutImageNormal(WobbleJuiceArtwork::aboutButtonNormalData, WobbleJuiceArtwork::aboutButtonNormalWidth, WobbleJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(WobbleJuiceArtwork::aboutButtonHoverData, WobbleJuiceArtwork::aboutButtonHoverWidth, WobbleJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(390, 20);
    fButtonAbout->setCallback(this);
}

WobbleJuiceUI::~WobbleJuiceUI()
{
    delete fKnobDivision;
    delete fKnobResonance;
    delete fKnobRange;
    delete fKnobWave;
    delete fKnobDrive;
    delete fKnobPhase;
    delete fButtonAbout;
}

// -----------------------------------------------------------------------
// DSP Callbacks

void WobbleJuiceUI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case WobbleJuicePlugin::paramDivision:
        fKnobDivision->setValue(value);
        break;
    case WobbleJuicePlugin::paramReso:
        fKnobResonance->setValue(value);
        break;
    case WobbleJuicePlugin::paramRange:
        fKnobRange->setValue(value);
        break;
    case WobbleJuicePlugin::paramPhase:
        fKnobPhase->setValue(value);
        break;
    case WobbleJuicePlugin::paramWave:
        fKnobWave->setValue(value);
        break;
    case WobbleJuicePlugin::paramDrive:
        fKnobDrive->setValue(value);
        break;
    }
}

void WobbleJuiceUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobDivision->setValue(4.0f);
    fKnobResonance->setValue(0.1f);
    fKnobRange->setValue(16000.0f);
    fKnobPhase->setValue(.0f);
    fKnobWave->setValue(2.0f);
    fKnobDrive->setValue(0.5f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void WobbleJuiceUI::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void WobbleJuiceUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobDivision)
        d_editParameter(WobbleJuicePlugin::paramDivision, true);
    else if (knob == fKnobResonance)
        d_editParameter(WobbleJuicePlugin::paramReso, true);
    else if (knob == fKnobRange)
        d_editParameter(WobbleJuicePlugin::paramRange, true);
    else if (knob == fKnobPhase)
        d_editParameter(WobbleJuicePlugin::paramPhase, true);
    else if (knob == fKnobWave)
        d_editParameter(WobbleJuicePlugin::paramWave, true);
    else if (knob == fKnobDrive)
        d_editParameter(WobbleJuicePlugin::paramDrive, true);
}

void WobbleJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobDivision)
        d_editParameter(WobbleJuicePlugin::paramDivision, false);
    else if (knob == fKnobResonance)
        d_editParameter(WobbleJuicePlugin::paramReso, false);
    else if (knob == fKnobRange)
        d_editParameter(WobbleJuicePlugin::paramRange, false);
    else if (knob == fKnobPhase)
        d_editParameter(WobbleJuicePlugin::paramPhase, false);
    else if (knob == fKnobWave)
        d_editParameter(WobbleJuicePlugin::paramWave, false);
    else if (knob == fKnobDrive)
        d_editParameter(WobbleJuicePlugin::paramDrive, false);
}

void WobbleJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobDivision)
        d_setParameterValue(WobbleJuicePlugin::paramDivision, value);
    else if (knob == fKnobResonance)
        d_setParameterValue(WobbleJuicePlugin::paramReso, value);
    else if (knob == fKnobRange)
        d_setParameterValue(WobbleJuicePlugin::paramRange, value);
    else if (knob == fKnobPhase)
        d_setParameterValue(WobbleJuicePlugin::paramPhase, value);
    else if (knob == fKnobWave)
        d_setParameterValue(WobbleJuicePlugin::paramWave, value);
    else if (knob == fKnobDrive)
        d_setParameterValue(WobbleJuicePlugin::paramDrive, value);
}

void WobbleJuiceUI::onDisplay()
{
    fImgBackground.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new WobbleJuiceUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
