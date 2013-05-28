/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __DISTRHO_UI_3BANDEQ_HPP__
#define __DISTRHO_UI_3BANDEQ_HPP__

#include "DistrhoUIOpenGL.hpp"
#include "dgl/ImageButton.hpp"
#include "dgl/ImageKnob.hpp"
#include "dgl/ImageSlider.hpp"

#include "DistrhoArtworkNekobi.hpp"
#include "DistrhoPluginNekobi.hpp"
#include "NekoWidget.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class DistrhoUINekobi : public OpenGLUI,
                        public ImageButton::Callback,
                        public ImageKnob::Callback,
                        public ImageSlider::Callback
{
public:
    DistrhoUINekobi();
    ~DistrhoUINekobi() override;

protected:
    // ---------------------------------------------
    // Information

    unsigned int d_width() const override
    {
        return DistrhoArtworkNekobi::backgroundWidth;
    }

    unsigned int d_height() const override
    {
        return DistrhoArtworkNekobi::backgroundHeight;
    }

    // ---------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;
    void d_noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) override;

    // ---------------------------------------------
    // UI Callbacks

    void d_uiIdle() override;

    // ---------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;
    void imageSliderDragStarted(ImageSlider* slider) override;
    void imageSliderDragFinished(ImageSlider* slider) override;
    void imageSliderValueChanged(ImageSlider* slider, float value) override;

    void onDisplay() override;

private:
    Image      fImgBackground;
    NekoWidget fNeko;

    ImageKnob* fKnobTuning;
    ImageKnob* fKnobCutoff;
    ImageKnob* fKnobResonance;
    ImageKnob* fKnobEnvMod;
    ImageKnob* fKnobDecay;
    ImageKnob* fKnobAccent;
    ImageKnob* fKnobVolume;

    ImageSlider* fSliderWaveform;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_3BANDEQ_HPP__
