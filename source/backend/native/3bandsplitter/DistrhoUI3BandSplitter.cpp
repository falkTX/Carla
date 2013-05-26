/*
 * DISTRHO 3BandSplitter Plugin, based on 3BandSplitter by Michael Gruhn
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

#include "DistrhoUI3BandSplitter.hpp"

#include "dgl/ImageAboutWindow.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

DistrhoUI3BandSplitter::DistrhoUI3BandSplitter()
    : OpenGLUI()
{
    Window* win = getParent();

    // background
    fImgBackground = Image(DistrhoArtwork3BandSplitter::backgroundData, DistrhoArtwork3BandSplitter::backgroundWidth, DistrhoArtwork3BandSplitter::backgroundHeight, GL_BGR);

    // sliders
    Image sliderImage(DistrhoArtwork3BandSplitter::sliderData, DistrhoArtwork3BandSplitter::sliderWidth, DistrhoArtwork3BandSplitter::sliderHeight);
    Point<int> sliderPosStart(57, 43);
    Point<int> sliderPosEnd(57, 43 + 160);

    // slider Low
    fSliderLow = new ImageSlider(win, sliderImage);
    fSliderLow->setStartPos(sliderPosStart);
    fSliderLow->setEndPos(sliderPosEnd);
    fSliderLow->setRange(-24.0f, 24.0f);
    fSliderLow->setValue(0.0f);
    fSliderLow->setCallback(this);

    // slider Mid
    sliderPosStart.setX(120);
    sliderPosEnd.setX(120);
    fSliderMid = new ImageSlider(*fSliderLow);
    fSliderMid->setStartPos(sliderPosStart);
    fSliderMid->setEndPos(sliderPosEnd);

    // slider High
    sliderPosStart.setX(183);
    sliderPosEnd.setX(183);
    fSliderHigh = new ImageSlider(*fSliderLow);
    fSliderHigh->setStartPos(sliderPosStart);
    fSliderHigh->setEndPos(sliderPosEnd);

    // slider Master
    sliderPosStart.setX(287);
    sliderPosEnd.setX(287);
    fSliderMaster = new ImageSlider(*fSliderLow);
    fSliderMaster->setStartPos(sliderPosStart);
    fSliderMaster->setEndPos(sliderPosEnd);

    // knobs
    Image knobImage(DistrhoArtwork3BandSplitter::knobData, DistrhoArtwork3BandSplitter::knobWidth, DistrhoArtwork3BandSplitter::knobHeight);

    // knob Low-Mid
    fKnobLowMid = new ImageKnob(win, knobImage);
    fKnobLowMid->setPos(66, 270);
    fKnobLowMid->setRange(0.0f, 1000.0f);
    fKnobLowMid->setValue(220.0f);
    fKnobLowMid->setCallback(this);

    // knob Mid-High
    fKnobMidHigh = new ImageKnob(win, knobImage);
    fKnobMidHigh->setPos(160, 270);
    fKnobMidHigh->setRange(1000.0f, 20000.0f);
    fKnobMidHigh->setValue(2000.0f);
    fKnobMidHigh->setCallback(this);

    // about button
    Image aboutImageNormal(DistrhoArtwork3BandSplitter::aboutButtonNormalData, DistrhoArtwork3BandSplitter::aboutButtonNormalWidth, DistrhoArtwork3BandSplitter::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtwork3BandSplitter::aboutButtonHoverData, DistrhoArtwork3BandSplitter::aboutButtonHoverWidth, DistrhoArtwork3BandSplitter::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(win, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(264, 300);
    fButtonAbout->setCallback(this);
}

DistrhoUI3BandSplitter::~DistrhoUI3BandSplitter()
{
    delete fSliderLow;
    delete fSliderMid;
    delete fSliderHigh;
    delete fSliderMaster;
    delete fKnobLowMid;
    delete fKnobMidHigh;
    delete fButtonAbout;
}

// -------------------------------------------------
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

// -------------------------------------------------
// Widget Callbacks

void DistrhoUI3BandSplitter::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    Image imageAbout(DistrhoArtwork3BandSplitter::aboutData, DistrhoArtwork3BandSplitter::aboutWidth, DistrhoArtwork3BandSplitter::aboutHeight, GL_BGR);
    ImageAboutWindow aboutWindow(getApp(), getParent(), imageAbout);
    aboutWindow.exec();
}

void DistrhoUI3BandSplitter::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobLowMid)
        d_editParameter(DistrhoPlugin3BandSplitter::paramLowMidFreq, true);
    else if (knob == fKnobMidHigh)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMidHighFreq, true);
}

void DistrhoUI3BandSplitter::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobLowMid)
        d_editParameter(DistrhoPlugin3BandSplitter::paramLowMidFreq, false);
    else if (knob == fKnobMidHigh)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMidHighFreq, false);
}


void DistrhoUI3BandSplitter::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobLowMid)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramLowMidFreq, value);
    else if (knob == fKnobMidHigh)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramMidHighFreq, value);
}

void DistrhoUI3BandSplitter::imageSliderDragStarted(ImageSlider* slider)
{
    if (slider == fSliderLow)
        d_editParameter(DistrhoPlugin3BandSplitter::paramLow, true);
    else if (slider == fSliderMid)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMid, true);
    else if (slider == fSliderHigh)
        d_editParameter(DistrhoPlugin3BandSplitter::paramHigh, true);
    else if (slider == fSliderMaster)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMaster, true);
}

void DistrhoUI3BandSplitter::imageSliderDragFinished(ImageSlider* slider)
{
    if (slider == fSliderLow)
        d_editParameter(DistrhoPlugin3BandSplitter::paramLow, false);
    else if (slider == fSliderMid)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMid, false);
    else if (slider == fSliderHigh)
        d_editParameter(DistrhoPlugin3BandSplitter::paramHigh, false);
    else if (slider == fSliderMaster)
        d_editParameter(DistrhoPlugin3BandSplitter::paramMaster, false);
}

void DistrhoUI3BandSplitter::imageSliderValueChanged(ImageSlider* slider, float value)
{
    if (slider == fSliderLow)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramLow, value);
    else if (slider == fSliderMid)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramMid, value);
    else if (slider == fSliderHigh)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramHigh, value);
    else if (slider == fSliderMaster)
        d_setParameterValue(DistrhoPlugin3BandSplitter::paramMaster, value);
}

void DistrhoUI3BandSplitter::onDisplay()
{
    fImgBackground.draw();
}

// -------------------------------------------------

UI* createUI()
{
    return new DistrhoUI3BandSplitter();
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
