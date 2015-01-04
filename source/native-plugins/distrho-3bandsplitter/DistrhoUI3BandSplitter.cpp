/*
 * DISTRHO 3BandSplitter Plugin, based on 3BandSplitter by Michael Gruhn
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

#include "DistrhoPlugin3BandSplitter.hpp"
#include "DistrhoUI3BandSplitter.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoUI3BandSplitter::DistrhoUI3BandSplitter()
    : UI(),
      fAboutWindow(this)
{
    // set UI size
    setSize(DistrhoArtwork3BandSplitter::backgroundWidth, DistrhoArtwork3BandSplitter::backgroundHeight);

    // background
    fImgBackground = Image(DistrhoArtwork3BandSplitter::backgroundData, DistrhoArtwork3BandSplitter::backgroundWidth, DistrhoArtwork3BandSplitter::backgroundHeight, GL_BGR);

    // about
    Image aboutImage(DistrhoArtwork3BandSplitter::aboutData, DistrhoArtwork3BandSplitter::aboutWidth, DistrhoArtwork3BandSplitter::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // sliders
    Image sliderImage(DistrhoArtwork3BandSplitter::sliderData, DistrhoArtwork3BandSplitter::sliderWidth, DistrhoArtwork3BandSplitter::sliderHeight);
    Point<int> sliderPosStart(57, 43);
    Point<int> sliderPosEnd(57, 43 + 160);

    // slider Low
    fSliderLow = new ImageSlider(this, sliderImage);
    fSliderLow->setId(DistrhoPlugin3BandSplitter::paramLow);
    fSliderLow->setInverted(true);
    fSliderLow->setStartPos(sliderPosStart);
    fSliderLow->setEndPos(sliderPosEnd);
    fSliderLow->setRange(-24.0f, 24.0f);
    fSliderLow->setCallback(this);

    // slider Mid
    sliderPosStart.setX(120);
    sliderPosEnd.setX(120);
    fSliderMid = new ImageSlider(*fSliderLow);
    fSliderMid->setId(DistrhoPlugin3BandSplitter::paramMid);
    fSliderMid->setStartPos(sliderPosStart);
    fSliderMid->setEndPos(sliderPosEnd);

    // slider High
    sliderPosStart.setX(183);
    sliderPosEnd.setX(183);
    fSliderHigh = new ImageSlider(*fSliderLow);
    fSliderHigh->setId(DistrhoPlugin3BandSplitter::paramHigh);
    fSliderHigh->setStartPos(sliderPosStart);
    fSliderHigh->setEndPos(sliderPosEnd);

    // slider Master
    sliderPosStart.setX(287);
    sliderPosEnd.setX(287);
    fSliderMaster = new ImageSlider(*fSliderLow);
    fSliderMaster->setId(DistrhoPlugin3BandSplitter::paramMaster);
    fSliderMaster->setStartPos(sliderPosStart);
    fSliderMaster->setEndPos(sliderPosEnd);

    // knobs
    Image knobImage(DistrhoArtwork3BandSplitter::knobData, DistrhoArtwork3BandSplitter::knobWidth, DistrhoArtwork3BandSplitter::knobHeight);

    // knob Low-Mid
    fKnobLowMid = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPlugin3BandSplitter::paramLowMidFreq);
    fKnobLowMid->setAbsolutePos(65, 269);
    fKnobLowMid->setRange(0.0f, 1000.0f);
    fKnobLowMid->setDefault(440.0f);
    fKnobLowMid->setRotationAngle(270);
    fKnobLowMid->setCallback(this);

    // knob Mid-High
    fKnobMidHigh = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPlugin3BandSplitter::paramMidHighFreq);
    fKnobMidHigh->setAbsolutePos(159, 269);
    fKnobMidHigh->setRange(1000.0f, 20000.0f);
    fKnobMidHigh->setDefault(1000.0f);
    fKnobMidHigh->setRotationAngle(270);
    fKnobMidHigh->setCallback(this);

    // about button
    Image aboutImageNormal(DistrhoArtwork3BandSplitter::aboutButtonNormalData, DistrhoArtwork3BandSplitter::aboutButtonNormalWidth, DistrhoArtwork3BandSplitter::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtwork3BandSplitter::aboutButtonHoverData, DistrhoArtwork3BandSplitter::aboutButtonHoverWidth, DistrhoArtwork3BandSplitter::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(264, 300);
    fButtonAbout->setCallback(this);

    // set default values
    d_programChanged(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUI3BandSplitter::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case DistrhoPlugin3BandSplitter::paramLow:
        fSliderLow->setValue(value);
        break;
    case DistrhoPlugin3BandSplitter::paramMid:
        fSliderMid->setValue(value);
        break;
    case DistrhoPlugin3BandSplitter::paramHigh:
        fSliderHigh->setValue(value);
        break;
    case DistrhoPlugin3BandSplitter::paramMaster:
        fSliderMaster->setValue(value);
        break;
    case DistrhoPlugin3BandSplitter::paramLowMidFreq:
        fKnobLowMid->setValue(value);
        break;
    case DistrhoPlugin3BandSplitter::paramMidHighFreq:
        fKnobMidHigh->setValue(value);
        break;
    }
}

void DistrhoUI3BandSplitter::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fSliderLow->setValue(0.0f);
    fSliderMid->setValue(0.0f);
    fSliderHigh->setValue(0.0f);
    fSliderMaster->setValue(0.0f);
    fKnobLowMid->setValue(220.0f);
    fKnobMidHigh->setValue(2000.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUI3BandSplitter::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void DistrhoUI3BandSplitter::imageKnobDragStarted(ImageKnob* knob)
{
    d_editParameter(knob->getId(), true);
}

void DistrhoUI3BandSplitter::imageKnobDragFinished(ImageKnob* knob)
{
    d_editParameter(knob->getId(), false);
}

void DistrhoUI3BandSplitter::imageKnobValueChanged(ImageKnob* knob, float value)
{
    d_setParameterValue(knob->getId(), value);
}

void DistrhoUI3BandSplitter::imageSliderDragStarted(ImageSlider* slider)
{
    d_editParameter(slider->getId(), true);
}

void DistrhoUI3BandSplitter::imageSliderDragFinished(ImageSlider* slider)
{
    d_editParameter(slider->getId(), false);
}

void DistrhoUI3BandSplitter::imageSliderValueChanged(ImageSlider* slider, float value)
{
    d_setParameterValue(slider->getId(), value);
}

void DistrhoUI3BandSplitter::onDisplay()
{
    fImgBackground.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUI3BandSplitter();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
