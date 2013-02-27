/*
 * DISTRHO 3BandEQ Plugin, based on 3BandEQ by Michael Gruhn
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_UI_3BANDEQ_HPP__
#define __DISTRHO_UI_3BANDEQ_HPP__

#include "DistrhoUIOpenGL.hpp"
#include "dgl/ImageButton.hpp"
#include "dgl/ImageKnob.hpp"
#include "dgl/ImageSlider.hpp"

#include "DistrhoArtwork3BandEQ.hpp"
#include "DistrhoPlugin3BandEQ.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class DistrhoUI3BandEQ : public OpenGLUI,
                         public ImageButton::Callback,
                         public ImageKnob::Callback,
                         public ImageSlider::Callback
{
public:
    DistrhoUI3BandEQ();
    ~DistrhoUI3BandEQ();

protected:
    // ---------------------------------------------
    // Information

    unsigned int d_width() const
    {
        return DistrhoArtwork3BandEQ::backgroundWidth;
    }

    unsigned int d_height() const
    {
        return DistrhoArtwork3BandEQ::backgroundHeight;
    }

    // ---------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value);
    void d_programChanged(uint32_t index);

    // ---------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int);
    void imageKnobDragStarted(ImageKnob* knob);
    void imageKnobDragFinished(ImageKnob* knob);
    void imageKnobValueChanged(ImageKnob* knob, float value);
    void imageSliderDragStarted(ImageSlider* slider);
    void imageSliderDragFinished(ImageSlider* slider);
    void imageSliderValueChanged(ImageSlider* slider, float value);

    void onDisplay();

private:
    Image fImgBackground;

    ImageSlider* fSliderLow;
    ImageSlider* fSliderMid;
    ImageSlider* fSliderHigh;
    ImageSlider* fSliderMaster;
    ImageKnob*   fKnobLowMid;
    ImageKnob*   fKnobMidHigh;
    ImageButton* fButtonAbout;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_3BANDEQ_HPP__
