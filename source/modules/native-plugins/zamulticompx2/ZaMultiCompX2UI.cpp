/*
 * ZaMultiCompX2  Stereo multiband compressor
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

#include "ZaMultiCompX2Plugin.hpp"
#include "ZaMultiCompX2UI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZaMultiCompX2UI::ZaMultiCompX2UI()
    : UI()
{
    // background
    fImgBackground = Image(ZaMultiCompX2Artwork::zamulticompx2Data, ZaMultiCompX2Artwork::zamulticompx2Width, ZaMultiCompX2Artwork::zamulticompx2Height, GL_BGR);

    // led images
    fLedRedImg = Image(ZaMultiCompX2Artwork::ledredData, ZaMultiCompX2Artwork::ledredWidth, ZaMultiCompX2Artwork::ledredHeight);
    fLedYellowImg = Image(ZaMultiCompX2Artwork::ledyellowData, ZaMultiCompX2Artwork::ledyellowWidth, ZaMultiCompX2Artwork::ledyellowHeight);

    // led values
    fLedRedValue1 = 0.0f;
    fLedRedValue2 = 0.0f;
    fLedRedValue3 = 0.0f;
    fLedYellowValueL = 0.0f;
    fLedYellowValueR = 0.0f;

    // knob
    Image knobImage(ZaMultiCompX2Artwork::knobData, ZaMultiCompX2Artwork::knobWidth, ZaMultiCompX2Artwork::knobHeight);

    // knob
    fKnobAttack = new ImageKnob(this, knobImage);
    fKnobAttack->setAbsolutePos(24, 43);
    fKnobAttack->setRange(0.1f, 200.0f);
    fKnobAttack->setUsingLogScale(true);
    fKnobAttack->setDefault(10.0f);
    fKnobAttack->setStep(0.1f);
    fKnobAttack->setRotationAngle(240);
    fKnobAttack->setCallback(this);

    fKnobRelease = new ImageKnob(this, knobImage);
    fKnobRelease->setAbsolutePos(108, 43);
    fKnobRelease->setRange(50.0f, 500.0f);
    fKnobRelease->setDefault(80.0f);
    fKnobRelease->setRotationAngle(240);
    fKnobRelease->setCallback(this);

    fKnobThresh3 = new ImageKnob(this, knobImage);
    fKnobThresh3->setAbsolutePos(94.5, 99);
    fKnobThresh3->setRange(-60.0f, 0.0f);
    fKnobThresh3->setDefault(-16.0f);
    fKnobThresh3->setRotationAngle(240);
    fKnobThresh3->setCallback(this);

    fKnobThresh2 = new ImageKnob(this, knobImage);
    fKnobThresh2->setAbsolutePos(94.5, 150);
    fKnobThresh2->setRange(-60.0f, 0.0f);
    fKnobThresh2->setDefault(-18.0f);
    fKnobThresh2->setRotationAngle(240);
    fKnobThresh2->setCallback(this);

    fKnobThresh1 = new ImageKnob(this, knobImage);
    fKnobThresh1->setAbsolutePos(94.5, 201);
    fKnobThresh1->setRange(-60.0f, 0.0f);
    fKnobThresh1->setDefault(-20.0f);
    fKnobThresh1->setRotationAngle(240);
    fKnobThresh1->setCallback(this);

    fKnobRatio = new ImageKnob(this, knobImage);
    fKnobRatio->setAbsolutePos(191.5, 43);
    fKnobRatio->setRange(1.0f, 20.0f);
    fKnobRatio->setDefault(4.0f);
    fKnobRatio->setRotationAngle(240);
    fKnobRatio->setCallback(this);

    fKnobKnee = new ImageKnob(this, knobImage);
    fKnobKnee->setAbsolutePos(273, 43);
    fKnobKnee->setRange(0.0f, 8.0f);
    fKnobKnee->setDefault(0.0f);
    fKnobKnee->setRotationAngle(240);
    fKnobKnee->setCallback(this);

    fKnobGlobalGain = new ImageKnob(this, knobImage);
    fKnobGlobalGain->setAbsolutePos(427.3, 43);
    fKnobGlobalGain->setRange(-30.0f, 30.0f);
    fKnobGlobalGain->setDefault(0.0f);
    fKnobGlobalGain->setRotationAngle(240);
    fKnobGlobalGain->setCallback(this);

    fKnobXover2 = new ImageKnob(this, knobImage);
    fKnobXover2->setAbsolutePos(23, 121);
    fKnobXover2->setRange(1400.f, 14000.f);
    fKnobXover2->setUsingLogScale(true);
    fKnobXover2->setDefault(1400.f);
    fKnobXover2->setStep(5.f);
    fKnobXover2->setRotationAngle(240);
    fKnobXover2->setCallback(this);

    fKnobXover1 = new ImageKnob(this, knobImage);
    fKnobXover1->setAbsolutePos(23, 175.5);
    fKnobXover1->setRange(20.0f, 1400.0f);
    fKnobXover1->setUsingLogScale(true);
    fKnobXover1->setDefault(250.0f);
    fKnobXover1->setStep(1.f);
    fKnobXover1->setRotationAngle(240);
    fKnobXover1->setCallback(this);

    fKnobMakeup3 = new ImageKnob(this, knobImage);
    fKnobMakeup3->setAbsolutePos(167.75, 99.5);
    fKnobMakeup3->setRange(0.0f, 30.0f);
    fKnobMakeup3->setDefault(0.0f);
    fKnobMakeup3->setRotationAngle(240);
    fKnobMakeup3->setCallback(this);

    fKnobMakeup2 = new ImageKnob(this, knobImage);
    fKnobMakeup2->setAbsolutePos(167.75, 150.25);
    fKnobMakeup2->setRange(0.0f, 30.0f);
    fKnobMakeup2->setDefault(0.0f);
    fKnobMakeup2->setRotationAngle(240);
    fKnobMakeup2->setCallback(this);

    fKnobMakeup1 = new ImageKnob(this, knobImage);
    fKnobMakeup1->setAbsolutePos(167.75, 201.4);
    fKnobMakeup1->setRange(0.0f, 30.0f);
    fKnobMakeup1->setDefault(0.0f);
    fKnobMakeup1->setRotationAngle(240);
    fKnobMakeup1->setCallback(this);

    Image toggleonImage(ZaMultiCompX2Artwork::toggleonData, ZaMultiCompX2Artwork::toggleonWidth, ZaMultiCompX2Artwork::toggleonHeight);
    Image toggleoffImage(ZaMultiCompX2Artwork::toggleoffData, ZaMultiCompX2Artwork::toggleoffWidth, ZaMultiCompX2Artwork::toggleoffHeight);
    Image toggleonhImage(ZaMultiCompX2Artwork::toggleonhorizData, ZaMultiCompX2Artwork::toggleonhorizWidth, ZaMultiCompX2Artwork::toggleonhorizHeight);
    Image toggleoffhImage(ZaMultiCompX2Artwork::toggleoffhorizData, ZaMultiCompX2Artwork::toggleoffhorizWidth, ZaMultiCompX2Artwork::toggleoffhorizHeight);

    Point<int> togglePosStart(247,108);

    fToggleBypass3 = new ImageToggle(this, toggleoffImage, toggleonImage);
    fToggleBypass3->setAbsolutePos(togglePosStart);
    fToggleBypass3->setCallback(this);

    togglePosStart.setY(158);

    fToggleBypass2 = new ImageToggle(this, toggleoffImage, toggleonImage);
    fToggleBypass2->setAbsolutePos(togglePosStart);
    fToggleBypass2->setCallback(this);

    togglePosStart.setY(209);

    fToggleBypass1 = new ImageToggle(this, toggleoffImage, toggleonImage);
    fToggleBypass1->setAbsolutePos(togglePosStart);
    fToggleBypass1->setCallback(this);

    togglePosStart.setX(278);
    togglePosStart.setY(113);

    fToggleListen3 = new ImageToggle(this, toggleoffhImage, toggleonhImage);
    fToggleListen3->setAbsolutePos(togglePosStart);
    fToggleListen3->setCallback(this);

    togglePosStart.setY(164);

    fToggleListen2 = new ImageToggle(this, toggleoffhImage, toggleonhImage);
    fToggleListen2->setAbsolutePos(togglePosStart);
    fToggleListen2->setCallback(this);

    togglePosStart.setY(214);

    fToggleListen1 = new ImageToggle(this, toggleoffhImage, toggleonhImage);
    fToggleListen1->setAbsolutePos(togglePosStart);
    fToggleListen1->setCallback(this);

    togglePosStart.setX(285.5);
    togglePosStart.setY(254);

    fToggleStereo = new ImageToggle(this, toggleoffImage, toggleonImage);
    fToggleStereo->setAbsolutePos(togglePosStart);
    fToggleStereo->setCallback(this);

    fCanvasArea.setPos(540, 32);
    fCanvasArea.setSize(102, 102);
    fThresh[0] = -20.f;
    fThresh[1] = -18.f;
    fThresh[2] = -16.f;
    fRatio = 4.f;
    fKnee = 0.f;
    fMakeup[0] = 0.f;
    fMakeup[1] = 0.f;
    fMakeup[2] = 0.f;
    fBypass[0] = 0.f;
    fBypass[1] = 0.f;
    fBypass[2] = 0.f;
    fMaster = 0.f;

    int i,k;

    for (k = 0; k < MAX_COMP; ++k) {
        for (i = 0; i < COMPOINTS; ++i) {
                compx[k][i] = fCanvasArea.getX();
		compy[k][i] = fCanvasArea.getY() + fCanvasArea.getHeight();
	}
    }

    // set default values
    d_programChanged(0);
}

void ZaMultiCompX2UI::compcurve(float in, int k, float *outx, float* outy) {
        float knee = fKnee;
        float ratio = fRatio;
        float makeup = fMakeup[k] + fMaster;
        float thresdb = fThresh[k];
        float width=((knee+1.f)-0.99f)*6.f;
        float xg, yg;

        yg = 0.f;
        xg = (in==0.f) ? -160.f : to_dB(fabs(in));
        xg = sanitize_denormal(xg);

        if (2.f*(xg-thresdb)<-width) {
                yg = xg;
        } else if (2.f*fabs(xg-thresdb)<=width) {
                yg = xg + (1.f/ratio-1.f)*(xg-thresdb+width/2.f)*(xg-thresdb+width/2.f)/(2.f*width);
        } else if (2.f*(xg-thresdb)>width) {
                yg = thresdb + (xg-thresdb)/ratio;
        }
        yg = sanitize_denormal(yg);

        *outx = (to_dB(in) + 1.) / 55. + 1.;
        *outy = !fBypass[k] ? (to_dB(in) + fMaster + 1.) / 55. + 1. : (yg + makeup + 1.) / 55. + 1.;
	//printf("x = %f  y = %f\n",*outx,*outy);
}

void ZaMultiCompX2UI::calc_compcurves() {
        float max_x = 1.f;
        float min_x = 0.f;
        float x2;
	for (int k = 0; k < MAX_COMP; ++k) {
	        for (int i = 0; i < COMPOINTS; ++i) {
	                x2 = (max_x - min_x) / COMPOINTS * i + min_x;
	                compcurve(x2, k, &compx[k][i], &compy[k][i]);
                        compx[k][i] = fCanvasArea.getX() + compx[k][i]*fCanvasArea.getWidth();
                        compy[k][i] = fCanvasArea.getY() + (1.-compy[k][i])*fCanvasArea.getHeight();
		}
	       	 //dot follows curve:
	        //compcurve(from_dB(-ui->gainred), k, &dotx[0], &doty[0]);
	        //dotx[0] = -(1-dotx[0])*280. + 280.;
	        //doty[0] = (1.-doty[0])*280.;

	        //dot follows centre:
	        //dotx[0] = -(1.- from_dB(-gainred))*280. + 280.;
	        //doty[0] = (1.- from_dB(-gainred))*280.;

	        //printf("gainr=%.2f x=%.2f y=%.2f\n",ui->gainred, ui->dotx[0], ui->doty[0]);
	}
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZaMultiCompX2UI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case ZaMultiCompX2Plugin::paramAttack:
        fKnobAttack->setValue(value);
        break;
    case ZaMultiCompX2Plugin::paramRelease:
        fKnobRelease->setValue(value);
        break;
    case ZaMultiCompX2Plugin::paramThresh1:
        fKnobThresh1->setValue(value);
        fThresh[0] = value;
        break;
    case ZaMultiCompX2Plugin::paramThresh2:
        fKnobThresh2->setValue(value);
        fThresh[1] = value;
        break;
    case ZaMultiCompX2Plugin::paramThresh3:
        fKnobThresh3->setValue(value);
        fThresh[2] = value;
        break;
    case ZaMultiCompX2Plugin::paramRatio:
        fKnobRatio->setValue(value);
        fRatio = value;
        break;
    case ZaMultiCompX2Plugin::paramKnee:
        fKnobKnee->setValue(value);
        fKnee = value;
        break;
    case ZaMultiCompX2Plugin::paramGlobalGain:
        fKnobGlobalGain->setValue(value);
        fMaster = value;
        break;
    case ZaMultiCompX2Plugin::paramGainR1:
        if (fLedRedValue1 != value)
        {
            fLedRedValue1 = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramGainR2:
        if (fLedRedValue2 != value)
        {
            fLedRedValue2 = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramGainR3:
        if (fLedRedValue3 != value)
        {
            fLedRedValue3 = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramOutputLevelL:
        if (fLedYellowValueL != value)
        {
            fLedYellowValueL = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramOutputLevelR:
        if (fLedYellowValueR != value)
        {
            fLedYellowValueR = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramMakeup1:
        fKnobMakeup1->setValue(value);
        if (fMakeup[0] != value)
        {
            fMakeup[0] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramMakeup2:
        fKnobMakeup2->setValue(value);
        if (fMakeup[1] != value)
        {
            fMakeup[1] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramMakeup3:
        fKnobMakeup3->setValue(value);
        if (fMakeup[2] != value)
        {
            fMakeup[2] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramToggle1:
        fToggleBypass1->setValue(value);
        if (fBypass[0] != value)
        {
            fBypass[0] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramToggle2:
        fToggleBypass2->setValue(value);
        if (fBypass[1] != value)
        {
            fBypass[1] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramToggle3:
        fToggleBypass3->setValue(value);
        if (fBypass[2] != value)
        {
            fBypass[2] = value;
            repaint();
        }
        break;
    case ZaMultiCompX2Plugin::paramListen1:
        fToggleListen1->setValue(value);
        break;
    case ZaMultiCompX2Plugin::paramListen2:
        fToggleListen2->setValue(value);
        break;
    case ZaMultiCompX2Plugin::paramListen3:
        fToggleListen3->setValue(value);
        break;
    }
}

void ZaMultiCompX2UI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobAttack->setValue(10.0f);
    fKnobRelease->setValue(80.0f);
/*
 * fKnobThresh->setValue(-6.0f);
    fKnobRatio->setValue(4.0f);
    fKnobKnee->setValue(0.0f);
    fKnobGlobalGain->setValue(0.0f);
    fKnobMakeup1->setValue(0.0f);
    fKnobMakeup2->setValue(0.0f);
    fKnobMakeup3->setValue(0.0f);
    fKnobXover1->setValue(250.0f);
    fKnobXover2->setValue(1400.0f);
   */
    fToggleBypass1->setValue(0.0f);
    fToggleBypass2->setValue(0.0f);
    fToggleBypass3->setValue(0.0f);
    fToggleListen1->setValue(0.0f);
    fToggleListen2->setValue(0.0f);
    fToggleListen3->setValue(0.0f);
    fToggleStereo->setValue(0.0f);
}

void ZaMultiCompX2UI::d_stateChanged(const char*, const char*)
{
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZaMultiCompX2UI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZaMultiCompX2Plugin::paramAttack, true);
    else if (knob == fKnobRelease)
        d_editParameter(ZaMultiCompX2Plugin::paramRelease, true);
    else if (knob == fKnobThresh1)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh1, true);
    else if (knob == fKnobThresh2)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh2, true);
    else if (knob == fKnobThresh3)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh3, true);
    else if (knob == fKnobRatio)
        d_editParameter(ZaMultiCompX2Plugin::paramRatio, true);
    else if (knob == fKnobKnee)
        d_editParameter(ZaMultiCompX2Plugin::paramKnee, true);
    else if (knob == fKnobGlobalGain)
        d_editParameter(ZaMultiCompX2Plugin::paramGlobalGain, true);
    else if (knob == fKnobMakeup1)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup1, true);
    else if (knob == fKnobMakeup2)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup2, true);
    else if (knob == fKnobMakeup3)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup3, true);
    else if (knob == fKnobXover1)
        d_editParameter(ZaMultiCompX2Plugin::paramXover1, true);
    else if (knob == fKnobXover2)
        d_editParameter(ZaMultiCompX2Plugin::paramXover2, true);
}

void ZaMultiCompX2UI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(ZaMultiCompX2Plugin::paramAttack, false);
    else if (knob == fKnobRelease)
        d_editParameter(ZaMultiCompX2Plugin::paramRelease, false);
    else if (knob == fKnobThresh1)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh1, false);
    else if (knob == fKnobThresh2)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh2, false);
    else if (knob == fKnobThresh3)
        d_editParameter(ZaMultiCompX2Plugin::paramThresh3, false);
    else if (knob == fKnobRatio)
        d_editParameter(ZaMultiCompX2Plugin::paramRatio, false);
    else if (knob == fKnobKnee)
        d_editParameter(ZaMultiCompX2Plugin::paramKnee, false);
    else if (knob == fKnobGlobalGain)
        d_editParameter(ZaMultiCompX2Plugin::paramGlobalGain, false);
    else if (knob == fKnobMakeup1)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup1, false);
    else if (knob == fKnobMakeup2)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup2, false);
    else if (knob == fKnobMakeup3)
        d_editParameter(ZaMultiCompX2Plugin::paramMakeup3, false);
    else if (knob == fKnobXover1)
        d_editParameter(ZaMultiCompX2Plugin::paramXover1, false);
    else if (knob == fKnobXover2)
        d_editParameter(ZaMultiCompX2Plugin::paramXover2, false);
}

void ZaMultiCompX2UI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobAttack)
        d_setParameterValue(ZaMultiCompX2Plugin::paramAttack, value);
    else if (knob == fKnobRelease)
        d_setParameterValue(ZaMultiCompX2Plugin::paramRelease, value);
    else if (knob == fKnobThresh1) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramThresh1, value);
	fThresh[0] = value;
    }
    else if (knob == fKnobThresh2) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramThresh2, value);
	fThresh[1] = value;
    }
    else if (knob == fKnobThresh3) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramThresh3, value);
	fThresh[2] = value;
    }
    else if (knob == fKnobRatio) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramRatio, value);
        fRatio = value;
    }
    else if (knob == fKnobKnee) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramKnee, value);
	fKnee = value;
    }
    else if (knob == fKnobGlobalGain) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramGlobalGain, value);
        fMaster = value;
    }
    else if (knob == fKnobMakeup1) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramMakeup1, value);
        fMakeup[0] = value;
    }
    else if (knob == fKnobMakeup2) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramMakeup2, value);
        fMakeup[1] = value;
    }
    else if (knob == fKnobMakeup3) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramMakeup3, value);
        fMakeup[2] = value;
    }
    else if (knob == fKnobXover1)
        d_setParameterValue(ZaMultiCompX2Plugin::paramXover1, value);
    else if (knob == fKnobXover2)
        d_setParameterValue(ZaMultiCompX2Plugin::paramXover2, value);
}

void ZaMultiCompX2UI::imageToggleClicked(ImageToggle* toggle, int)
{
    float v = toggle->getValue();
    if (toggle == fToggleBypass1) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramToggle1, v);
        fBypass[0] = v;
    }
    else if (toggle == fToggleBypass2) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramToggle2, v);
        fBypass[1] = v;
    }
    else if (toggle == fToggleBypass3) {
        d_setParameterValue(ZaMultiCompX2Plugin::paramToggle3, v);
        fBypass[2] = v;
    }
    else if (toggle == fToggleListen1)
        d_setParameterValue(ZaMultiCompX2Plugin::paramListen1, v);
    else if (toggle == fToggleListen2)
        d_setParameterValue(ZaMultiCompX2Plugin::paramListen2, v);
    else if (toggle == fToggleListen3)
        d_setParameterValue(ZaMultiCompX2Plugin::paramListen3, v);
    else if (toggle == fToggleStereo)
        d_setParameterValue(ZaMultiCompX2Plugin::paramStereoDet, v);
}

void ZaMultiCompX2UI::onDisplay()
{
    fImgBackground.draw();

    d_setState("stateMeterReset", "");

    // draw leds
    static const float sLedSpacing  = 15.5f;
    static const int   sLedInitialX = 343;

    static const int sYellowLedStaticYL = 254.5;
    static const int sYellowLedStaticYR = 269.5;
    static const int sRedLed1StaticY    = 215;
    static const int sRedLed2StaticY    = 164;
    static const int sRedLed3StaticY    = 113;

    int numRedLeds1;
    int numRedLeds2;
    int numRedLeds3;
    int numYellowLedsL;
    int numYellowLedsR;

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
        fLedRedImg.drawAt(sLedInitialX + (12 - i)*sLedSpacing, sRedLed1StaticY);

    for (int i=numRedLeds2; i>0; --i)
        fLedRedImg.drawAt(sLedInitialX + (12 - i)*sLedSpacing, sRedLed2StaticY);

    for (int i=numRedLeds3; i>0; --i)
        fLedRedImg.drawAt(sLedInitialX + (12 - i)*sLedSpacing, sRedLed3StaticY);

	if (fLedYellowValueL >= 20.f)
		numYellowLedsL = 19;
	else if (fLedYellowValueL >= 10.f)
		numYellowLedsL = 18;
	else if (fLedYellowValueL >= 8.f)
		numYellowLedsL = 17;
	else if (fLedYellowValueL >= 4.f)
		numYellowLedsL = 16;
	else if (fLedYellowValueL >= 2.f)
		numYellowLedsL = 15;
	else if (fLedYellowValueL >= 1.f)
		numYellowLedsL = 14;
	else if (fLedYellowValueL >= 0.f)
		numYellowLedsL = 13;
	else if (fLedYellowValueL >= -1.f)
		numYellowLedsL = 12;
	else if (fLedYellowValueL >= -2.f)
		numYellowLedsL = 11;
	else if (fLedYellowValueL >= -3.f)
		numYellowLedsL = 10;
	else if (fLedYellowValueL >= -4.f)
		numYellowLedsL = 9;
	else if (fLedYellowValueL >= -5.f)
		numYellowLedsL = 8;
	else if (fLedYellowValueL >= -6.f)
		numYellowLedsL = 7;
	else if (fLedYellowValueL >= -8.f)
		numYellowLedsL = 6;
	else if (fLedYellowValueL >= -10.f)
		numYellowLedsL = 5;
	else if (fLedYellowValueL >= -15.f)
		numYellowLedsL = 4;
	else if (fLedYellowValueL >= -20.f)
		numYellowLedsL = 3;
	else if (fLedYellowValueL >= -30.f)
		numYellowLedsL = 2;
	else if (fLedYellowValueL >= -40.f)
		numYellowLedsL = 1;
	else numYellowLedsL = 0;

	if (numYellowLedsL > 12) {
		for (int i=12; i<numYellowLedsL; ++i)
			fLedRedImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYL);
		for (int i=0; i<12; ++i)
			fLedYellowImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYL);
	} else {
		for (int i=0; i<numYellowLedsL; ++i)
			fLedYellowImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYL);
	}

	if (fLedYellowValueR >= 20.f)
		numYellowLedsR = 19;
	else if (fLedYellowValueR >= 10.f)
		numYellowLedsR = 18;
	else if (fLedYellowValueR >= 8.f)
		numYellowLedsR = 17;
	else if (fLedYellowValueR >= 4.f)
		numYellowLedsR = 16;
	else if (fLedYellowValueR >= 2.f)
		numYellowLedsR = 15;
	else if (fLedYellowValueR >= 1.f)
		numYellowLedsR = 14;
	else if (fLedYellowValueR >= 0.f)
		numYellowLedsR = 13;
	else if (fLedYellowValueR >= -1.f)
		numYellowLedsR = 12;
	else if (fLedYellowValueR >= -2.f)
		numYellowLedsR = 11;
	else if (fLedYellowValueR >= -3.f)
		numYellowLedsR = 10;
	else if (fLedYellowValueR >= -4.f)
		numYellowLedsR = 9;
	else if (fLedYellowValueR >= -5.f)
		numYellowLedsR = 8;
	else if (fLedYellowValueR >= -6.f)
		numYellowLedsR = 7;
	else if (fLedYellowValueR >= -8.f)
		numYellowLedsR = 6;
	else if (fLedYellowValueR >= -10.f)
		numYellowLedsR = 5;
	else if (fLedYellowValueR >= -15.f)
		numYellowLedsR = 4;
	else if (fLedYellowValueR >= -20.f)
		numYellowLedsR = 3;
	else if (fLedYellowValueR >= -30.f)
		numYellowLedsR = 2;
	else if (fLedYellowValueR >= -40.f)
		numYellowLedsR = 1;
	else numYellowLedsR = 0;

	if (numYellowLedsR > 12) {
		for (int i=12; i<numYellowLedsR; ++i)
			fLedRedImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYR);
		for (int i=0; i<12; ++i)
			fLedYellowImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYR);
	} else {
		for (int i=0; i<numYellowLedsR; ++i)
			fLedYellowImg.drawAt(sLedInitialX + i*sLedSpacing, sYellowLedStaticYR);
	}
/*
// TESTING - remove later
// this paints the 'fCanvasArea' so we can clearly see its bounds
{
const int x = fCanvasArea.getX();
const int y = fCanvasArea.getY();
const int w = fCanvasArea.getWidth();
const int h = fCanvasArea.getHeight();

glColor4f(0.0f, 1.0f, 0.0f, 0.9f);

glBegin(GL_QUADS);
glTexCoord2f(0.0f, 0.0f);
glVertex2i(x, y);

glTexCoord2f(1.0f, 0.0f);
glVertex2i(x+w, y);

glTexCoord2f(1.0f, 1.0f);
glVertex2i(x+w, y+h);

glTexCoord2f(0.0f, 1.0f);
glVertex2i(x, y+h);
glEnd();

// reset color
glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
*/

    calc_compcurves();

    //draw comp curves
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glLineWidth(2);
    int i,k;
    for (k = 0; k < MAX_COMP; ++k) {
        glColor4f((k==0)?1.f:0.235f, (k==1)?1.f:0.235f, (k==2)?1.f:0.235f, 1.0f);
        for (i = 2; i < COMPOINTS; ++i) {
            glBegin(GL_LINES);
                if (compy[k][i-1] < fCanvasArea.getY() + fCanvasArea.getHeight()
			&& compy[k][i-1] > fCanvasArea.getY()
                	&& compy[k][i] < fCanvasArea.getY() + fCanvasArea.getHeight()
			&& compy[k][i] > fCanvasArea.getY()) {
                    glVertex2i(compx[k][i-1], compy[k][i-1]);
                    glVertex2i(compx[k][i], compy[k][i]);
                }
	    glEnd();
        }
    }
    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new ZaMultiCompX2UI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
