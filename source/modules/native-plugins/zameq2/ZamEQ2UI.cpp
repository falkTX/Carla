/*
 * ZamEQ2 2 band parametric equaliser
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

#include "ZamEQ2UI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamEQ2UI::ZamEQ2UI()
    : UI()
{
    // background
    fImgBackground = Image(ZamEQ2Artwork::zameq2Data, ZamEQ2Artwork::zameq2Width, ZamEQ2Artwork::zameq2Height, GL_BGR);

    // toggle image
    Image sliderImage(ZamEQ2Artwork::togglesliderData, ZamEQ2Artwork::togglesliderWidth, ZamEQ2Artwork::togglesliderHeight);

    // knob
    Image knobImage(ZamEQ2Artwork::knobData, ZamEQ2Artwork::knobWidth, ZamEQ2Artwork::knobHeight);

    // knob
    fKnobGain1 = new ImageKnob(this, knobImage);
    fKnobGain1->setAbsolutePos(91, 172);
    fKnobGain1->setRange(-50.f, 20.0f);
    fKnobGain1->setRotationAngle(240);
    fKnobGain1->setDefault(0.0f);
    fKnobGain1->setCallback(this);

    fKnobQ1 = new ImageKnob(this, knobImage);
    fKnobQ1->setAbsolutePos(91, 122);
    fKnobQ1->setRange(0.1f, 6.0f);
    fKnobQ1->setUsingLogScale(true);
    fKnobQ1->setRotationAngle(240);
    fKnobQ1->setDefault(1.0f);
    fKnobQ1->setCallback(this);

    fKnobFreq1 = new ImageKnob(this, knobImage);
    fKnobFreq1->setAbsolutePos(23, 144);
    fKnobFreq1->setRange(20.f, 14000.0f);
    fKnobFreq1->setUsingLogScale(true);
    fKnobFreq1->setRotationAngle(240);
    fKnobFreq1->setDefault(500.0f);
    fKnobFreq1->setCallback(this);

    fKnobGain2 = new ImageKnob(this, knobImage);
    fKnobGain2->setAbsolutePos(567, 172);
    fKnobGain2->setRange(-50.f, 20.0f);
    fKnobGain2->setRotationAngle(240);
    fKnobGain2->setDefault(0.0f);
    fKnobGain2->setCallback(this);

    fKnobQ2 = new ImageKnob(this, knobImage);
    fKnobQ2->setAbsolutePos(567, 122);
    fKnobQ2->setRange(0.1f, 6.0f);
    fKnobQ2->setUsingLogScale(true);
    fKnobQ2->setRotationAngle(240);
    fKnobQ2->setDefault(1.0f);
    fKnobQ2->setCallback(this);

    fKnobFreq2 = new ImageKnob(this, knobImage);
    fKnobFreq2->setAbsolutePos(499, 144);
    fKnobFreq2->setRange(20.f, 14000.0f);
    fKnobFreq2->setUsingLogScale(true);
    fKnobFreq2->setRotationAngle(240);
    fKnobFreq2->setDefault(3000.0f);
    fKnobFreq2->setCallback(this);

    fKnobGainL = new ImageKnob(this, knobImage);
    fKnobGainL->setAbsolutePos(91, 52);
    fKnobGainL->setRange(-50.f, 20.0f);
    fKnobGainL->setRotationAngle(240);
    fKnobGainL->setDefault(0.0f);
    fKnobGainL->setCallback(this);

    fKnobFreqL = new ImageKnob(this, knobImage);
    fKnobFreqL->setAbsolutePos(23, 23);
    fKnobFreqL->setRange(20.f, 14000.0f);
    fKnobFreqL->setUsingLogScale(true);
    fKnobFreqL->setRotationAngle(240);
    fKnobFreqL->setDefault(250.0f);
    fKnobFreqL->setCallback(this);

    fKnobGainH = new ImageKnob(this, knobImage);
    fKnobGainH->setAbsolutePos(567, 53);
    fKnobGainH->setRange(-50.f, 20.0f);
    fKnobGainH->setRotationAngle(240);
    fKnobGainH->setDefault(0.0f);
    fKnobGainH->setCallback(this);

    fKnobFreqH = new ImageKnob(this, knobImage);
    fKnobFreqH->setAbsolutePos(499, 24);
    fKnobFreqH->setRange(20.f, 14000.0f);
    fKnobFreqH->setUsingLogScale(true);
    fKnobFreqH->setRotationAngle(240);
    fKnobFreqH->setDefault(8000.0f);
    fKnobFreqH->setCallback(this);

    Point<int> masterPosStart(211,204);
    Point<int> masterPosEnd(288,204);

    fSliderMaster = new ImageSlider(this, sliderImage);
    fSliderMaster->setStartPos(masterPosStart);
    fSliderMaster->setEndPos(masterPosEnd);
    fSliderMaster->setRange(-12.f,12.f);
    fSliderMaster->setValue(0.f);
    fSliderMaster->setStep(6.f);
    fSliderMaster->setCallback(this);

    fCanvasArea.setPos(165,10);
    fCanvasArea.setSize(305,180);

    // set default values
    d_programChanged(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZamEQ2UI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case ZamEQ2Plugin::paramGain1:
        fKnobGain1->setValue(value);
        break;
    case ZamEQ2Plugin::paramQ1:
        fKnobQ1->setValue(value);
        break;
    case ZamEQ2Plugin::paramFreq1:
        fKnobFreq1->setValue(value);
        break;
    case ZamEQ2Plugin::paramGain2:
        fKnobGain2->setValue(value);
        break;
    case ZamEQ2Plugin::paramQ2:
        fKnobQ2->setValue(value);
        break;
    case ZamEQ2Plugin::paramFreq2:
        fKnobFreq2->setValue(value);
        break;
    case ZamEQ2Plugin::paramGainL:
        fKnobGainL->setValue(value);
        break;
    case ZamEQ2Plugin::paramFreqL:
        fKnobFreqL->setValue(value);
        break;
    case ZamEQ2Plugin::paramGainH:
        fKnobGainH->setValue(value);
        break;
    case ZamEQ2Plugin::paramFreqH:
        fKnobFreqH->setValue(value);
        break;
    case ZamEQ2Plugin::paramMaster:
    	fSliderMaster->setValue(value);
	break;
    }
}

void ZamEQ2UI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
/*
    fKnobGain1->resetDefault();
    fKnobQ1->resetDefault();
    fKnobFreq1->resetDefault();
    fKnobGain2->resetDefault();
    fKnobQ2->resetDefault();
    fKnobFreq2->resetDefault();
    fKnobGainL->resetDefault();
    fKnobFreqL->resetDefault();
    fKnobGainH->resetDefault();
    fKnobFreqH->resetDefault();
*/
    fSliderMaster->setValue(0.f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZamEQ2UI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobGain1)
        d_editParameter(ZamEQ2Plugin::paramGain1, true);
    else if (knob == fKnobQ1)
        d_editParameter(ZamEQ2Plugin::paramQ1, true);
    else if (knob == fKnobFreq1)
        d_editParameter(ZamEQ2Plugin::paramFreq1, true);
    else if (knob == fKnobGain2)
        d_editParameter(ZamEQ2Plugin::paramGain2, true);
    else if (knob == fKnobQ2)
        d_editParameter(ZamEQ2Plugin::paramQ2, true);
    else if (knob == fKnobFreq2)
        d_editParameter(ZamEQ2Plugin::paramFreq2, true);
    else if (knob == fKnobGainL)
        d_editParameter(ZamEQ2Plugin::paramGainL, true);
    else if (knob == fKnobFreqL)
        d_editParameter(ZamEQ2Plugin::paramFreqL, true);
    else if (knob == fKnobGainH)
        d_editParameter(ZamEQ2Plugin::paramGainH, true);
    else if (knob == fKnobFreqH)
        d_editParameter(ZamEQ2Plugin::paramFreqH, true);
}

void ZamEQ2UI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobGain1)
        d_editParameter(ZamEQ2Plugin::paramGain1, false);
    else if (knob == fKnobQ1)
        d_editParameter(ZamEQ2Plugin::paramQ1, false);
    else if (knob == fKnobFreq1)
        d_editParameter(ZamEQ2Plugin::paramFreq1, false);
    else if (knob == fKnobGain2)
        d_editParameter(ZamEQ2Plugin::paramGain2, false);
    else if (knob == fKnobQ2)
        d_editParameter(ZamEQ2Plugin::paramQ2, false);
    else if (knob == fKnobFreq2)
        d_editParameter(ZamEQ2Plugin::paramFreq2, false);
    else if (knob == fKnobGainL)
        d_editParameter(ZamEQ2Plugin::paramGainL, false);
    else if (knob == fKnobFreqL)
        d_editParameter(ZamEQ2Plugin::paramFreqL, false);
    else if (knob == fKnobGainH)
        d_editParameter(ZamEQ2Plugin::paramGainH, false);
    else if (knob == fKnobFreqH)
        d_editParameter(ZamEQ2Plugin::paramFreqH, false);
}

void ZamEQ2UI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobGain1)
        d_setParameterValue(ZamEQ2Plugin::paramGain1, value);
    else if (knob == fKnobQ1)
        d_setParameterValue(ZamEQ2Plugin::paramQ1, value);
    else if (knob == fKnobFreq1)
        d_setParameterValue(ZamEQ2Plugin::paramFreq1, value);
    else if (knob == fKnobGain2)
        d_setParameterValue(ZamEQ2Plugin::paramGain2, value);
    else if (knob == fKnobQ2)
        d_setParameterValue(ZamEQ2Plugin::paramQ2, value);
    else if (knob == fKnobFreq2)
        d_setParameterValue(ZamEQ2Plugin::paramFreq2, value);
    else if (knob == fKnobGainL)
        d_setParameterValue(ZamEQ2Plugin::paramGainL, value);
    else if (knob == fKnobFreqL)
        d_setParameterValue(ZamEQ2Plugin::paramFreqL, value);
    else if (knob == fKnobGainH)
        d_setParameterValue(ZamEQ2Plugin::paramGainH, value);
    else if (knob == fKnobFreqH)
        d_setParameterValue(ZamEQ2Plugin::paramFreqH, value);
}

void ZamEQ2UI::imageSliderDragStarted(ImageSlider* slider)
{
    if (slider == fSliderMaster)
        d_editParameter(ZamEQ2Plugin::paramMaster, true);
}

void ZamEQ2UI::imageSliderDragFinished(ImageSlider* slider)
{
    if (slider == fSliderMaster)
        d_editParameter(ZamEQ2Plugin::paramMaster, false);
}

void ZamEQ2UI::imageSliderValueChanged(ImageSlider* slider, float value)
{
    if (slider == fSliderMaster)
        d_setParameterValue(ZamEQ2Plugin::paramMaster, value);
}

void ZamEQ2UI::lowshelf(int i, int ch, float srate, float fc, float g)
{
	float k, v0;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);

	if (g < 0.f) {
		// LF cut
		float denom = v0 + sqrt(2. * v0)*k + k*k;
		b0[ch][i] = v0 * (1. + sqrt(2.)*k + k*k) / denom;
		b1[ch][i] = 2. * v0*(k*k - 1.) / denom;
		b2[ch][i] = v0 * (1. - sqrt(2.)*k + k*k) / denom;
		a1[ch][i] = 2. * (k*k - v0) / denom;
		a2[ch][i] = (v0 - sqrt(2. * v0)*k + k*k) / denom;
	} else {
		// LF boost
		float denom = 1. + sqrt(2.)*k + k*k;
		b0[ch][i] = (1. + sqrt(2. * v0)*k + v0*k*k) / denom;
		b1[ch][i] = 2. * (v0*k*k - 1.) / denom;
		b2[ch][i] = (1. - sqrt(2. * v0)*k + v0*k*k) / denom;
		a1[ch][i] = 2. * (k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2.)*k + k*k) / denom;
	}
}

void ZamEQ2UI::highshelf(int i, int ch, float srate, float fc, float g)
{
	float k, v0;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);

	if (g < 0.f) {
		// HF cut
		float denom = 1. + sqrt(2. * v0)*k + v0*k*k;
		b0[ch][i] = v0*(1. + sqrt(2.)*k + k*k) / denom;
		b1[ch][i] = 2. * v0*(k*k - 1.) / denom;
		b2[ch][i] = v0*(1. - sqrt(2.)*k + k*k) / denom;
		a1[ch][i] = 2. * (v0*k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2. * v0)*k + v0*k*k) / denom;
	} else {
		// HF boost
		float denom = 1. + sqrt(2.)*k + k*k;
		b0[ch][i] = (v0 + sqrt(2. * v0)*k + k*k) / denom;
		b1[ch][i] = 2. * (k*k - v0) / denom;
		b2[ch][i] = (v0 - sqrt(2. * v0)*k + k*k) / denom;
		a1[ch][i] = 2. * (k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2.)*k + k*k) / denom;
	}
}

void ZamEQ2UI::peq(int i, int ch, float srate, float fc, float g, float bw)
{
	float k, v0, q;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);
	q = powf(2., 1./bw)/(powf(2., bw) - 1.); //q from octave bw

	if (g < 0.f) {
		// cut
		float denom = 1. + k/(v0*q) + k*k;
		b0[ch][i] = (1. + k/q + k*k) / denom;
		b1[ch][i] = 2. * (k*k - 1.) / denom;
		b2[ch][i] = (1. - k/q + k*k) / denom;
		a1[ch][i] = b1[ch][i];
		a2[ch][i] = (1. - k/(v0*q) + k*k) / denom;
	} else {
		// boost
		float denom = 1. + k/q + k*k;
		b0[ch][i] = (1. + k*v0/q + k*k) / denom;
		b1[ch][i] = 2. * (k*k - 1.) / denom;
		b2[ch][i] = (1. - k*v0/q + k*k) / denom;
		a1[ch][i] = b1[ch][i];
		a2[ch][i] = (1. - k/q + k*k) / denom;
	}
}

void ZamEQ2UI::calceqcurve(float x[], float y[])
{
        float SR = d_getSampleRate();
        float p1 = 10000.;
        float p2 = 5000.;
        float c2 = log10(1.+SR);
        float c1 = (1.+p1/SR)/(EQPOINTS*(p2/SR)*(p2/SR));

	double bw1 = fKnobQ1->getValue();
	double boost1 = fKnobGain1->getValue();
	double freq1 = fKnobFreq1->getValue();

	double bw2 = fKnobQ2->getValue();
	double boost2 = fKnobGain2->getValue();
	double freq2 = fKnobFreq2->getValue();

	double boostl = fKnobGainL->getValue();
	double freql = fKnobFreqL->getValue();

	double boosth = fKnobGainH->getValue();
	double freqh = fKnobFreqH->getValue();

        for (uint32_t i = 0; i < EQPOINTS; ++i) {
                x[i] = 1.5*log10(1.+i+c1)/c2;

                std::complex<double> H;
		double theta = -(i+0.005)*M_PI/EQPOINTS*20./(SR/1000.);
                std::complex<double> expiw = std::polar(1.0, theta);
                std::complex<double> exp2iw = std::polar(1.0, 2.0*theta);
                double freqH; //phaseH;

                lowshelf(0, 0, SR, freql, boostl);
                peq(1, 0, SR, freq1, boost1, bw1);
                peq(2, 0, SR, freq2, boost2, bw2);
                highshelf(3, 0, SR, freqh, boosth);

                H = (1. + a1[0][0]*expiw + a2[0][0]*exp2iw)/(b0[0][0] + b1[0][0]*expiw + b2[0][0]*exp2iw);
                H += (1. + a1[0][1]*expiw + a2[0][1]*exp2iw)/(b0[0][1] + b1[0][1]*expiw + b2[0][1]*exp2iw);
                H += (1. + a1[0][2]*expiw + a2[0][2]*exp2iw)/(b0[0][2] + b1[0][2]*expiw + b2[0][2]*exp2iw);
                H += (1. + a1[0][3]*expiw + a2[0][3]*exp2iw)/(b0[0][3] + b1[0][3]*expiw + b2[0][3]*exp2iw);

                freqH = std::abs(H);
                y[i] = (to_dB(freqH/4.)/5.)-(fSliderMaster->getValue())/24.f+0.5;
		x[i] = fCanvasArea.getX() + x[i]*fCanvasArea.getWidth();
		y[i] = fCanvasArea.getY() + y[i]*fCanvasArea.getHeight();
        }
}


void ZamEQ2UI::onDisplay()
{
    fImgBackground.draw();

    calceqcurve(eqx, eqy);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glLineWidth(2);
    int i;
    glColor4f(1.f, 1.f, 0.235f, 1.0f);
    for (i = 1; i < EQPOINTS; ++i) {
        glBegin(GL_LINES);
            if (eqy[i-1] < fCanvasArea.getY() + fCanvasArea.getHeight()
			&& eqy[i] < fCanvasArea.getY() + fCanvasArea.getHeight()
			&& eqy[i-1] > fCanvasArea.getY()
			&& eqy[i] > fCanvasArea.getY()) {
                glVertex2i(eqx[i-1], eqy[i-1]);
                glVertex2i(eqx[i], eqy[i]);
	    }
        glEnd();
    }
    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new ZamEQ2UI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
