/*
 * DISTRHO 3BandSplitter Plugin, based on 3BandSplitter by Michael Gruhn
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_UI_3BANDSPLITTER_HPP__
#define __DISTRHO_UI_3BANDSPLITTER_HPP__

#include "DistrhoUIOpenGLExt.h"

#include "DistrhoArtwork3BandSplitter.hpp"
#include "DistrhoPlugin3BandSplitter.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class DistrhoUI3BandSplitter : public OpenGLExtUI
{
public:
    DistrhoUI3BandSplitter();
    ~DistrhoUI3BandSplitter();

    // ---------------------------------------------
protected:

    // Information
    unsigned int d_width()
    {
        return DistrhoArtwork3BandSplitter::backgroundWidth;
    }

    unsigned int d_height()
    {
        return DistrhoArtwork3BandSplitter::backgroundHeight;
    }

    // DSP Callbacks
    void d_parameterChanged(uint32_t index, float value);
    void d_programChanged(uint32_t index);

    // Extended Callbacks
    void imageButtonClicked(ImageButton* button);
    void imageKnobDragStarted(ImageKnob* knob);
    void imageKnobDragFinished(ImageKnob* knob);
    void imageKnobValueChanged(ImageKnob* knob, float value);
    void imageSliderDragStarted(ImageSlider* slider);
    void imageSliderDragFinished(ImageSlider* slider);
    void imageSliderValueChanged(ImageSlider* slider, float value);

private:
    ImageSlider* sliderLow;
    ImageSlider* sliderMid;
    ImageSlider* sliderHigh;
    ImageSlider* sliderMaster;
    ImageKnob* knobLowMid;
    ImageKnob* knobMidHigh;
    ImageButton* buttonAbout;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_3BANDSPLITTER_HPP__
