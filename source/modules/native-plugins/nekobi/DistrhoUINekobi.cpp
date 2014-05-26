/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#include "DistrhoPluginNekobi.hpp"
#include "DistrhoUINekobi.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoUINekobi::DistrhoUINekobi()
    : UI(),
      fAboutWindow(this)
{
    // FIXME
    fNeko.setTimerSpeed(5);

    // background
    fImgBackground = Image(DistrhoArtworkNekobi::backgroundData, DistrhoArtworkNekobi::backgroundWidth, DistrhoArtworkNekobi::backgroundHeight, GL_BGR);

    Image aboutImage(DistrhoArtworkNekobi::aboutData, DistrhoArtworkNekobi::aboutWidth, DistrhoArtworkNekobi::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // slider
    Image sliderImage(DistrhoArtworkNekobi::sliderData, DistrhoArtworkNekobi::sliderWidth, DistrhoArtworkNekobi::sliderHeight);

    fSliderWaveform = new ImageSlider(this, sliderImage, DistrhoPluginNekobi::paramWaveform);
    fSliderWaveform->setStartPos(133, 40);
    fSliderWaveform->setEndPos(133, 60);
    fSliderWaveform->setRange(0.0f, 1.0f);
    fSliderWaveform->setStep(1.0f);
    fSliderWaveform->setValue(0.0f);
    fSliderWaveform->setCallback(this);

    // knobs
    Image knobImage(DistrhoArtworkNekobi::knobData, DistrhoArtworkNekobi::knobWidth, DistrhoArtworkNekobi::knobHeight);

    // knob Tuning
    fKnobTuning = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramTuning);
    fKnobTuning->setAbsolutePos(41, 43);
    fKnobTuning->setRange(-12.0f, 12.0f);
    fKnobTuning->setDefault(0.0f);
    fKnobTuning->setValue(0.0f);
    fKnobTuning->setRotationAngle(305);
    fKnobTuning->setCallback(this);

    // knob Cutoff
    fKnobCutoff = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramCutoff);
    fKnobCutoff->setAbsolutePos(185, 43);
    fKnobCutoff->setRange(0.0f, 100.0f);
    fKnobCutoff->setDefault(25.0f);
    fKnobCutoff->setValue(25.0f);
    fKnobCutoff->setRotationAngle(305);
    fKnobCutoff->setCallback(this);

    // knob Resonance
    fKnobResonance = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramResonance);
    fKnobResonance->setAbsolutePos(257, 43);
    fKnobResonance->setRange(0.0f, 95.0f);
    fKnobResonance->setDefault(25.0f);
    fKnobResonance->setValue(25.0f);
    fKnobResonance->setRotationAngle(305);
    fKnobResonance->setCallback(this);

    // knob Env Mod
    fKnobEnvMod = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramEnvMod);
    fKnobEnvMod->setAbsolutePos(329, 43);
    fKnobEnvMod->setRange(0.0f, 100.0f);
    fKnobEnvMod->setDefault(50.0f);
    fKnobEnvMod->setValue(50.0f);
    fKnobEnvMod->setRotationAngle(305);
    fKnobEnvMod->setCallback(this);

    // knob Decay
    fKnobDecay = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramDecay);
    fKnobDecay->setAbsolutePos(400, 43);
    fKnobDecay->setRange(0.0f, 100.0f);
    fKnobDecay->setDefault(75.0f);
    fKnobDecay->setValue(75.0f);
    fKnobDecay->setRotationAngle(305);
    fKnobDecay->setCallback(this);

    // knob Accent
    fKnobAccent = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramAccent);
    fKnobAccent->setAbsolutePos(473, 43);
    fKnobAccent->setRange(0.0f, 100.0f);
    fKnobAccent->setDefault(25.0f);
    fKnobAccent->setValue(25.0f);
    fKnobAccent->setRotationAngle(305);
    fKnobAccent->setCallback(this);

    // knob Volume
    fKnobVolume = new ImageKnob(this, knobImage, ImageKnob::Vertical, DistrhoPluginNekobi::paramVolume);
    fKnobVolume->setAbsolutePos(545, 43);
    fKnobVolume->setRange(0.0f, 100.0f);
    fKnobVolume->setDefault(75.0f);
    fKnobVolume->setValue(75.0f);
    fKnobVolume->setRotationAngle(305);
    fKnobVolume->setCallback(this);

    // about button
    Image aboutImageNormal(DistrhoArtworkNekobi::aboutButtonNormalData, DistrhoArtworkNekobi::aboutButtonNormalWidth, DistrhoArtworkNekobi::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtworkNekobi::aboutButtonHoverData, DistrhoArtworkNekobi::aboutButtonHoverWidth, DistrhoArtworkNekobi::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(505, 5);
    fButtonAbout->setCallback(this);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUINekobi::d_parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case DistrhoPluginNekobi::paramTuning:
        fKnobTuning->setValue(value);
        break;
    case DistrhoPluginNekobi::paramWaveform:
        fSliderWaveform->setValue(value);
        break;
    case DistrhoPluginNekobi::paramCutoff:
        fKnobCutoff->setValue(value);
        break;
    case DistrhoPluginNekobi::paramResonance:
        fKnobResonance->setValue(value);
        break;
    case DistrhoPluginNekobi::paramEnvMod:
        fKnobEnvMod->setValue(value);
        break;
    case DistrhoPluginNekobi::paramDecay:
        fKnobDecay->setValue(value);
        break;
    case DistrhoPluginNekobi::paramAccent:
        fKnobAccent->setValue(value);
        break;
    case DistrhoPluginNekobi::paramVolume:
        fKnobVolume->setValue(value);
        break;
    }
}

// -----------------------------------------------------------------------
// UI Callbacks

void DistrhoUINekobi::d_uiIdle()
{
    if (fNeko.idle())
        repaint();
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUINekobi::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void DistrhoUINekobi::imageKnobDragStarted(ImageKnob* knob)
{
    d_editParameter(knob->getId(), true);
}

void DistrhoUINekobi::imageKnobDragFinished(ImageKnob* knob)
{
    d_editParameter(knob->getId(), false);
}

void DistrhoUINekobi::imageKnobValueChanged(ImageKnob* knob, float value)
{
    d_setParameterValue(knob->getId(), value);
}

void DistrhoUINekobi::imageSliderDragStarted(ImageSlider* slider)
{
    d_editParameter(slider->getId(), true);
}

void DistrhoUINekobi::imageSliderDragFinished(ImageSlider* slider)
{
    d_editParameter(slider->getId(), false);
}

void DistrhoUINekobi::imageSliderValueChanged(ImageSlider* slider, float value)
{
    d_setParameterValue(slider->getId(), value);
}

void DistrhoUINekobi::onDisplay()
{
    fImgBackground.draw();
    fNeko.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUINekobi();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
