/*
 * DISTRHO 3BandSplitter Plugin, based on 3BandSplitter by Michael Gruhn
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

START_NAMESPACE_DISTRHO

namespace Art = DistrhoArtwork3BandSplitter;

// -----------------------------------------------------------------------

DistrhoUI3BandSplitter::DistrhoUI3BandSplitter()
    : UI(Art::backgroundWidth, Art::backgroundHeight),
      fImgBackground(Art::backgroundData, Art::backgroundWidth, Art::backgroundHeight, GL_BGR),
      fAboutWindow(this)
{
    // about
    Image aboutImage(Art::aboutData, Art::aboutWidth, Art::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // sliders
    Image sliderImage(Art::sliderData, Art::sliderWidth, Art::sliderHeight);
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
    fSliderMid = new ImageSlider(this, sliderImage);
    fSliderMid->setId(DistrhoPlugin3BandSplitter::paramMid);
    fSliderMid->setInverted(true);
    fSliderMid->setStartPos(sliderPosStart);
    fSliderMid->setEndPos(sliderPosEnd);
    fSliderMid->setRange(-24.0f, 24.0f);
    fSliderMid->setCallback(this);

    // slider High
    sliderPosStart.setX(183);
    sliderPosEnd.setX(183);
    fSliderHigh = new ImageSlider(this, sliderImage);
    fSliderHigh->setId(DistrhoPlugin3BandSplitter::paramHigh);
    fSliderHigh->setInverted(true);
    fSliderHigh->setStartPos(sliderPosStart);
    fSliderHigh->setEndPos(sliderPosEnd);
    fSliderHigh->setRange(-24.0f, 24.0f);
    fSliderHigh->setCallback(this);

    // slider Master
    sliderPosStart.setX(287);
    sliderPosEnd.setX(287);
    fSliderMaster = new ImageSlider(this, sliderImage);
    fSliderMaster->setId(DistrhoPlugin3BandSplitter::paramMaster);
    fSliderMaster->setInverted(true);
    fSliderMaster->setStartPos(sliderPosStart);
    fSliderMaster->setEndPos(sliderPosEnd);
    fSliderMaster->setRange(-24.0f, 24.0f);
    fSliderMaster->setCallback(this);

    // knobs
    Image knobImage(Art::knobData, Art::knobWidth, Art::knobHeight);

    // knob Low-Mid
    fKnobLowMid = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobLowMid->setId(DistrhoPlugin3BandSplitter::paramLowMidFreq);
    fKnobLowMid->setAbsolutePos(65, 269);
    fKnobLowMid->setRange(0.0f, 1000.0f);
    fKnobLowMid->setDefault(440.0f);
    fKnobLowMid->setRotationAngle(270);
    fKnobLowMid->setCallback(this);

    // knob Mid-High
    fKnobMidHigh = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobMidHigh->setId(DistrhoPlugin3BandSplitter::paramMidHighFreq);
    fKnobMidHigh->setAbsolutePos(159, 269);
    fKnobMidHigh->setRange(1000.0f, 20000.0f);
    fKnobMidHigh->setDefault(1000.0f);
    fKnobMidHigh->setRotationAngle(270);
    fKnobMidHigh->setCallback(this);

    // about button
    Image aboutImageNormal(Art::aboutButtonNormalData, Art::aboutButtonNormalWidth, Art::aboutButtonNormalHeight);
    Image aboutImageHover(Art::aboutButtonHoverData, Art::aboutButtonHoverWidth, Art::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(264, 300);
    fButtonAbout->setCallback(this);

    // set default values
    programLoaded(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUI3BandSplitter::parameterChanged(uint32_t index, float value)
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

void DistrhoUI3BandSplitter::programLoaded(uint32_t index)
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
    editParameter(knob->getId(), true);
}

void DistrhoUI3BandSplitter::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void DistrhoUI3BandSplitter::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);
}

void DistrhoUI3BandSplitter::imageSliderDragStarted(ImageSlider* slider)
{
    editParameter(slider->getId(), true);
}

void DistrhoUI3BandSplitter::imageSliderDragFinished(ImageSlider* slider)
{
    editParameter(slider->getId(), false);
}

void DistrhoUI3BandSplitter::imageSliderValueChanged(ImageSlider* slider, float value)
{
    setParameterValue(slider->getId(), value);
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
