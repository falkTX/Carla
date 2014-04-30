/*
 * ZaMultiComp mono multiband compressor 
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

#include "ZaMultiCompUI.hpp"
#include <stdio.h>

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZaMultiCompUI::ZaMultiCompUI()
    : UI()
{
    // background
    fImgBackground = Image(ZaMultiCompArtwork::zamulticompData, ZaMultiCompArtwork::zamulticompWidth, ZaMultiCompArtwork::zamulticompHeight, GL_BGR);

    // led images
    fLedRedImg = Image(ZaMultiCompArtwork::ledredData, ZaMultiCompArtwork::ledredWidth, ZaMultiCompArtwork::ledredHeight);
    fLedYellowImg = Image(ZaMultiCompArtwork::ledyellowData, ZaMultiCompArtwork::ledyellowWidth, ZaMultiCompArtwork::ledyellowHeight);

    // led values
    fLedRedValue1 = 0.0f;
    fLedRedValue2 = 0.0f;
    fLedRedValue3 = 0.0f;
    fLedYellowValue = 0.0f; 

    // knob
    Image knobImage(ZaMultiCompArtwork::knobData, ZaMultiCompArtwork::knobWidth, ZaMultiCompArtwork::knobHeight);

    // knob 
    fKnobAttack = new ImageKnob(this, knobImage);
    fKnobAttack->setPos(24, 43);
    fKnobAttack->setRange(0.1f, 200.0f);
    fKnobAttack->setStep(0.1f);
    //fKnobAttack->setLogScale(true);
    //fKnobAttack->setDefault(10.0f);
    fKnobAttack->setRotationAngle(240);
    fKnobAttack->setCallback(this);

    fKnobRelease = new ImageKnob(this, knobImage);
    fKnobRelease->setPos(108, 43);
    fKnobRelease->setRange(50.0f, 500.0f);
    fKnobRelease->setStep(1.0f);
    //fKnobRelease->setDefault(80.0f);
    fKnobRelease->setRotationAngle(240);
    fKnobRelease->setCallback(this);

    fKnobThresh = new ImageKnob(this, knobImage);
    fKnobThresh->setPos(191.5, 43);
    fKnobThresh->setRange(-60.0f, 0.0f);
    fKnobThresh->setStep(1.0f);
    //fKnobThresh->setDefault(0.0f);
    fKnobThresh->setRotationAngle(240);
    fKnobThresh->setCallback(this);

    fKnobRatio = new ImageKnob(this, knobImage);
    fKnobRatio->setPos(270, 43);
    fKnobRatio->setRange(1.0f, 20.0f);
    fKnobRatio->setStep(0.1f);
    //fKnobRatio->setDefault(4.0f);
    fKnobRatio->setRotationAngle(240);
    fKnobRatio->setCallback(this);

    fKnobKnee = new ImageKnob(this, knobImage);
    fKnobKnee->setPos(348.5, 43);
    fKnobKnee->setRange(0.0f, 8.0f);
    fKnobKnee->setStep(0.1f);
    //fKnobKnee->setDefault(0.0f);
    fKnobKnee->setRotationAngle(240);
    fKnobKnee->setCallback(this);

    fKnobGlobalGain = new ImageKnob(this, knobImage);
    fKnobGlobalGain->setPos(427.3, 43);
    fKnobGlobalGain->setRange(-30.0f, 30.0f);
    fKnobGlobalGain->setStep(1.0f);
    //fKnobGlobalGain->setDefault(0.0f);
    fKnobGlobalGain->setRotationAngle(240);
    fKnobGlobalGain->setCallback(this);

    fKnobXover2 = new ImageKnob(this, knobImage);
    fKnobXover2->setPos(84, 121);
    fKnobXover2->setRange(1400.f, 14000.f);
    fKnobXover2->setStep(1.0f);
    //fKnobXover2->setLogScale(true);
    //fKnobXover2->setDefault(1400.f);
    fKnobXover2->setRotationAngle(240);
    fKnobXover2->setCallback(this);

    fKnobXover1 = new ImageKnob(this, knobImage);
    fKnobXover1->setPos(84, 176);
    fKnobXover1->setRange(20.0f, 1400.0f);
    fKnobXover1->setStep(1.0f);
    //fKnobXover1->setLogScale(true);
    //fKnobXover1->setDefault(250.0f);
    fKnobXover1->setRotationAngle(240);
    fKnobXover1->setCallback(this);

    fKnobMakeup3 = new ImageKnob(this, knobImage);
    fKnobMakeup3->setPos(167.75, 99.5);
    fKnobMakeup3->setRange(0.0f, 30.0f);
    fKnobMakeup3->setStep(0.1f);
    //fKnobMakeup3->setDefault(0.0f);
    fKnobMakeup3->setRotationAngle(240);
    fKnobMakeup3->setCallback(this);

    fKnobMakeup2 = new ImageKnob(this, knobImage);
    fKnobMakeup2->setPos(167.75, 150.25);
    fKnobMakeup2->setRange(0.0f, 30.0f);
    fKnobMakeup2->setStep(0.1f);
    //fKnobMakeup2->setDefault(0.0f);
    fKnobMakeup2->setRotationAngle(240);
    fKnobMakeup2->setCallback(this);

    fKnobMakeup1 = new ImageKnob(this, knobImage);
    fKnobMakeup1->setPos(167.75, 201.4);
    fKnobMakeup1->setRange(0.0f, 30.0f);
    fKnobMakeup1->setStep(0.1f);
    //fKnobMakeup1->setDefault(0.0f);
    fKnobMakeup1->setRotationAngle(240);
    fKnobMakeup1->setCallback(this);

    Image toggleonImage(ZaMultiCompArtwork::toggleonData, ZaMultiCompArtwork::toggleonWidth, ZaMultiCompArtwork::toggleonHeight);
    Image toggleoffImage(ZaMultiCompArtwork::toggleoffData, ZaMultiCompArtwork::toggleoffWidth, ZaMultiCompArtwork::toggleoffHeight);
    Image toggleonhImage(ZaMultiCompArtwork::toggleonhorizData, ZaMultiCompArtwork::toggleonhorizWidth, ZaMultiCompArtwork::toggleonhorizHeight);
    Image toggleoffhImage(ZaMultiCompArtwork::toggleoffhorizData, ZaMultiCompArtwork::toggleoffhorizWidth, ZaMultiCompArtwork::toggleoffhorizHeight);

    Point<int> togglePosStart(247,109);

    fToggleBypass3 = new ImageToggle(this, toggleoffImage, toggleoffImage, toggleonImage);
    fToggleBypass3->setPos(togglePosStart);
    fToggleBypass3->setCallback(this);

    togglePosStart.setY(158);

    fToggleBypass2 = new ImageToggle(this, toggleoffImage, toggleoffImage, toggleonImage);
    fToggleBypass2->setPos(togglePosStart);
    fToggleBypass2->setCallback(this);

    togglePosStart.setY(209);

    fToggleBypass1 = new ImageToggle(this, toggleoffImage, toggleoffImage, toggleonImage);
    fToggleBypass1->setPos(togglePosStart);
    fToggleBypass1->setCallback(this);

    togglePosStart.setX(278);
    togglePosStart.setY(113);

    fToggleListen3 = new ImageToggle(this,  toggleoffhImage, toggleoffhImage, toggleonhImage);
    fToggleListen3->setPos(togglePosStart);
    fToggleListen3->setCallback(this);

    togglePosStart.setY(164);

    fToggleListen2 = new ImageToggle(this, toggleoffhImage, toggleoffhImage, toggleonhImage); 
    fToggleListen2->setPos(togglePosStart);
    fToggleListen2->setCallback(this);

    togglePosStart.setY(214);

    fToggleListen1 = new ImageToggle(this, toggleoffhImage, toggleoffhImage, toggleonhImage);
    fToggleListen1->setPos(togglePosStart);
    fToggleListen1->setCallback(this);
}

ZaMultiCompUI::~ZaMultiCompUI()
{
    delete fKnobAttack;
    delete fKnobRelease;
    delete fKnobThresh;
    delete fKnobRatio;
    delete fKnobKnee;
    delete fKnobGlobalGain;
    delete fKnobMakeup1;
    delete fKnobMakeup2;
    delete fKnobMakeup3;
    delete fKnobXover1;
    delete fKnobXover2;
    delete fToggleBypass1;
    delete fToggleBypass2;
    delete fToggleBypass3;
    delete fToggleListen1;
    delete fToggleListen2;
    delete fToggleListen3;
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZaMultiCompUI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case ZaMultiCompPlugin::paramAttack:
        fKnobAttack->setValue(value);
        break;
    case ZaMultiCompPlugin::paramRelease:
        fKnobRelease->setValue(value);
        break;
    case ZaMultiCompPlugin::paramThresh:
        fKnobThresh->setValue(value);
        break;
    case ZaMultiCompPlugin::paramRatio:
        fKnobRatio->setValue(value);
        break;
    case ZaMultiCompPlugin::paramKnee:
        fKnobKnee->setValue(value);
        break;
    case ZaMultiCompPlugin::paramGlobalGain:
        fKnobGlobalGain->setValue(value);
        break;
    case ZaMultiCompPlugin::paramGainR1:
        if (fLedRedValue1 != value)
        {
            fLedRedValue1 = value;
            repaint();
        }
        break;
    case ZaMultiCompPlugin::paramGainR2:
        if (fLedRedValue2 != value)
        {
            fLedRedValue2 = value;
            repaint();
        }
        break;
    case ZaMultiCompPlugin::paramGainR3:
        if (fLedRedValue3 != value)
        {
            fLedRedValue3 = value;
            repaint();
        }
        break;
    case ZaMultiCompPlugin::paramOutputLevel:
        if (fLedYellowValue != value)
        {
            fLedYellowValue = value;
            repaint();
        }
        break;
    case ZaMultiCompPlugin::paramMakeup1:
        fKnobMakeup1->setValue(value);
        break;
    case ZaMultiCompPlugin::paramMakeup2:
        fKnobMakeup2->setValue(value);
        break;
    case ZaMultiCompPlugin::paramMakeup3:
        fKnobMakeup3->setValue(value);
        break;
    case ZaMultiCompPlugin::paramToggle1:
        //fToggleBypass1->setValue(value);
        break;
    case ZaMultiCompPlugin::paramToggle2:
        //fToggleBypass2->setValue(value);
        break;
    case ZaMultiCompPlugin::paramToggle3:
        //fToggleBypass3->setValue(value);
        break;
    case ZaMultiCompPlugin::paramListen1:
        //fToggleListen1->setValue(value);
        break;
    case ZaMultiCompPlugin::paramListen2:
        //fToggleListen2->setValue(value);
        break;
    case ZaMultiCompPlugin::paramListen3:
        //fToggleListen3->setValue(value);
        break;
    }
}

void ZaMultiCompUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobAttack->setValue(10.0f);
    fKnobRelease->setValue(80.0f);
    fKnobThresh->setValue(0.0f);
    fKnobRatio->setValue(4.0f);
    fKnobKnee->setValue(0.0f);
    fKnobGlobalGain->setValue(0.0f);
    fKnobMakeup1->setValue(0.0f);
    fKnobMakeup2->setValue(0.0f);
    fKnobMakeup3->setValue(0.0f);
    fKnobXover1->setValue(250.0f);
    fKnobXover2->setValue(1400.0f);
    fToggleBypass1->setValue(1.0f);
    fToggleBypass2->setValue(1.0f);
    fToggleBypass3->setValue(1.0f);
    fToggleListen1->setValue(1.0f);
    fToggleListen2->setValue(1.0f);
    fToggleListen3->setValue(1.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZaMultiCompUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZaMultiCompPlugin::paramAttack, true);
    else if (knob == fKnobRelease)
        d_editParameter(ZaMultiCompPlugin::paramRelease, true);
    else if (knob == fKnobThresh)
        d_editParameter(ZaMultiCompPlugin::paramThresh, true);
    else if (knob == fKnobRatio)
        d_editParameter(ZaMultiCompPlugin::paramRatio, true);
    else if (knob == fKnobKnee)
        d_editParameter(ZaMultiCompPlugin::paramKnee, true);
    else if (knob == fKnobGlobalGain)
        d_editParameter(ZaMultiCompPlugin::paramGlobalGain, true);
    else if (knob == fKnobMakeup1)
        d_editParameter(ZaMultiCompPlugin::paramMakeup1, true);
    else if (knob == fKnobMakeup2)
        d_editParameter(ZaMultiCompPlugin::paramMakeup2, true);
    else if (knob == fKnobMakeup3)
        d_editParameter(ZaMultiCompPlugin::paramMakeup3, true);
    else if (knob == fKnobXover1)
        d_editParameter(ZaMultiCompPlugin::paramXover1, true);
    else if (knob == fKnobXover2)
        d_editParameter(ZaMultiCompPlugin::paramXover2, true);
}

void ZaMultiCompUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZaMultiCompPlugin::paramAttack, false);
    else if (knob == fKnobRelease)
        d_editParameter(ZaMultiCompPlugin::paramRelease, false);
    else if (knob == fKnobThresh)
        d_editParameter(ZaMultiCompPlugin::paramThresh, false);
    else if (knob == fKnobRatio)
        d_editParameter(ZaMultiCompPlugin::paramRatio, false);
    else if (knob == fKnobKnee)
        d_editParameter(ZaMultiCompPlugin::paramKnee, false);
    else if (knob == fKnobGlobalGain)
        d_editParameter(ZaMultiCompPlugin::paramGlobalGain, false);
    else if (knob == fKnobMakeup1)
        d_editParameter(ZaMultiCompPlugin::paramMakeup1, false);
    else if (knob == fKnobMakeup2)
        d_editParameter(ZaMultiCompPlugin::paramMakeup2, false);
    else if (knob == fKnobMakeup3)
        d_editParameter(ZaMultiCompPlugin::paramMakeup3, false);
    else if (knob == fKnobXover1)
        d_editParameter(ZaMultiCompPlugin::paramXover1, false);
    else if (knob == fKnobXover2)
        d_editParameter(ZaMultiCompPlugin::paramXover2, false);
}

void ZaMultiCompUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobAttack)
        d_setParameterValue(ZaMultiCompPlugin::paramAttack, value);
    else if (knob == fKnobRelease)
        d_setParameterValue(ZaMultiCompPlugin::paramRelease, value);
    else if (knob == fKnobThresh)
        d_setParameterValue(ZaMultiCompPlugin::paramThresh, value);
    else if (knob == fKnobRatio)
        d_setParameterValue(ZaMultiCompPlugin::paramRatio, value);
    else if (knob == fKnobKnee)
        d_setParameterValue(ZaMultiCompPlugin::paramKnee, value);
    else if (knob == fKnobGlobalGain)
        d_setParameterValue(ZaMultiCompPlugin::paramGlobalGain, value);
    else if (knob == fKnobMakeup1)
        d_setParameterValue(ZaMultiCompPlugin::paramMakeup1, value);
    else if (knob == fKnobMakeup2)
        d_setParameterValue(ZaMultiCompPlugin::paramMakeup2, value);
    else if (knob == fKnobMakeup3)
        d_setParameterValue(ZaMultiCompPlugin::paramMakeup3, value);
    else if (knob == fKnobXover1)
        d_setParameterValue(ZaMultiCompPlugin::paramXover1, value);
    else if (knob == fKnobXover2)
        d_setParameterValue(ZaMultiCompPlugin::paramXover2, value);
}

void ZaMultiCompUI::imageToggleClicked(ImageToggle* toggle, int)
{
    float v = toggle->getValue();
    if (toggle == fToggleBypass1)
        d_setParameterValue(ZaMultiCompPlugin::paramToggle1, v);
    else if (toggle == fToggleBypass2)
        d_setParameterValue(ZaMultiCompPlugin::paramToggle2, v);
    else if (toggle == fToggleBypass3)
        d_setParameterValue(ZaMultiCompPlugin::paramToggle3, v);
    else if (toggle == fToggleListen1)
        d_setParameterValue(ZaMultiCompPlugin::paramListen1, v);
    else if (toggle == fToggleListen2)
        d_setParameterValue(ZaMultiCompPlugin::paramListen2, v);
    else if (toggle == fToggleListen3)
        d_setParameterValue(ZaMultiCompPlugin::paramListen3, v);
}

void ZaMultiCompUI::onDisplay()
{
    fImgBackground.draw();

    // draw leds
    static const float sLedSpacing  = 15.5f;
    static const int   sLedInitialX = 343;

    static const int sYellowLedStaticY = 265;
    static const int sRedLed1StaticY    = 215;
    static const int sRedLed2StaticY    = 164;
    static const int sRedLed3StaticY    = 113;

    int numRedLeds1;
    int numRedLeds2;
    int numRedLeds3;
    int numYellowLeds;

	if (fLedRedValue1 >= 40.f)
		numRedLeds1 = 12;
	else if (fLedRedValue1 >= 30.f)
		numRedLeds1 = 11;
	else if (fLedRedValue1 >= 20.f)
		numRedLeds1 = 10;
	else if (fLedRedValue1 >= 15.f)
		numRedLeds1 = 9;
	else if (fLedRedValue1 >= 10.f)
		numRedLeds1 = 8;
	else if (fLedRedValue1 >= 8.f)
		numRedLeds1 = 7;
	else if (fLedRedValue1 >= 6.f)
		numRedLeds1 = 6;
	else if (fLedRedValue1 >= 5.f)
		numRedLeds1 = 5;
	else if (fLedRedValue1 >= 4.f)
		numRedLeds1 = 4;
	else if (fLedRedValue1 >= 3.f)
		numRedLeds1 = 3;
	else if (fLedRedValue1 >= 2.f)
		numRedLeds1 = 2;
	else if (fLedRedValue1 >= 1.f)
		numRedLeds1 = 1;
	else numRedLeds1 = 0;

	if (fLedRedValue2 >= 40.f)
		numRedLeds2 = 12;
	else if (fLedRedValue2 >= 30.f)
		numRedLeds2 = 11;
	else if (fLedRedValue2 >= 20.f)
		numRedLeds2 = 10;
	else if (fLedRedValue2 >= 15.f)
		numRedLeds2 = 9;
	else if (fLedRedValue2 >= 10.f)
		numRedLeds2 = 8;
	else if (fLedRedValue2 >= 8.f)
		numRedLeds2 = 7;
	else if (fLedRedValue2 >= 6.f)
		numRedLeds2 = 6;
	else if (fLedRedValue2 >= 5.f)
		numRedLeds2 = 5;
	else if (fLedRedValue2 >= 4.f)
		numRedLeds2 = 4;
	else if (fLedRedValue2 >= 3.f)
		numRedLeds2 = 3;
	else if (fLedRedValue2 >= 2.f)
		numRedLeds2 = 2;
	else if (fLedRedValue2 >= 1.f)
		numRedLeds2 = 1;
	else numRedLeds2 = 0;

	if (fLedRedValue3 >= 40.f)
		numRedLeds3 = 12;
	else if (fLedRedValue3 >= 30.f)
		numRedLeds3 = 11;
	else if (fLedRedValue3 >= 20.f)
		numRedLeds3 = 10;
	else if (fLedRedValue3 >= 15.f)
		numRedLeds3 = 9;
	else if (fLedRedValue3 >= 10.f)
		numRedLeds3 = 8;
	else if (fLedRedValue3 >= 8.f)
		numRedLeds3 = 7;
	else if (fLedRedValue3 >= 6.f)
		numRedLeds3 = 6;
	else if (fLedRedValue3 >= 5.f)
		numRedLeds3 = 5;
	else if (fLedRedValue3 >= 4.f)
		numRedLeds3 = 4;
	else if (fLedRedValue3 >= 3.f)
		numRedLeds3 = 3;
	else if (fLedRedValue3 >= 2.f)
		numRedLeds3 = 2;
	else if (fLedRedValue3 >= 1.f)
		numRedLeds3 = 1;
	else numRedLeds3 = 0;

    for (int i=numRedLeds1; i>0; --i)
        fLedRedImg.draw(sLedInitialX + (12 - i)*sLedSpacing, sRedLed1StaticY);

    for (int i=numRedLeds2; i>0; --i)
        fLedRedImg.draw(sLedInitialX + (12 - i)*sLedSpacing, sRedLed2StaticY);

    for (int i=numRedLeds3; i>0; --i)
        fLedRedImg.draw(sLedInitialX + (12 - i)*sLedSpacing, sRedLed3StaticY);

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
    return new ZaMultiCompUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
