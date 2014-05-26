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

    void updateSynth();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return SegmentJuiceArtwork::backgroundWidth;
    }

    uint d_getHeight() const noexcept override
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

    ScopedPointer<ImageButton> fButtonAbout;
    ScopedPointer<ImageKnob> fKnobWave1, fKnobWave2, fKnobWave3, fKnobWave4, fKnobWave5, fKnobWave6;
    ScopedPointer<ImageKnob> fKnobFM1, fKnobFM2, fKnobFM3, fKnobFM4, fKnobFM5, fKnobFM6;
    ScopedPointer<ImageKnob> fKnobPan1, fKnobPan2, fKnobPan3, fKnobPan4, fKnobPan5, fKnobPan6;
    ScopedPointer<ImageKnob> fKnobAmp1, fKnobAmp2, fKnobAmp3, fKnobAmp4, fKnobAmp5, fKnobAmp6;
    ScopedPointer<ImageKnob> fKnobAttack, fKnobDecay, fKnobSustain, fKnobRelease;
    ScopedPointer<ImageKnob> fKnobStereo, fKnobTune, fKnobVolume, fKnobGlide;

    CSynth synthL, synthR;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // WOBBLEJUICEUI_HPP_INCLUDED
