/*
 * ZamCompX2 Stereo compressor 
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

#include "ZamCompX2UI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamCompX2UI::ZamCompX2UI()
    : UI()
{
    // background
    fImgBackground = Image(ZamCompX2Artwork::zamcompx2Data, ZamCompX2Artwork::zamcompx2Width, ZamCompX2Artwork::zamcompx2Height, GL_BGR);

    // led images
    fLedRedImg = Image(ZamCompX2Artwork::ledredData, ZamCompX2Artwork::ledredWidth, ZamCompX2Artwork::ledredHeight);
    fLedYellowImg = Image(ZamCompX2Artwork::ledyellowData, ZamCompX2Artwork::ledyellowWidth, ZamCompX2Artwork::ledyellowHeight);

    // led values
    fLedRedValue = 0.0f;
    fLedYellowValue = 0.0f; 

    // knob
    Image knobImage(ZamCompX2Artwork::knobData, ZamCompX2Artwork::knobWidth, ZamCompX2Artwork::knobHeight);

    // knob 
    fKnobAttack = new ImageKnob(this, knobImage);
    fKnobAttack->setPos(24, 45);
    fKnobAttack->setRange(0.1f, 200.0f);
    fKnobAttack->setStep(0.1f);
    //fKnobAttack->setLogScale(true);
    //fKnobAttack->setDefault(10.0f);
    fKnobAttack->setRotationAngle(240);
    fKnobAttack->setCallback(this);

    fKnobRelease = new ImageKnob(this, knobImage);
    fKnobRelease->setPos(108, 45);
    fKnobRelease->setRange(50.0f, 500.0f);
    fKnobRelease->setStep(1.0f);
    //fKnobRelease->setDefault(80.0f);
    fKnobRelease->setRotationAngle(240);
    fKnobRelease->setCallback(this);

    fKnobThresh = new ImageKnob(this, knobImage);
    fKnobThresh->setPos(191.5, 45);
    fKnobThresh->setRange(-60.0f, 0.0f);
    fKnobThresh->setStep(1.0f);
    //fKnobThresh->setDefault(0.0f);
    fKnobThresh->setRotationAngle(240);
    fKnobThresh->setCallback(this);

    fKnobRatio = new ImageKnob(this, knobImage);
    fKnobRatio->setPos(270, 45);
    fKnobRatio->setRange(1.0f, 20.0f);
    fKnobRatio->setStep(0.1f);
    //fKnobRatio->setDefault(4.0f);
    fKnobRatio->setRotationAngle(240);
    fKnobRatio->setCallback(this);

    fKnobKnee = new ImageKnob(this, knobImage);
    fKnobKnee->setPos(348.5, 45);
    fKnobKnee->setRange(0.0f, 8.0f);
    fKnobKnee->setStep(0.1f);
    //fKnobKnee->setDefault(0.0f);
    fKnobKnee->setRotationAngle(240);
    fKnobKnee->setCallback(this);

    fKnobMakeup = new ImageKnob(this, knobImage);
    fKnobMakeup->setPos(427.3, 45);
    fKnobMakeup->setRange(-30.0f, 30.0f);
    fKnobMakeup->setStep(1.0f);
    //fKnobMakeup->setDefault(0.0f);
    fKnobMakeup->setRotationAngle(240);
    fKnobMakeup->setCallback(this);

    Image toggleonImage(ZamCompX2Artwork::toggleonData, ZamCompX2Artwork::toggleonWidth, ZamCompX2Artwork::toggleonHeight);
    Image toggleoffImage(ZamCompX2Artwork::toggleoffData, ZamCompX2Artwork::toggleoffWidth, ZamCompX2Artwork::toggleoffHeight);

    Point<int> togglePosStart(652,72);

    fToggleStereo = new ImageToggle(this, toggleoffImage, toggleoffImage, toggleonImage);
    fToggleStereo->setPos(togglePosStart);
    fToggleStereo->setCallback(this);

}

ZamCompX2UI::~ZamCompX2UI()
{
    delete fKnobAttack;
    delete fKnobRelease;
    delete fKnobThresh;
    delete fKnobRatio;
    delete fKnobKnee;
    delete fKnobMakeup;
    delete fToggleStereo;
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZamCompX2UI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case ZamCompX2Plugin::paramAttack:
        fKnobAttack->setValue(value);
        break;
    case ZamCompX2Plugin::paramRelease:
        fKnobRelease->setValue(value);
        break;
    case ZamCompX2Plugin::paramThresh:
        fKnobThresh->setValue(value);
        break;
    case ZamCompX2Plugin::paramRatio:
        fKnobRatio->setValue(value);
        break;
    case ZamCompX2Plugin::paramKnee:
        fKnobKnee->setValue(value);
        break;
    case ZamCompX2Plugin::paramMakeup:
        fKnobMakeup->setValue(value);
        break;
    case ZamCompX2Plugin::paramGainRed:
        if (fLedRedValue != value)
        {
            fLedRedValue = value;
            repaint();
        }
        break;
    case ZamCompX2Plugin::paramOutputLevel:
        if (fLedYellowValue != value)
        {
            fLedYellowValue = value;
            repaint();
        }
        break;
    case ZamCompX2Plugin::paramStereo:
        //fToggleStereo->setValue((int)value);
	break;
    }
}

void ZamCompX2UI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    /*
    fKnobAttack->setValue(10.0f);
    fKnobRelease->setValue(80.0f);
    fKnobThresh->setValue(0.0f);
    fKnobRatio->setValue(4.0f);
    fKnobKnee->setValue(0.0f);
    fKnobMakeup->setValue(0.0f);
    */
    //fToggleStereo->setValue(1.f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZamCompX2UI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZamCompX2Plugin::paramAttack, true);
    else if (knob == fKnobRelease)
        d_editParameter(ZamCompX2Plugin::paramRelease, true);
    else if (knob == fKnobThresh)
        d_editParameter(ZamCompX2Plugin::paramThresh, true);
    else if (knob == fKnobRatio)
        d_editParameter(ZamCompX2Plugin::paramRatio, true);
    else if (knob == fKnobKnee)
        d_editParameter(ZamCompX2Plugin::paramKnee, true);
    else if (knob == fKnobMakeup)
        d_editParameter(ZamCompX2Plugin::paramMakeup, true);
}

void ZamCompX2UI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZamCompX2Plugin::paramAttack, false);
    else if (knob == fKnobRelease)
        d_editParameter(ZamCompX2Plugin::paramRelease, false);
    else if (knob == fKnobThresh)
        d_editParameter(ZamCompX2Plugin::paramThresh, false);
    else if (knob == fKnobRatio)
        d_editParameter(ZamCompX2Plugin::paramRatio, false);
    else if (knob == fKnobKnee)
        d_editParameter(ZamCompX2Plugin::paramKnee, false);
    else if (knob == fKnobMakeup)
        d_editParameter(ZamCompX2Plugin::paramMakeup, false);
}

void ZamCompX2UI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobAttack)
        d_setParameterValue(ZamCompX2Plugin::paramAttack, value);
    else if (knob == fKnobRelease)
        d_setParameterValue(ZamCompX2Plugin::paramRelease, value);
    else if (knob == fKnobThresh)
        d_setParameterValue(ZamCompX2Plugin::paramThresh, value);
    else if (knob == fKnobRatio)
        d_setParameterValue(ZamCompX2Plugin::paramRatio, value);
    else if (knob == fKnobKnee)
        d_setParameterValue(ZamCompX2Plugin::paramKnee, value);
    else if (knob == fKnobMakeup)
        d_setParameterValue(ZamCompX2Plugin::paramMakeup, value);
}

void ZamCompX2UI::imageToggleClicked(ImageToggle *toggle, int)
{
    int flip = !toggle->getValue();
    if (toggle == fToggleStereo) {
        d_setParameterValue(ZamCompX2Plugin::paramStereo, flip);
    }
}

void ZamCompX2UI::onDisplay()
{
    fImgBackground.draw();

    // draw leds
    static const float sLedSpacing  = 15.5f;
    static const int   sLedInitialX = 498;

    static const int sYellowLedStaticY = 16;
    static const int sRedLedStaticY    = 45;

    int numRedLeds;
    int numYellowLeds;

	if (fLedRedValue >= 40.f)
		numRedLeds = 12;
	else if (fLedRedValue >= 30.f)
		numRedLeds = 11;
	else if (fLedRedValue >= 20.f)
		numRedLeds = 10;
	else if (fLedRedValue >= 15.f)
		numRedLeds = 9;
	else if (fLedRedValue >= 10.f)
		numRedLeds = 8;
	else if (fLedRedValue >= 8.f)
		numRedLeds = 7;
	else if (fLedRedValue >= 6.f)
		numRedLeds = 6;
	else if (fLedRedValue >= 5.f)
		numRedLeds = 5;
	else if (fLedRedValue >= 4.f)
		numRedLeds = 4;
	else if (fLedRedValue >= 3.f)
		numRedLeds = 3;
	else if (fLedRedValue >= 2.f)
		numRedLeds = 2;
	else if (fLedRedValue >= 1.f)
		numRedLeds = 1;
	else numRedLeds = 0;

    for (int i=numRedLeds; i>0; --i)
        fLedRedImg.draw(sLedInitialX + (12 - i)*sLedSpacing, sRedLedStaticY);

	if (fLedYellowValue >= 20.f)
		numYellowLeds = 19;
	else if (fLedYellowValue >= 10.f)
		numYellowLeds = 18;
	else if (fLedYellowValue >= 8.f)
		numYellowLeds = 17;
	else if (fLedYellowValue >= 4.f)
		numYellowLeds = 16;
	else if (fLedYellowValue >= 2.f)
		numYellowLeds = 15;
	else if (fLedYellowValue >= 1.f)
		numYellowLeds = 14;
	else if (fLedYellowValue >= 0.f)
		numYellowLeds = 13;
	else if (fLedYellowValue >= -1.f)
		numYellowLeds = 12;
	else if (fLedYellowValue >= -2.f)
		numYellowLeds = 11;
	else if (fLedYellowValue >= -3.f)
		numYellowLeds = 10;
	else if (fLedYellowValue >= -4.f)
		numYellowLeds = 9;
	else if (fLedYellowValue >= -5.f)
		numYellowLeds = 8;
	else if (fLedYellowValue >= -6.f)
		numYellowLeds = 7;
	else if (fLedYellowValue >= -8.f)
		numYellowLeds = 6;
	else if (fLedYellowValue >= -10.f)
		numYellowLeds = 5;
	else if (fLedYellowValue >= -15.f)
		numYellowLeds = 4;
	else if (fLedYellowValue >= -20.f)
		numYellowLeds = 3;
	else if (fLedYellowValue >= -30.f)
		numYellowLeds = 2;
	else if (fLedYellowValue >= -40.f)
		numYellowLeds = 1;
	else numYellowLeds = 0;

	if (numYellowLeds > 12) {
		for (int i=12; i<numYellowLeds; ++i)
			fLedRedImg.draw(sLedInitialX + i*sLedSpacing, sYellowLedStaticY);
		for (int i=0; i<12; ++i)
			fLedYellowImg.draw(sLedInitialX + i*sLedSpacing, sYellowLedStaticY);
	} else {
		for (int i=0; i<numYellowLeds; ++i)
			fLedYellowImg.draw(sLedInitialX + i*sLedSpacing, sYellowLedStaticY);
	}
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new ZamCompX2UI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
