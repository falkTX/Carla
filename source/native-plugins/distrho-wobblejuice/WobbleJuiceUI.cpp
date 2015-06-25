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

#include "WobbleJuicePlugin.hpp"
#include "WobbleJuiceUI.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

WobbleJuiceUI::WobbleJuiceUI()
    : UI(),
      fAboutWindow(this)
{
    setSize(WobbleJuiceArtwork::backgroundWidth, WobbleJuiceArtwork::backgroundHeight);

    // background
    fImgBackground = Image(WobbleJuiceArtwork::backgroundData, WobbleJuiceArtwork::backgroundWidth, WobbleJuiceArtwork::backgroundHeight, GL_BGR);

    // about
    Image aboutImage(WobbleJuiceArtwork::aboutData, WobbleJuiceArtwork::aboutWidth, WobbleJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // knobs
    Image knobImage(WobbleJuiceArtwork::knobData, WobbleJuiceArtwork::knobWidth, WobbleJuiceArtwork::knobHeight);

    // knob Division
    fKnobDivision = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobDivision->setId(WobbleJuicePlugin::paramDivision);
    fKnobDivision->setAbsolutePos(222, 74);
    fKnobDivision->setRotationAngle(270);
    fKnobDivision->setRange(1.0f, 16.0f);
    fKnobDivision->setDefault(4.0f);
    fKnobDivision->setStep(1.0f);
    fKnobDivision->setCallback(this);

    // knob Resonance
    fKnobResonance = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobResonance->setId(WobbleJuicePlugin::paramReso);
    fKnobResonance->setAbsolutePos(222, 199);
    fKnobResonance->setRotationAngle(270);
    fKnobResonance->setRange(0.0f, 0.2f);
    fKnobResonance->setDefault(0.1f);
    fKnobResonance->setCallback(this);

    // knob Range
    fKnobRange = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobRange->setId(WobbleJuicePlugin::paramRange);
    fKnobRange->setAbsolutePos(77, 199);
    fKnobRange->setRotationAngle(270);
    fKnobRange->setRange(500.0f, 16000.0f);
    fKnobRange->setDefault(16000.0f);
    fKnobRange->setCallback(this);

    // knob Phase
    fKnobPhase = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobPhase->setId(WobbleJuicePlugin::paramPhase);
    fKnobPhase->setAbsolutePos(362, 74);
    fKnobPhase->setRotationAngle(270);
    fKnobPhase->setRange(-1.0f, 1.0f);
    fKnobPhase->setDefault(0.0f);
    fKnobPhase->setCallback(this);

    // knob Wave
    fKnobWave = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobWave->setId(WobbleJuicePlugin::paramWave);
    fKnobWave->setAbsolutePos(77, 74);
    fKnobWave->setRotationAngle(270);
    fKnobWave->setRange(1.0f, 4.0f);
    fKnobWave->setDefault(2.0f);
    fKnobWave->setCallback(this);

    // knob Drive
    fKnobDrive = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobDrive->setId(WobbleJuicePlugin::paramDrive);
    fKnobDrive->setAbsolutePos(362, 199);
    fKnobDrive->setRotationAngle(270);
    fKnobDrive->setRange(0.0f, 1.0f);
    fKnobDrive->setDefault(0.5f);
    fKnobDrive->setCallback(this);

    // about button
    Image aboutImageNormal(WobbleJuiceArtwork::aboutButtonNormalData, WobbleJuiceArtwork::aboutButtonNormalWidth, WobbleJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(WobbleJuiceArtwork::aboutButtonHoverData, WobbleJuiceArtwork::aboutButtonHoverWidth, WobbleJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(390, 20);
    fButtonAbout->setCallback(this);

    // set default values
    programLoaded(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void WobbleJuiceUI::parameterChanged(uint32_t index, float value)
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

void WobbleJuiceUI::programLoaded(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobDivision->setValue(4.0f);
    fKnobResonance->setValue(0.1f);
    fKnobRange->setValue(16000.0f);
    fKnobPhase->setValue(0.0f);
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
    editParameter(knob->getId(), true);
}

void WobbleJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void WobbleJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);
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
