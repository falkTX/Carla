/*
 * Power Juice Plugin
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

#include "PowerJuiceUI.hpp"

#include <cstdlib>
#include <ctime>

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PowerJuiceUI::PowerJuiceUI()
    : UI(),
      fAboutWindow(this),
      shmData(nullptr)
{
    // background
    fImgBackground = Image(PowerJuiceArtwork::backgroundData, PowerJuiceArtwork::backgroundWidth, PowerJuiceArtwork::backgroundHeight, GL_BGR);

    // about
    Image imageAbout(PowerJuiceArtwork::aboutData, PowerJuiceArtwork::aboutWidth, PowerJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // knobs
    Image knobImage(PowerJuiceArtwork::knobData, PowerJuiceArtwork::knobWidth, PowerJuiceArtwork::knobHeight);

    // knob Attack
    fKnobAttack = new ImageKnob(this, knobImage);
    fKnobAttack->setPos(37, 213);
    fKnobAttack->setRange(0.1f, 1000.0f);
    fKnobAttack->setStep(0.1f);
    fKnobAttack->setValue(20.0f);
    fKnobAttack->setRotationAngle(270);
    fKnobAttack->setCallback(this);

    // knob Release
    fKnobRelease = new ImageKnob(this, knobImage);
    fKnobRelease->setPos(136, 213);
    fKnobRelease->setRange(0.1f, 1000.0f);
    fKnobRelease->setValue(0.1f);
    fKnobRelease->setRotationAngle(270);
    fKnobRelease->setCallback(this);

    // knob Threshold
    fKnobThreshold = new ImageKnob(this, knobImage);
    fKnobThreshold->setPos(235, 213);
    fKnobThreshold->setRange(-60.0f, 0.0f);
    fKnobThreshold->setValue(0.0f);
    fKnobThreshold->setRotationAngle(270);
    fKnobThreshold->setCallback(this);

    // knob Ratio
    fKnobRatio = new ImageKnob(this, knobImage);
    fKnobRatio->setPos(334, 213);
    fKnobRatio->setRange(1.0f, 10.0f);
    fKnobRatio->setValue(1.0f);
    fKnobRatio->setRotationAngle(270);
    fKnobRatio->setCallback(this);

    // knob Make-Up
    fKnobMakeup = new ImageKnob(this, knobImage);
    fKnobMakeup->setPos(433, 213);
    fKnobMakeup->setRange(0.0f, 20.0f);
    fKnobMakeup->setValue(0.0f);
    fKnobMakeup->setRotationAngle(270);
    fKnobMakeup->setCallback(this);

    // knob Mix
    fKnobMix = new ImageKnob(this, knobImage);
    fKnobMix->setPos(532, 213);
    fKnobMix->setRange(0.0f, 1.0f);
    fKnobMix->setValue(1.0f);
    fKnobMix->setRotationAngle(270);
    fKnobMix->setCallback(this);

    // about button
    Image aboutImageNormal(PowerJuiceArtwork::aboutButtonNormalData, PowerJuiceArtwork::aboutButtonNormalWidth, PowerJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(PowerJuiceArtwork::aboutButtonHoverData, PowerJuiceArtwork::aboutButtonHoverWidth, PowerJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(502, 17);
    fButtonAbout->setCallback(this);

    // init shm vars
    carla_shm_init(shm);
    shmData = nullptr;

    fFirstDisplay = true;
}

PowerJuiceUI::~PowerJuiceUI()
{
    delete fKnobAttack;
    delete fKnobRelease;
    delete fKnobThreshold;
    delete fKnobRatio;
    delete fKnobMakeup;
    delete fKnobMix;
    delete fButtonAbout;

    closeShm();
}

// -----------------------------------------------------------------------
// DSP Callbacks

void PowerJuiceUI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case PowerJuicePlugin::paramAttack:
        fKnobAttack->setValue(value);
        break;
    case PowerJuicePlugin::paramRelease:
        fKnobRelease->setValue(value);
        break;
    case PowerJuicePlugin::paramThreshold:
        fKnobThreshold->setValue(value);
        break;
    case PowerJuicePlugin::paramRatio:
        fKnobRatio->setValue(value);
        break;
    case PowerJuicePlugin::paramMakeup:
        fKnobMakeup->setValue(value);
        break;
    case PowerJuicePlugin::paramMix:
        fKnobMix->setValue(value);
        break;
    }
}

void PowerJuiceUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobAttack->setValue(20.0f);
    fKnobRelease->setValue(200.0f);
    fKnobThreshold->setValue(0.0f);
    fKnobRatio->setValue(1.0f);
    fKnobMakeup->setValue(0.0f);
    fKnobMix->setValue(1.0f);
}

void PowerJuiceUI::d_stateChanged(const char*, const char*)
{
}

// -----------------------------------------------------------------------
// Widget Callbacks

void PowerJuiceUI::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void PowerJuiceUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(PowerJuicePlugin::paramAttack, true);
    else if (knob == fKnobRelease)
        d_editParameter(PowerJuicePlugin::paramRelease, true);
    else if (knob == fKnobThreshold)
        d_editParameter(PowerJuicePlugin::paramThreshold, true);
    else if (knob == fKnobRatio)
        d_editParameter(PowerJuicePlugin::paramRatio, true);
    else if (knob == fKnobMakeup)
        d_editParameter(PowerJuicePlugin::paramMakeup, true);
    else if (knob == fKnobMix)
        d_editParameter(PowerJuicePlugin::paramMix, true);
}

void PowerJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobAttack)
        d_editParameter(PowerJuicePlugin::paramAttack, false);
    else if (knob == fKnobRelease)
        d_editParameter(PowerJuicePlugin::paramRelease, false);
    else if (knob == fKnobThreshold)
        d_editParameter(PowerJuicePlugin::paramThreshold, false);
    else if (knob == fKnobRatio)
        d_editParameter(PowerJuicePlugin::paramRatio, false);
    else if (knob == fKnobMakeup)
        d_editParameter(PowerJuicePlugin::paramMakeup, false);
    else if (knob == fKnobMix)
        d_editParameter(PowerJuicePlugin::paramMix, false);
}

void PowerJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobAttack)
        d_setParameterValue(PowerJuicePlugin::paramAttack, value);
    else if (knob == fKnobRelease)
        d_setParameterValue(PowerJuicePlugin::paramRelease, value);
    else if (knob == fKnobThreshold)
        d_setParameterValue(PowerJuicePlugin::paramThreshold, value);
    else if (knob == fKnobRatio)
        d_setParameterValue(PowerJuicePlugin::paramRatio, value);
    else if (knob == fKnobMakeup)
        d_setParameterValue(PowerJuicePlugin::paramMakeup, value);
    else if (knob == fKnobMix)
        d_setParameterValue(PowerJuicePlugin::paramMix, value);

}

void PowerJuiceUI::d_uiIdle() {
    repaint();
}

void PowerJuiceUI::onDisplay()
{
    if (fFirstDisplay)
    {
        initShm();
        fFirstDisplay = false;
    }

    fImgBackground.draw();

    if (shmData == nullptr)
        return;

    int w = 563; //waveform plane size, size of the plane in pixels;
    int w2 = 1126; //wavefowm array
    int h = 60; //waveform plane height
    int x = 28; //waveform plane positions
    int y = 51;
    int dc = 113; //0DC line y position

    //draw waveform
    for (int i=0; i<w2; i+=2) {
        //glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glEnable(GL_LINE_SMOOTH);
        //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
            glVertex2i(x+(i/2), shmData->input[i]*h+dc);
            glVertex2i(x+(i/2), shmData->input[i+1]*h+dc);
        glEnd();

        // reset color
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    //draw shits
}

void PowerJuiceUI::onClose()
{
    // tell DSP to stop sending SHM data
    d_setState("shmKey", "");
}

void PowerJuiceUI::initShm()
{
    // generate a random key
    static const char charSet[]  = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int  charSetLen = sizeof(charSet) - 1; // -1 to avoid trailing '\0'

    char shmKey[24+1];
    shmKey[24] = '\0';

    std::srand(std::time(nullptr));

    for (int i=0; i<24; ++i)
        shmKey[i] = charSet[std::rand() % charSetLen];

    // create shared memory
    shm = carla_shm_create(shmKey);

    if (! carla_is_shm_valid(shm))
    {
        carla_stderr2("Failed to created shared memory!");
        return;
    }

    if (! carla_shm_map<SharedMemData>(shm, shmData))
    {
        carla_stderr2("Failed to map shared memory!");
        return;
    }

    std::memset(shmData, 0, sizeof(SharedMemData));

    // tell DSP to use this key for SHM
    carla_stdout("Sending shmKey %s", shmKey);
    d_setState("shmKey", shmKey);
}

void PowerJuiceUI::closeShm()
{
    fFirstDisplay = true;

    if (! carla_is_shm_valid(shm))
        return;

    if (shmData != nullptr)
    {
        carla_shm_unmap<SharedMemData>(shm, shmData);
        shmData = nullptr;
    }

    carla_shm_close(shm);
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new PowerJuiceUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
