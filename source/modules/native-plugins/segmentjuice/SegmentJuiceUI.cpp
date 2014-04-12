/*
 * Segment Juice Plugin
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

#include "SegmentJuiceUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

SegmentJuiceUI::SegmentJuiceUI()
    : UI(),
      fAboutWindow(this)
{
    // background
    fImgBackground = Image(SegmentJuiceArtwork::backgroundData, SegmentJuiceArtwork::backgroundWidth, SegmentJuiceArtwork::backgroundHeight, GL_BGR);

    // about
    Image imageAbout(SegmentJuiceArtwork::aboutData, SegmentJuiceArtwork::aboutWidth, SegmentJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // knobs
    Image knobImage(SegmentJuiceArtwork::knobData, SegmentJuiceArtwork::knobWidth, SegmentJuiceArtwork::knobHeight);

    // knobs2
    Image knobImage2(SegmentJuiceArtwork::knob2Data, SegmentJuiceArtwork::knob2Width, SegmentJuiceArtwork::knob2Height);

    // knob Wave1
    fKnobWave1 = new ImageKnob(this, knobImage);
    fKnobWave1->setPos(446, 79);
    fKnobWave1->setRange(1.0f, 4.0f);
    fKnobWave1->setValue(0.3f);
    fKnobWave1->setRotationAngle(270);
    fKnobWave1->setCallback(this);

    // knob Wave2
    fKnobWave2 = new ImageKnob(this, knobImage);
    fKnobWave2->setPos(446, 139);
    fKnobWave2->setRange(1.0f, 4.0f);
    fKnobWave2->setValue(3.0f);
    fKnobWave2->setRotationAngle(270);
    fKnobWave2->setCallback(this);

    // knob Wave3
    fKnobWave3 = new ImageKnob(this, knobImage);
    fKnobWave3->setPos(446, 199);
    fKnobWave3->setRange(1.0f, 4.0f);
    fKnobWave3->setValue(3.0f);
    fKnobWave3->setRotationAngle(270);
    fKnobWave3->setCallback(this);

    // knob Wave4
    fKnobWave4 = new ImageKnob(this, knobImage);
    fKnobWave4->setPos(446, 259);
    fKnobWave4->setRange(1.0f, 4.0f);
    fKnobWave4->setValue(3.0f);
    fKnobWave4->setRotationAngle(270);
    fKnobWave4->setCallback(this);

    // knob Wave5
    fKnobWave5 = new ImageKnob(this, knobImage);
    fKnobWave5->setPos(446, 319);
    fKnobWave5->setRange(1.0f, 4.0f);
    fKnobWave5->setValue(3.0f);
    fKnobWave5->setRotationAngle(270);
    fKnobWave5->setCallback(this);

    // knob Wave6
    fKnobWave6 = new ImageKnob(this, knobImage);
    fKnobWave6->setPos(446, 379);
    fKnobWave6->setRange(1.0f, 4.0f);
    fKnobWave6->setValue(3.0f);
    fKnobWave6->setRotationAngle(270);
    fKnobWave6->setCallback(this);

    // knob FM1
    fKnobFM1 = new ImageKnob(this, knobImage);
    fKnobFM1->setPos(510, 79);
    fKnobFM1->setRange(0.0f, 1.0f);
    fKnobFM1->setValue(0.5f);
    fKnobFM1->setRotationAngle(270);
    fKnobFM1->setCallback(this);

    // knob FM2
    fKnobFM2 = new ImageKnob(this, knobImage);
    fKnobFM2->setPos(510, 139);
    fKnobFM2->setRange(0.0f, 1.0f);
    fKnobFM2->setValue(0.5f);
    fKnobFM2->setRotationAngle(270);
    fKnobFM2->setCallback(this);

    // knob FM3
    fKnobFM3 = new ImageKnob(this, knobImage);
    fKnobFM3->setPos(510, 199);
    fKnobFM3->setRange(0.0f, 1.0f);
    fKnobFM3->setValue(0.5f);
    fKnobFM3->setRotationAngle(270);
    fKnobFM3->setCallback(this);

    // knob FM4
    fKnobFM4 = new ImageKnob(this, knobImage);
    fKnobFM4->setPos(510, 259);
    fKnobFM4->setRange(0.0f, 1.0f);
    fKnobFM4->setValue(0.5f);
    fKnobFM4->setRotationAngle(270);
    fKnobFM4->setCallback(this);

    // knob FM5
    fKnobFM5 = new ImageKnob(this, knobImage);
    fKnobFM5->setPos(510, 319);
    fKnobFM5->setRange(0.0f, 1.0f);
    fKnobFM5->setValue(0.5f);
    fKnobFM5->setRotationAngle(270);
    fKnobFM5->setCallback(this);

    // knob FM6
    fKnobFM6 = new ImageKnob(this, knobImage);
    fKnobFM6->setPos(510, 379);
    fKnobFM6->setRange(0.0f, 1.0f);
    fKnobFM6->setValue(0.5f);
    fKnobFM6->setRotationAngle(270);
    fKnobFM6->setCallback(this);

    // knob Pan1
    fKnobPan1 = new ImageKnob(this, knobImage);
    fKnobPan1->setPos(574, 79);
    fKnobPan1->setRange(-1.0f, 1.0f);
    fKnobPan1->setValue(0.0f);
    fKnobPan1->setRotationAngle(270);
    fKnobPan1->setCallback(this);

    // knob Pan2
    fKnobPan2 = new ImageKnob(this, knobImage);
    fKnobPan2->setPos(574, 139);
    fKnobPan2->setRange(-1.0f, 1.0f);
    fKnobPan2->setValue(0.0f);
    fKnobPan2->setRotationAngle(270);
    fKnobPan2->setCallback(this);

    // knob Pan3
    fKnobPan3 = new ImageKnob(this, knobImage);
    fKnobPan3->setPos(574, 199);
    fKnobPan3->setRange(-1.0f, 1.0f);
    fKnobPan3->setValue(0.0f);
    fKnobPan3->setRotationAngle(270);
    fKnobPan3->setCallback(this);

    // knob Pan4
    fKnobPan4 = new ImageKnob(this, knobImage);
    fKnobPan4->setPos(574, 259);
    fKnobPan4->setRange(-1.0f, 1.0f);
    fKnobPan4->setValue(0.0f);
    fKnobPan4->setRotationAngle(270);
    fKnobPan4->setCallback(this);

    // knob Pan5
    fKnobPan5 = new ImageKnob(this, knobImage);
    fKnobPan5->setPos(574, 319);
    fKnobPan5->setRange(-1.0f, 1.0f);
    fKnobPan5->setValue(0.0f);
    fKnobPan5->setRotationAngle(270);
    fKnobPan5->setCallback(this);

    // knob Pan6
    fKnobPan6 = new ImageKnob(this, knobImage);
    fKnobPan6->setPos(574, 379);
    fKnobPan6->setRange(-1.0f, 1.0f);
    fKnobPan6->setValue(0.0f);
    fKnobPan6->setRotationAngle(270);
    fKnobPan6->setCallback(this);

    // knob Amp1
    fKnobAmp1 = new ImageKnob(this, knobImage);
    fKnobAmp1->setPos(638, 79);
    fKnobAmp1->setRange(0.0f, 1.0f);
    fKnobAmp1->setValue(0.5f);
    fKnobAmp1->setRotationAngle(270);
    fKnobAmp1->setCallback(this);

    // knob Amp2
    fKnobAmp2 = new ImageKnob(this, knobImage);
    fKnobAmp2->setPos(638, 139);
    fKnobAmp2->setRange(0.0f, 1.0f);
    fKnobAmp2->setValue(0.5f);
    fKnobAmp2->setRotationAngle(270);
    fKnobAmp2->setCallback(this);

    // knob Amp3
    fKnobAmp3 = new ImageKnob(this, knobImage);
    fKnobAmp3->setPos(638, 199);
    fKnobAmp3->setRange(0.0f, 1.0f);
    fKnobAmp3->setValue(0.5f);
    fKnobAmp3->setRotationAngle(270);
    fKnobAmp3->setCallback(this);

    // knob Amp4
    fKnobAmp4 = new ImageKnob(this, knobImage);
    fKnobAmp4->setPos(638, 259);
    fKnobAmp4->setRange(0.0f, 1.0f);
    fKnobAmp4->setValue(0.5f);
    fKnobAmp4->setRotationAngle(270);
    fKnobAmp4->setCallback(this);

    // knob Amp5
    fKnobAmp5 = new ImageKnob(this, knobImage);
    fKnobAmp5->setPos(638, 319);
    fKnobAmp5->setRange(0.0f, 1.0f);
    fKnobAmp5->setValue(0.5f);
    fKnobAmp5->setRotationAngle(270);
    fKnobAmp5->setCallback(this);

    // knob Amp6
    fKnobAmp6 = new ImageKnob(this, knobImage);
    fKnobAmp6->setPos(638, 379);
    fKnobAmp6->setRange(0.0f, 1.0f);
    fKnobAmp6->setValue(0.5f);
    fKnobAmp6->setRotationAngle(270);
    fKnobAmp6->setCallback(this);

    // knob Attack
    fKnobAttack = new ImageKnob(this, knobImage2);
    fKnobAttack->setPos(34, 248);
    fKnobAttack->setRange(0.0f, 1.0f);
    fKnobAttack->setValue(0.0f);
    fKnobAttack->setRotationAngle(270);
    fKnobAttack->setCallback(this);

    // knob Decay
    fKnobDecay = new ImageKnob(this, knobImage2);
    fKnobDecay->setPos(132, 248);
    fKnobDecay->setRange(0.0f, 1.0f);
    fKnobDecay->setValue(0.0f);
    fKnobDecay->setRotationAngle(270);
    fKnobDecay->setCallback(this);

    // knob Sustain
    fKnobSustain = new ImageKnob(this, knobImage2);
    fKnobSustain->setPos(232, 248);
    fKnobSustain->setRange(0.0f, 1.0f);
    fKnobSustain->setValue(1.0f);
    fKnobSustain->setRotationAngle(270);
    fKnobSustain->setCallback(this);

    // knob Release
    fKnobRelease = new ImageKnob(this, knobImage2);
    fKnobRelease->setPos(330, 248);
    fKnobRelease->setRange(0.0f, 1.0f);
    fKnobRelease->setValue(0.0f);
    fKnobRelease->setRotationAngle(270);
    fKnobRelease->setCallback(this);

    // knob Stereo
    fKnobStereo = new ImageKnob(this, knobImage2);
    fKnobStereo->setPos(34, 339);
    fKnobStereo->setRange(-1.0f, 1.0f);
    fKnobStereo->setValue(0.0f);
    fKnobStereo->setRotationAngle(270);
    fKnobStereo->setCallback(this);

    // knob Tune
    fKnobTune = new ImageKnob(this, knobImage2);
    fKnobTune->setPos(132, 339);
    fKnobTune->setRange(-1.0f, 1.0f);
    fKnobTune->setValue(0.0f);
    fKnobTune->setRotationAngle(270);
    fKnobTune->setCallback(this);

    // knob Volume
    fKnobVolume = new ImageKnob(this, knobImage2);
    fKnobVolume->setPos(232, 339);
    fKnobVolume->setRange(0.0f, 1.0f);
    fKnobVolume->setValue(0.5f);
    fKnobVolume->setRotationAngle(270);
    fKnobVolume->setCallback(this);

    // knob Glide
    fKnobGlide = new ImageKnob(this, knobImage2);
    fKnobGlide->setPos(330, 339);
    fKnobGlide->setRange(0.0f, 1.0f);
    fKnobGlide->setValue(0.0f);
    fKnobGlide->setRotationAngle(270);
    fKnobGlide->setCallback(this);

    // about button
    Image aboutImageNormal(SegmentJuiceArtwork::aboutButtonNormalData, SegmentJuiceArtwork::aboutButtonNormalWidth, SegmentJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(SegmentJuiceArtwork::aboutButtonHoverData, SegmentJuiceArtwork::aboutButtonHoverWidth, SegmentJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(599, 17);
    fButtonAbout->setCallback(this);
}

SegmentJuiceUI::~SegmentJuiceUI()
{
    delete fKnobWave1;
    delete fKnobWave2;
    delete fKnobWave3;
    delete fKnobWave4;
    delete fKnobWave5;
    delete fKnobWave6;

    delete fKnobFM1;
    delete fKnobFM2;
    delete fKnobFM3;
    delete fKnobFM4;
    delete fKnobFM5;
    delete fKnobFM6;

    delete fKnobPan1;
    delete fKnobPan2;
    delete fKnobPan3;
    delete fKnobPan4;
    delete fKnobPan5;
    delete fKnobPan6;

    delete fKnobAmp1;
    delete fKnobAmp2;
    delete fKnobAmp3;
    delete fKnobAmp4;
    delete fKnobAmp5;
    delete fKnobAmp6;

    delete fKnobAttack;
    delete fKnobDecay;
    delete fKnobSustain;
    delete fKnobRelease;
    delete fKnobStereo;
    delete fKnobTune;
    delete fKnobVolume;
    delete fKnobGlide;
    delete fButtonAbout;
}

void SegmentJuiceUI::updateSynth() {
    synthL.setWave(0, fKnobWave1->getValue());
    synthL.setWave(1, fKnobWave2->getValue());
    synthL.setWave(2, fKnobWave3->getValue());
    synthL.setWave(3, fKnobWave4->getValue());
    synthL.setWave(4, fKnobWave5->getValue());
    synthL.setWave(5, fKnobWave6->getValue());
    synthL.setFM(0, fKnobFM1->getValue());
    synthL.setFM(1, fKnobFM2->getValue());
    synthL.setFM(2, fKnobFM3->getValue());
    synthL.setFM(3, fKnobFM4->getValue());
    synthL.setFM(4, fKnobFM5->getValue());
    synthL.setFM(5, fKnobFM6->getValue());
    synthL.setPan(0, -fKnobPan1->getValue());
    synthL.setPan(1, -fKnobPan2->getValue());
    synthL.setPan(2, -fKnobPan3->getValue());
    synthL.setPan(3, -fKnobPan4->getValue());
    synthL.setPan(4, -fKnobPan5->getValue());
    synthL.setPan(5, -fKnobPan6->getValue());
    synthL.setAmp(0, fKnobAmp1->getValue());
    synthL.setAmp(1, fKnobAmp2->getValue());
    synthL.setAmp(2, fKnobAmp3->getValue());
    synthL.setAmp(3, fKnobAmp4->getValue());
    synthL.setAmp(4, fKnobAmp5->getValue());
    synthL.setAmp(5, fKnobAmp6->getValue());
    synthL.setMAmp(fKnobVolume->getValue());

    synthR.setWave(0, fKnobWave1->getValue());
    synthR.setWave(1, fKnobWave2->getValue());
    synthR.setWave(2, fKnobWave3->getValue());
    synthR.setWave(3, fKnobWave4->getValue());
    synthR.setWave(4, fKnobWave5->getValue());
    synthR.setWave(5, fKnobWave6->getValue());
    synthR.setFM(0, fKnobFM1->getValue());
    synthR.setFM(1, fKnobFM2->getValue());
    synthR.setFM(2, fKnobFM3->getValue());
    synthR.setFM(3, fKnobFM4->getValue());
    synthR.setFM(4, fKnobFM5->getValue());
    synthR.setFM(5, fKnobFM6->getValue());
    synthR.setPan(0, fKnobPan1->getValue());
    synthR.setPan(1, fKnobPan2->getValue());
    synthR.setPan(2, fKnobPan3->getValue());
    synthR.setPan(3, fKnobPan4->getValue());
    synthR.setPan(4, fKnobPan5->getValue());
    synthR.setPan(5, fKnobPan6->getValue());
    synthR.setAmp(0, fKnobAmp1->getValue());
    synthR.setAmp(1, fKnobAmp2->getValue());
    synthR.setAmp(2, fKnobAmp3->getValue());
    synthR.setAmp(3, fKnobAmp4->getValue());
    synthR.setAmp(4, fKnobAmp5->getValue());
    synthR.setAmp(5, fKnobAmp6->getValue());
    synthR.setMAmp(fKnobVolume->getValue());
}

// -----------------------------------------------------------------------
// DSP Callbacks

void SegmentJuiceUI::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case SegmentJuicePlugin::paramWave1:
        fKnobWave1->setValue(value);
        break;
    case SegmentJuicePlugin::paramWave2:
        fKnobWave2->setValue(value);
        break;
    case SegmentJuicePlugin::paramWave3:
        fKnobWave3->setValue(value);
        break;
    case SegmentJuicePlugin::paramWave4:
        fKnobWave4->setValue(value);
        break;
    case SegmentJuicePlugin::paramWave5:
        fKnobWave5->setValue(value);
        break;
    case SegmentJuicePlugin::paramWave6:
        fKnobWave6->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM1:
        fKnobFM1->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM2:
        fKnobFM2->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM3:
        fKnobFM3->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM4:
        fKnobFM4->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM5:
        fKnobFM5->setValue(value);
        break;
    case SegmentJuicePlugin::paramFM6:
        fKnobFM6->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan1:
        fKnobPan1->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan2:
        fKnobPan2->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan3:
        fKnobPan3->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan4:
        fKnobPan4->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan5:
        fKnobPan5->setValue(value);
        break;
    case SegmentJuicePlugin::paramPan6:
        fKnobPan6->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp1:
        fKnobAmp1->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp2:
        fKnobAmp2->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp3:
        fKnobAmp3->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp4:
        fKnobAmp4->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp5:
        fKnobAmp5->setValue(value);
        break;
    case SegmentJuicePlugin::paramAmp6:
        fKnobAmp6->setValue(value);
        break;
    case SegmentJuicePlugin::paramAttack:
        fKnobAttack->setValue(value);
        break;
    case SegmentJuicePlugin::paramDecay:
        fKnobDecay->setValue(value);
        break;
    case SegmentJuicePlugin::paramSustain:
        fKnobSustain->setValue(value);
        break;
    case SegmentJuicePlugin::paramRelease:
        fKnobRelease->setValue(value);
        break;
    case SegmentJuicePlugin::paramStereo:
        fKnobStereo->setValue(value);
        break;
    case SegmentJuicePlugin::paramTune:
        fKnobTune->setValue(value);
        break;
    case SegmentJuicePlugin::paramVolume:
        fKnobVolume->setValue(value);
        break;
    case SegmentJuicePlugin::paramGlide:
        fKnobGlide->setValue(value);
        break;
    }

    updateSynth();
}

void SegmentJuiceUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    fKnobWave1->setValue(3.0f);
    fKnobWave2->setValue(3.0f);
    fKnobWave3->setValue(3.0f);
    fKnobWave6->setValue(3.0f);
    fKnobWave5->setValue(3.0f);
    fKnobWave6->setValue(3.0f);

    fKnobFM1->setValue(0.5f);
    fKnobFM2->setValue(0.5f);
    fKnobFM3->setValue(0.5f);
    fKnobFM6->setValue(0.5f);
    fKnobFM5->setValue(0.5f);
    fKnobFM6->setValue(0.5f);

    fKnobPan1->setValue(0.0f);
    fKnobPan2->setValue(0.0f);
    fKnobPan3->setValue(0.0f);
    fKnobPan6->setValue(0.0f);
    fKnobPan5->setValue(0.0f);
    fKnobPan6->setValue(0.0f);

    fKnobAmp1->setValue(0.5f);
    fKnobAmp2->setValue(0.5f);
    fKnobAmp3->setValue(0.5f);
    fKnobAmp6->setValue(0.5f);
    fKnobAmp5->setValue(0.5f);
    fKnobAmp6->setValue(0.5f);

    fKnobAttack->setValue(0.0f);
    fKnobDecay->setValue(0.0f);
    fKnobSustain->setValue(1.0f);
    fKnobRelease->setValue(0.0f);

    fKnobStereo->setValue(0.0f);
    fKnobTune->setValue(0.0f);
    fKnobVolume->setValue(0.5f);
    fKnobGlide->setValue(0.0f);

    for (int i=0; i<6; i++) {
        synthL.setFM(i, 0.5f);
        synthR.setFM(i, 0.5f);

        synthL.setPan(i, 0.0f);
        synthR.setPan(i, 0.0f);

        synthL.setAmp(i, 0.5f);
        synthR.setAmp(i, 0.5f);
    }

    synthL.setSampleRate(d_getSampleRate());
    synthR.setSampleRate(d_getSampleRate());

    synthL.setMAmp(0.5f);
    synthR.setMAmp(0.5f);

    synthL.play(69);
    synthR.play(69);
    synthL.setAttack(0);
    synthR.setAttack(0);
    synthL.setDecay(0);
    synthR.setDecay(0);
    synthL.setSustain(1);
    synthR.setSustain(1);
    synthL.setRelease(0);
    synthR.setRelease(0);
    synthL.setGlide(0);
    synthR.setGlide(0);
    synthL.setTune(0);
    synthR.setTune(0);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void SegmentJuiceUI::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void SegmentJuiceUI::imageKnobDragStarted(ImageKnob* knob)
{
    if (knob == fKnobWave1)
        d_editParameter(SegmentJuicePlugin::paramWave1, true);
    else if (knob == fKnobWave2)
        d_editParameter(SegmentJuicePlugin::paramWave2, true);
    else if (knob == fKnobWave3)
        d_editParameter(SegmentJuicePlugin::paramWave3, true);
    else if (knob == fKnobWave4)
        d_editParameter(SegmentJuicePlugin::paramWave4, true);
    else if (knob == fKnobWave5)
        d_editParameter(SegmentJuicePlugin::paramWave5, true);
    else if (knob == fKnobWave6)
        d_editParameter(SegmentJuicePlugin::paramWave6, true);
    else if (knob == fKnobFM1)
        d_editParameter(SegmentJuicePlugin::paramFM1, true);
    else if (knob == fKnobFM2)
        d_editParameter(SegmentJuicePlugin::paramFM2, true);
    else if (knob == fKnobFM3)
        d_editParameter(SegmentJuicePlugin::paramFM3, true);
    else if (knob == fKnobFM4)
        d_editParameter(SegmentJuicePlugin::paramFM4, true);
    else if (knob == fKnobFM5)
        d_editParameter(SegmentJuicePlugin::paramFM5, true);
    else if (knob == fKnobFM6)
        d_editParameter(SegmentJuicePlugin::paramFM6, true);
    else if (knob == fKnobPan1)
        d_editParameter(SegmentJuicePlugin::paramPan1, true);
    else if (knob == fKnobPan2)
        d_editParameter(SegmentJuicePlugin::paramPan2, true);
    else if (knob == fKnobPan3)
        d_editParameter(SegmentJuicePlugin::paramPan3, true);
    else if (knob == fKnobPan4)
        d_editParameter(SegmentJuicePlugin::paramPan4, true);
    else if (knob == fKnobPan5)
        d_editParameter(SegmentJuicePlugin::paramPan5, true);
    else if (knob == fKnobPan6)
        d_editParameter(SegmentJuicePlugin::paramPan6, true);
    else if (knob == fKnobAmp1)
        d_editParameter(SegmentJuicePlugin::paramAmp1, true);
    else if (knob == fKnobAmp2)
        d_editParameter(SegmentJuicePlugin::paramAmp2, true);
    else if (knob == fKnobAmp3)
        d_editParameter(SegmentJuicePlugin::paramAmp3, true);
    else if (knob == fKnobAmp4)
        d_editParameter(SegmentJuicePlugin::paramAmp4, true);
    else if (knob == fKnobAmp5)
        d_editParameter(SegmentJuicePlugin::paramAmp5, true);
    else if (knob == fKnobAmp6)
        d_editParameter(SegmentJuicePlugin::paramAmp6, true);
    else if (knob == fKnobAttack)
        d_editParameter(SegmentJuicePlugin::paramAttack, true);
    else if (knob == fKnobDecay)
        d_editParameter(SegmentJuicePlugin::paramDecay, true);
    else if (knob == fKnobSustain)
        d_editParameter(SegmentJuicePlugin::paramSustain, true);
    else if (knob == fKnobRelease)
        d_editParameter(SegmentJuicePlugin::paramRelease, true);
    else if (knob == fKnobStereo)
        d_editParameter(SegmentJuicePlugin::paramStereo, true);
    else if (knob == fKnobTune)
        d_editParameter(SegmentJuicePlugin::paramTune, true);
    else if (knob == fKnobVolume)
        d_editParameter(SegmentJuicePlugin::paramVolume, true);
    else if (knob == fKnobGlide)
        d_editParameter(SegmentJuicePlugin::paramGlide, true);
}

void SegmentJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobWave1)
        d_editParameter(SegmentJuicePlugin::paramWave1, false);
    else if (knob == fKnobWave2)
        d_editParameter(SegmentJuicePlugin::paramWave2, false);
    else if (knob == fKnobWave3)
        d_editParameter(SegmentJuicePlugin::paramWave3, false);
    else if (knob == fKnobWave4)
        d_editParameter(SegmentJuicePlugin::paramWave4, false);
    else if (knob == fKnobWave5)
        d_editParameter(SegmentJuicePlugin::paramWave5, false);
    else if (knob == fKnobWave6)
        d_editParameter(SegmentJuicePlugin::paramWave6, false);
    else if (knob == fKnobFM1)
        d_editParameter(SegmentJuicePlugin::paramFM1, false);
    else if (knob == fKnobFM2)
        d_editParameter(SegmentJuicePlugin::paramFM2, false);
    else if (knob == fKnobFM3)
        d_editParameter(SegmentJuicePlugin::paramFM3, false);
    else if (knob == fKnobFM4)
        d_editParameter(SegmentJuicePlugin::paramFM4, false);
    else if (knob == fKnobFM5)
        d_editParameter(SegmentJuicePlugin::paramFM5, false);
    else if (knob == fKnobFM6)
        d_editParameter(SegmentJuicePlugin::paramFM6, false);
    else if (knob == fKnobPan1)
        d_editParameter(SegmentJuicePlugin::paramPan1, false);
    else if (knob == fKnobPan2)
        d_editParameter(SegmentJuicePlugin::paramPan2, false);
    else if (knob == fKnobPan3)
        d_editParameter(SegmentJuicePlugin::paramPan3, false);
    else if (knob == fKnobPan4)
        d_editParameter(SegmentJuicePlugin::paramPan4, false);
    else if (knob == fKnobPan5)
        d_editParameter(SegmentJuicePlugin::paramPan5, false);
    else if (knob == fKnobPan6)
        d_editParameter(SegmentJuicePlugin::paramPan6, false);
    else if (knob == fKnobAmp1)
        d_editParameter(SegmentJuicePlugin::paramAmp1, false);
    else if (knob == fKnobAmp2)
        d_editParameter(SegmentJuicePlugin::paramAmp2, false);
    else if (knob == fKnobAmp3)
        d_editParameter(SegmentJuicePlugin::paramAmp3, false);
    else if (knob == fKnobAmp4)
        d_editParameter(SegmentJuicePlugin::paramAmp4, false);
    else if (knob == fKnobAmp5)
        d_editParameter(SegmentJuicePlugin::paramAmp5, false);
    else if (knob == fKnobAmp6)
        d_editParameter(SegmentJuicePlugin::paramAmp6, false);
    else if (knob == fKnobAttack)
        d_editParameter(SegmentJuicePlugin::paramAttack, false);
    else if (knob == fKnobDecay)
        d_editParameter(SegmentJuicePlugin::paramDecay, false);
    else if (knob == fKnobSustain)
        d_editParameter(SegmentJuicePlugin::paramSustain, false);
    else if (knob == fKnobRelease)
        d_editParameter(SegmentJuicePlugin::paramRelease, false);
    else if (knob == fKnobStereo)
        d_editParameter(SegmentJuicePlugin::paramStereo, false);
    else if (knob == fKnobTune)
        d_editParameter(SegmentJuicePlugin::paramTune, false);
    else if (knob == fKnobVolume)
        d_editParameter(SegmentJuicePlugin::paramVolume, false);
    else if (knob == fKnobGlide)
        d_editParameter(SegmentJuicePlugin::paramGlide, false);
}

void SegmentJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobWave1)
        d_setParameterValue(SegmentJuicePlugin::paramWave1, value);
    else if (knob == fKnobWave2)
        d_setParameterValue(SegmentJuicePlugin::paramWave2, value);
    else if (knob == fKnobWave3)
        d_setParameterValue(SegmentJuicePlugin::paramWave3, value);
    else if (knob == fKnobWave4)
        d_setParameterValue(SegmentJuicePlugin::paramWave4, value);
    else if (knob == fKnobWave5)
        d_setParameterValue(SegmentJuicePlugin::paramWave5, value);
    else if (knob == fKnobWave6)
        d_setParameterValue(SegmentJuicePlugin::paramWave6, value);
    else if (knob == fKnobFM1)
        d_setParameterValue(SegmentJuicePlugin::paramFM1, value);
    else if (knob == fKnobFM2)
        d_setParameterValue(SegmentJuicePlugin::paramFM2, value);
    else if (knob == fKnobFM3)
        d_setParameterValue(SegmentJuicePlugin::paramFM3, value);
    else if (knob == fKnobFM4)
        d_setParameterValue(SegmentJuicePlugin::paramFM4, value);
    else if (knob == fKnobFM5)
        d_setParameterValue(SegmentJuicePlugin::paramFM5, value);
    else if (knob == fKnobFM6)
        d_setParameterValue(SegmentJuicePlugin::paramFM6, value);
    else if (knob == fKnobPan1)
        d_setParameterValue(SegmentJuicePlugin::paramPan1, value);
    else if (knob == fKnobPan2)
        d_setParameterValue(SegmentJuicePlugin::paramPan2, value);
    else if (knob == fKnobPan3)
        d_setParameterValue(SegmentJuicePlugin::paramPan3, value);
    else if (knob == fKnobPan4)
        d_setParameterValue(SegmentJuicePlugin::paramPan4, value);
    else if (knob == fKnobPan5)
        d_setParameterValue(SegmentJuicePlugin::paramPan5, value);
    else if (knob == fKnobPan6)
        d_setParameterValue(SegmentJuicePlugin::paramPan6, value);
    else if (knob == fKnobAmp1)
        d_setParameterValue(SegmentJuicePlugin::paramAmp1, value);
    else if (knob == fKnobAmp2)
        d_setParameterValue(SegmentJuicePlugin::paramAmp2, value);
    else if (knob == fKnobAmp3)
        d_setParameterValue(SegmentJuicePlugin::paramAmp3, value);
    else if (knob == fKnobAmp4)
        d_setParameterValue(SegmentJuicePlugin::paramAmp4, value);
    else if (knob == fKnobAmp5)
        d_setParameterValue(SegmentJuicePlugin::paramAmp5, value);
    else if (knob == fKnobAmp6)
        d_setParameterValue(SegmentJuicePlugin::paramAmp6, value);
    else if (knob == fKnobAttack)
        d_setParameterValue(SegmentJuicePlugin::paramAttack, value);
    else if (knob == fKnobDecay)
        d_setParameterValue(SegmentJuicePlugin::paramDecay, value);
    else if (knob == fKnobSustain)
        d_setParameterValue(SegmentJuicePlugin::paramSustain, value);
    else if (knob == fKnobRelease)
        d_setParameterValue(SegmentJuicePlugin::paramRelease, value);
    else if (knob == fKnobStereo)
        d_setParameterValue(SegmentJuicePlugin::paramStereo, value);
    else if (knob == fKnobTune)
        d_setParameterValue(SegmentJuicePlugin::paramTune, value);
    else if (knob == fKnobVolume)
        d_setParameterValue(SegmentJuicePlugin::paramVolume, value);
    else if (knob == fKnobGlide)
        d_setParameterValue(SegmentJuicePlugin::paramGlide, value);

    updateSynth();

}

void SegmentJuiceUI::onDisplay()
{
    fImgBackground.draw();

    int cX = 23+4;
    int cY = 50;
    int cW = 388-cX-3;
    int cH = 216-cY;

    //draw waveform
    synthL.play(71);
    synthR.play(71);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable( GL_LINE_SMOOTH );
    //glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glLineWidth(1.0f);
    //draw #left waveform
    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
    glBegin(GL_LINE_STRIP);
        for (int i = 0; i<cW; i++) {
            float out = synthL.run()*cH/2+cH/2+cY;
            glVertex2i(i+cX, out);
            //std::cout << out << std::endl;
        }
    //draw #right waveform
    glEnd();
    glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
    glBegin(GL_LINE_STRIP);
            for (int i = 0; i<cW; i++) {
                    glVertex2i(i+cX, synthR.run()*cH/2+cH/2+cY);
            }
    glEnd();

    //draw 0dc line

    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glBegin(GL_LINES);
            glVertex2i(cX, cY+cH/2);
            glVertex2i(cX+cW, cY+cH/2);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new SegmentJuiceUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
