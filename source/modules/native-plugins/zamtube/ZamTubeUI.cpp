/*
 * ZamTube triode WDF distortion model
 * Copyright (C) 2014  Damien Zammit <damien@zamaudio.com>
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

#include "ZamTubePlugin.hpp"

// conflict macros in wdf
#undef max
#undef min

#include "ZamTubeUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamTubeUI::ZamTubeUI()
    : UI()
{
    // background
    fImgBackground = Image(ZamTubeArtwork::zamtubeData, ZamTubeArtwork::zamtubeWidth, ZamTubeArtwork::zamtubeHeight, GL_BGR);

    // knob
    Image knobImage(ZamTubeArtwork::knobData, ZamTubeArtwork::knobWidth, ZamTubeArtwork::knobHeight);

    // notch slider
    Image notchImage(ZamTubeArtwork::notchData, ZamTubeArtwork::notchWidth, ZamTubeArtwork::notchHeight);

    // knobs
    fKnobTube = new ImageKnob(this, knobImage);
    fKnobTube->setAbsolutePos(177, 76);
    fKnobTube->setRange(0.f, 30.0f);
    fKnobTube->setStep(0.1f);
    fKnobTube->setDefault(0.0f);
    fKnobTube->setRotationAngle(240);
    fKnobTube->setCallback(this);

    fKnobBass = new ImageKnob(this, knobImage);
    fKnobBass->setAbsolutePos(63, 140.5);
    fKnobBass->setRange(0.f, 1.0f);
    fKnobBass->setStep(0.01f);
    fKnobBass->setDefault(0.5f);
    fKnobBass->setRotationAngle(240);
    fKnobBass->setCallback(this);

    fKnobMids = new ImageKnob(this, knobImage);
    fKnobMids->setAbsolutePos(63, 87);
    fKnobMids->setRange(0.f, 1.0f);
    fKnobMids->setStep(0.01f);
    fKnobMids->setDefault(0.5f);
    fKnobMids->setRotationAngle(240);
    fKnobMids->setCallback(this);

    fKnobTreb = new ImageKnob(this, knobImage);
    fKnobTreb->setAbsolutePos(63, 33);
    fKnobTreb->setRange(0.f, 1.0f);
    fKnobTreb->setStep(0.01f);
    fKnobTreb->setDefault(0.0f);
    fKnobTreb->setRotationAngle(240);
    fKnobTreb->setCallback(this);

    fKnobGain = new ImageKnob(this, knobImage);
    fKnobGain->setAbsolutePos(63, 231);
    fKnobGain->setRange(-30.f, 30.0f);
    fKnobGain->setStep(1.f);
    fKnobGain->setDefault(0.0f);
    fKnobGain->setRotationAngle(240);
    fKnobGain->setCallback(this);

    Point<int> notchPosStart(289,5);
    Point<int> notchPosEnd(289,5+280);

    fSliderNotch = new ImageSlider(this, notchImage);
    fSliderNotch->setStartPos(notchPosStart);
    fSliderNotch->setEndPos(notchPosEnd);
    fSliderNotch->setRange(0.f, 24.f);
    fSliderNotch->setStep(1.f);
    fSliderNotch->setValue(0.f);
    fSliderNotch->setCallback(this);

    // set default values
    d_programChanged(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZamTubeUI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case ZamTubePlugin::paramTubedrive:
        fKnobTube->setValue(value);
        break;
    case ZamTubePlugin::paramBass:
        fKnobBass->setValue(value);
        break;
    case ZamTubePlugin::paramMiddle:
        fKnobMids->setValue(value);
        break;
    case ZamTubePlugin::paramTreble:
        fKnobTreb->setValue(value);
        break;
    case ZamTubePlugin::paramGain:
        fKnobGain->setValue(value);
        break;
    }
}

void ZamTubeUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobTube->setValue(0.0f);
    fKnobBass->setValue(0.5f);
    fKnobMids->setValue(0.5f);
    fKnobTreb->setValue(0.0f);
    fKnobGain->setValue(0.0f);
    fSliderNotch->setValue(0.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZamTubeUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobTube)
        d_editParameter(ZamTubePlugin::paramTubedrive, true);
    else if (knob == fKnobBass)
        d_editParameter(ZamTubePlugin::paramBass, true);
    else if (knob == fKnobMids)
        d_editParameter(ZamTubePlugin::paramMiddle, true);
    else if (knob == fKnobTreb)
        d_editParameter(ZamTubePlugin::paramTreble, true);
    else if (knob == fKnobGain)
        d_editParameter(ZamTubePlugin::paramGain, true);
}

void ZamTubeUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobTube)
        d_editParameter(ZamTubePlugin::paramTubedrive, false);
    else if (knob == fKnobBass)
        d_editParameter(ZamTubePlugin::paramBass, false);
    else if (knob == fKnobMids)
        d_editParameter(ZamTubePlugin::paramMiddle, false);
    else if (knob == fKnobTreb)
        d_editParameter(ZamTubePlugin::paramTreble, false);
    else if (knob == fKnobGain)
        d_editParameter(ZamTubePlugin::paramGain, false);
}

void ZamTubeUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobTube)
        d_setParameterValue(ZamTubePlugin::paramTubedrive, value);
    else if (knob == fKnobBass)
        d_setParameterValue(ZamTubePlugin::paramBass, value);
    else if (knob == fKnobMids)
        d_setParameterValue(ZamTubePlugin::paramMiddle, value);
    else if (knob == fKnobTreb)
        d_setParameterValue(ZamTubePlugin::paramTreble, value);
    else if (knob == fKnobGain)
        d_setParameterValue(ZamTubePlugin::paramGain, value);
}

void ZamTubeUI::imageSliderDragStarted(ImageSlider*)
{
    d_editParameter(ZamTubePlugin::paramToneStack, true);
}

void ZamTubeUI::imageSliderDragFinished(ImageSlider*)
{
    d_editParameter(ZamTubePlugin::paramToneStack, false);
}

void ZamTubeUI::imageSliderValueChanged(ImageSlider*, float value)
{
    d_setParameterValue(ZamTubePlugin::paramToneStack, value);
}

void ZamTubeUI::onDisplay()
{
    fImgBackground.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new ZamTubeUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
