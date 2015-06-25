/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
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

namespace Art = DistrhoArtworkNekobi;

// -----------------------------------------------------------------------

DistrhoUINekobi::DistrhoUINekobi()
    : UI(Art::backgroundWidth, Art::backgroundHeight),
      fImgBackground(Art::backgroundData, Art::backgroundWidth, Art::backgroundHeight, GL_BGR),
      fAboutWindow(this)
{
    // FIXME
    fNeko.setTimerSpeed(5);

    // about
    Image aboutImage(Art::aboutData, Art::aboutWidth, Art::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // slider
    Image sliderImage(Art::sliderData, Art::sliderWidth, Art::sliderHeight);

    fSliderWaveform = new ImageSlider(this, sliderImage);
    fSliderWaveform->setId(DistrhoPluginNekobi::paramWaveform);
    fSliderWaveform->setStartPos(133, 40);
    fSliderWaveform->setEndPos(133, 60);
    fSliderWaveform->setRange(0.0f, 1.0f);
    fSliderWaveform->setStep(1.0f);
    fSliderWaveform->setValue(0.0f);
    fSliderWaveform->setCallback(this);

    // knobs
    Image knobImage(Art::knobData, Art::knobWidth, Art::knobHeight);

    // knob Tuning
    fKnobTuning = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobTuning->setId(DistrhoPluginNekobi::paramTuning);
    fKnobTuning->setAbsolutePos(41, 43);
    fKnobTuning->setRange(-12.0f, 12.0f);
    fKnobTuning->setDefault(0.0f);
    fKnobTuning->setValue(0.0f);
    fKnobTuning->setRotationAngle(305);
    fKnobTuning->setCallback(this);

    // knob Cutoff
    fKnobCutoff = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobCutoff->setId(DistrhoPluginNekobi::paramCutoff);
    fKnobCutoff->setAbsolutePos(185, 43);
    fKnobCutoff->setRange(0.0f, 100.0f);
    fKnobCutoff->setDefault(25.0f);
    fKnobCutoff->setValue(25.0f);
    fKnobCutoff->setRotationAngle(305);
    fKnobCutoff->setCallback(this);

    // knob Resonance
    fKnobResonance = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobResonance->setId(DistrhoPluginNekobi::paramResonance);
    fKnobResonance->setAbsolutePos(257, 43);
    fKnobResonance->setRange(0.0f, 95.0f);
    fKnobResonance->setDefault(25.0f);
    fKnobResonance->setValue(25.0f);
    fKnobResonance->setRotationAngle(305);
    fKnobResonance->setCallback(this);

    // knob Env Mod
    fKnobEnvMod = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobEnvMod->setId(DistrhoPluginNekobi::paramEnvMod);
    fKnobEnvMod->setAbsolutePos(329, 43);
    fKnobEnvMod->setRange(0.0f, 100.0f);
    fKnobEnvMod->setDefault(50.0f);
    fKnobEnvMod->setValue(50.0f);
    fKnobEnvMod->setRotationAngle(305);
    fKnobEnvMod->setCallback(this);

    // knob Decay
    fKnobDecay = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobDecay->setId(DistrhoPluginNekobi::paramDecay);
    fKnobDecay->setAbsolutePos(400, 43);
    fKnobDecay->setRange(0.0f, 100.0f);
    fKnobDecay->setDefault(75.0f);
    fKnobDecay->setValue(75.0f);
    fKnobDecay->setRotationAngle(305);
    fKnobDecay->setCallback(this);

    // knob Accent
    fKnobAccent = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobAccent->setId(DistrhoPluginNekobi::paramAccent);
    fKnobAccent->setAbsolutePos(473, 43);
    fKnobAccent->setRange(0.0f, 100.0f);
    fKnobAccent->setDefault(25.0f);
    fKnobAccent->setValue(25.0f);
    fKnobAccent->setRotationAngle(305);
    fKnobAccent->setCallback(this);

    // knob Volume
    fKnobVolume = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobVolume->setId(DistrhoPluginNekobi::paramVolume);
    fKnobVolume->setAbsolutePos(545, 43);
    fKnobVolume->setRange(0.0f, 100.0f);
    fKnobVolume->setDefault(75.0f);
    fKnobVolume->setValue(75.0f);
    fKnobVolume->setRotationAngle(305);
    fKnobVolume->setCallback(this);

    // about button
    Image aboutImageNormal(Art::aboutButtonNormalData, Art::aboutButtonNormalWidth, Art::aboutButtonNormalHeight);
    Image aboutImageHover(Art::aboutButtonHoverData, Art::aboutButtonHoverWidth, Art::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(505, 5);
    fButtonAbout->setCallback(this);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUINekobi::parameterChanged(uint32_t index, float value)
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

void DistrhoUINekobi::uiIdle()
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
    editParameter(knob->getId(), true);
}

void DistrhoUINekobi::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void DistrhoUINekobi::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);
}

void DistrhoUINekobi::imageSliderDragStarted(ImageSlider* slider)
{
    editParameter(slider->getId(), true);
}

void DistrhoUINekobi::imageSliderDragFinished(ImageSlider* slider)
{
    editParameter(slider->getId(), false);
}

void DistrhoUINekobi::imageSliderValueChanged(ImageSlider* slider, float value)
{
    setParameterValue(slider->getId(), value);
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
