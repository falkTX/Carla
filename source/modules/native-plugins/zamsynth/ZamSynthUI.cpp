/*
 * ZamSynth polyphonic synthesiser
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

#include "ZamSynthPlugin.hpp"
#include "ZamSynthUI.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamSynthUI::ZamSynthUI()
    : UI()
{
    // background
    fImgBackground = Image(ZamSynthArtwork::zamsynthData, ZamSynthArtwork::zamsynthWidth, ZamSynthArtwork::zamsynthHeight, GL_BGR);

    fDragging = false;
    fDragValid = false;

    // knob
    Image knobImage(ZamSynthArtwork::knobData, ZamSynthArtwork::knobWidth, ZamSynthArtwork::knobHeight);

    // button
    Image smoothrImage(ZamSynthArtwork::smoothrData, ZamSynthArtwork::smoothrWidth, ZamSynthArtwork::smoothrHeight);
    Image smoothyImage(ZamSynthArtwork::smoothyData, ZamSynthArtwork::smoothyWidth, ZamSynthArtwork::smoothyHeight);

    // toggle
    Image toggleonImage(ZamSynthArtwork::toggleonData, ZamSynthArtwork::toggleonWidth, ZamSynthArtwork::toggleonHeight);
    Image toggleoffImage(ZamSynthArtwork::toggleoffData, ZamSynthArtwork::toggleoffWidth, ZamSynthArtwork::toggleoffHeight);

    // knob

    fKnobGain = new ImageKnob(this, knobImage);
    fKnobGain->setAbsolutePos(284.75, 240);
    fKnobGain->setRange(-30.f, 30.0f);
    fKnobGain->setDefault(0.0f);
    fKnobGain->setRotationAngle(240);
    fKnobGain->setCallback(this);

    fKnobSpeed = new ImageKnob(this, knobImage);
    fKnobSpeed->setAbsolutePos(284.75, 92.5);
    fKnobSpeed->setRange(1.f, 20.0f);
    fKnobSpeed->setDefault(10.0f);
    fKnobSpeed->setStep(1.0f);
    fKnobSpeed->setRotationAngle(240);
    fKnobSpeed->setCallback(this);

    // button
    fButtonSmooth = new ImageButton(this, smoothrImage, smoothrImage, smoothyImage);
    fButtonSmooth->setAbsolutePos(265, 165);
    fButtonSmooth->setCallback(this);

    // drawing area
    fCanvasArea.setPos(10,10);
    fCanvasArea.setSize(AREAHEIGHT,AREAHEIGHT);
    for (int i = 0; i < AREAHEIGHT; i++) {
        wave_y[i] = -(AREAHEIGHT*(sin(2.*i*M_PI/AREAHEIGHT)-1.0))/2.;
        env_y[i] = -(2*AREAHEIGHT*(sin(2.*i*M_PI/AREAHEIGHT/2.)-1.0))/2. < AREAHEIGHT / 2. ? -(2*AREAHEIGHT*(sin(2.*i*M_PI/AREAHEIGHT/2.)-1.0))/2. : AREAHEIGHT / 2.;
    }

    // toggle
    fToggleGraph = new ImageToggle(this, toggleonImage, toggleoffImage);
    fToggleGraph->setAbsolutePos(300, 33);
    fToggleGraph->setCallback(this);
    fToggleGraph->setValue(0.f);

    // set default values
    d_programChanged(0);
}

void ZamSynthUI::d_stateChanged(const char* key, const char* value)
{
        if (strcmp(key, "waveform") == 0) {
	        char* tmp;
	        int i = 0;
	        char tmpbuf[4*AREAHEIGHT+1] = {0};
	        snprintf(tmpbuf, 4*AREAHEIGHT, "%s", value);
	        tmp = strtok(tmpbuf, " ");
	        while ((tmp != NULL) && (i < AREAHEIGHT)) {
	                wave_y[i] = AREAHEIGHT-((float)atoi(tmp));
	                i++;
	                //printf("reload dsp wave_y[%d]=%.2f ", i, wave_y[i]);
	                tmp = strtok(NULL, " ");
	        }
	} else if (strcmp(key, "envelope") == 0) {
	        char* tmp;
	        int i = 0;
	        char tmpbuf[4*AREAHEIGHT+1] = {0};
	        snprintf(tmpbuf, 4*AREAHEIGHT, "%s", value);
	        tmp = strtok(tmpbuf, " ");
	        while ((tmp != NULL) && (i < AREAHEIGHT)) {
	                env_y[i] = AREAHEIGHT-((float)atoi(tmp));
	                i++;
	                //printf("reload dsp env_y[%d]=%.2f ", i, env_y[i]);
	                tmp = strtok(NULL, " ");
	        }
	}
}

// -----------------------------------------------------------------------
// DSP Callbacks

void ZamSynthUI::d_parameterChanged(uint32_t index, float value)
{
	switch (index)
	{
	case ZamSynthPlugin::paramGain:
		fKnobGain->setValue(value);
		break;
	case ZamSynthPlugin::paramSpeed:
		fKnobSpeed->setValue(value);
		break;
	case ZamSynthPlugin::paramGraph:
		fToggleGraph->setValue(value);
		break;
	}
}

void ZamSynthUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    fKnobGain->setDefault(0.0f);
    fKnobSpeed->setDefault(10.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void ZamSynthUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobGain)
        d_editParameter(ZamSynthPlugin::paramGain, true);
    else if (knob == fKnobSpeed)
        d_editParameter(ZamSynthPlugin::paramSpeed, true);
}

void ZamSynthUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobGain)
        d_editParameter(ZamSynthPlugin::paramGain, false);
    else if (knob == fKnobSpeed)
        d_editParameter(ZamSynthPlugin::paramSpeed, false);
}

void ZamSynthUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobGain)
        d_setParameterValue(ZamSynthPlugin::paramGain, value);
    else if (knob == fKnobSpeed)
        d_setParameterValue(ZamSynthPlugin::paramSpeed, value);
}

void ZamSynthUI::imageButtonClicked(ImageButton*, int)
{
	float wavesmooth[AREAHEIGHT];
	float xs[AREAHEIGHT];
	int i;
	for (i = 0; i < AREAHEIGHT; i++) {
		xs[i] = i;
	}

	float *gr;
	gr = (fToggleGraph->getValue() == 1.f) ? env_y : wave_y;

	gaussiansmooth(wavesmooth, xs, gr, AREAHEIGHT, 4);
	memcpy(gr, wavesmooth, AREAHEIGHT*sizeof(float));

	char tmp[4*AREAHEIGHT+1] = {0};
	for(i = 0; i < AREAHEIGHT; i++) {
		char wavestr[5] = {0};
		snprintf(wavestr, sizeof(wavestr), "%03d ", (int) (fCanvasArea.getHeight()-gr[i]));
		strcat(tmp, wavestr);
	}

	if (fToggleGraph->getValue() == 1.f)
		d_setState("envelope", tmp);
	else
		d_setState("waveform", tmp);
}

void ZamSynthUI::imageToggleClicked(ImageToggle*, int)
{
	float toggle = fToggleGraph->getValue();
	fToggleGraph->setValue(toggle);
	d_setParameterValue(ZamSynthPlugin::paramGraph, toggle);
}

void ZamSynthUI::gaussiansmooth(float* smoothed, float* xs, float* ys, int n, int radius)
{
	int i,j;
	float numer;
	float denom;
	float kernel;
	for (i = 0; i < n; i++) {
		numer = 0.f;
		denom = 0.f;
		for (j = 0; j < n; j++) {
			kernel = expf(-(i - xs[j])*(i - xs[j]) / (2. * radius));
			numer += kernel * ys[j];
			denom += kernel;
		}
		smoothed[i] = numer / denom;
	}
}

bool ZamSynthUI::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! fCanvasArea.contains(ev.pos)) {
            //fDragValid = false;
            return false;
	}

        fDragging = true;
        fDragValid = true;
        return true;
    }
    else if (fDragging)
    {
        fDragging = false;
        return true;
    }

    return false;
}

bool ZamSynthUI::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;
    if (! fDragValid)
    {
        fDragValid = true;
    }

    int x = ev.pos.getX();
    int y = ev.pos.getY();

    if (x > fCanvasArea.getWidth()+10)
    x = fCanvasArea.getWidth()+10;
    if (x < 10) x = 10;
    if (y < 10) y = 10;

    float *gr;
    if (fToggleGraph->getValue() == 0.f) {
	gr = wave_y;
	if (y > fCanvasArea.getHeight()+10)
		y = fCanvasArea.getHeight()+10;
    } else {
    	gr = env_y;
	if (y > fCanvasArea.getHeight() / 2. + 10)
		y = fCanvasArea.getHeight() / 2. + 10;
    }

    if (gr[x-10] != (y-10)) {
	char tmp[4*AREAHEIGHT+1] = {0};
	int i;
	for(i = 0; i < AREAHEIGHT; i++) {
		char wavestr[5] = {0};
		snprintf(wavestr, sizeof(wavestr), "%03d ", (int) (fCanvasArea.getHeight()-gr[i]));
		strcat(tmp, wavestr);
	}

        gr[x-10] = y-10;

	if (gr == env_y)
		d_setState("envelope",tmp);
	else
		d_setState("waveform",tmp);

	repaint();
    }

    return true;
}


void ZamSynthUI::onDisplay()
{
	fImgBackground.draw();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glLineWidth(2);
    float *gr;
    gr = (fToggleGraph->getValue() == 1.f) ? env_y : wave_y;

    int i;
        glColor4f(0.235f, 1.f, 0.235f, 1.0f);
        for (i = 2; i < AREAHEIGHT; ++i) {
            glBegin(GL_LINES);
                    glVertex2i(i-1+fCanvasArea.getX(), gr[i-1]+fCanvasArea.getY());
                    glVertex2i(i+fCanvasArea.getX(), gr[i]+fCanvasArea.getY());
            glEnd();
        }
    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
// -----------------------------------------------------------------------

UI* createUI()
{
    return new ZamSynthUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
