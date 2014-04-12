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

#ifndef WOBBLEJUICEUI_HPP_INCLUDED
#define WOBBLEJUICEUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageAboutWindow.hpp"
#include "ImageButton.hpp"
#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "SegmentJuiceArtwork.hpp"
#include "SegmentJuicePlugin.hpp"
#include <iostream>
#include "Synth.hxx"

using DGL::Image;
using DGL::ImageAboutWindow;
using DGL::ImageButton;
using DGL::ImageKnob;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class SegmentJuiceUI : public UI,
                       public ImageButton::Callback,
                       public ImageKnob::Callback
{
public:
    SegmentJuiceUI();
    ~SegmentJuiceUI() override;

    void updateSynth();

protected:
    // -------------------------------------------------------------------
    // Information

    unsigned int d_getWidth() const noexcept override
    {
        return SegmentJuiceArtwork::backgroundWidth;
    }

    unsigned int d_getHeight() const noexcept override
    {
        return SegmentJuiceArtwork::backgroundHeight;
    }

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void onDisplay() override;

private:
    Image fImgBackground;
    ImageAboutWindow fAboutWindow;

    ImageKnob* fKnobWave1;
    ImageKnob* fKnobWave2;
    ImageKnob* fKnobWave3;
    ImageKnob* fKnobWave4;
    ImageKnob* fKnobWave5;
    ImageKnob* fKnobWave6;

    ImageKnob* fKnobFM1;
    ImageKnob* fKnobFM2;
    ImageKnob* fKnobFM3;
    ImageKnob* fKnobFM4;
    ImageKnob* fKnobFM5;
    ImageKnob* fKnobFM6;

    ImageKnob* fKnobPan1;
    ImageKnob* fKnobPan2;
    ImageKnob* fKnobPan3;
    ImageKnob* fKnobPan4;
    ImageKnob* fKnobPan5;
    ImageKnob* fKnobPan6;

    ImageKnob* fKnobAmp1;
    ImageKnob* fKnobAmp2;
    ImageKnob* fKnobAmp3;
    ImageKnob* fKnobAmp4;
    ImageKnob* fKnobAmp5;
    ImageKnob* fKnobAmp6;

    ImageKnob* fKnobAttack;
    ImageKnob* fKnobDecay;
    ImageKnob* fKnobSustain;
    ImageKnob* fKnobRelease;
    ImageKnob* fKnobStereo;
    ImageKnob* fKnobTune;
    ImageKnob* fKnobVolume;
    ImageKnob* fKnobGlide;
    ImageButton* fButtonAbout;

    CSynth synthL, synthR;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // WOBBLEJUICEUI_HPP_INCLUDED
