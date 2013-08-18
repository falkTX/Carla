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
 * For a full copy of the license see the doc/LGPL.txt file.
 */

#ifndef DISTRHO_UI_3BANDEQ_HPP_INCLUDED
#define DISTRHO_UI_3BANDEQ_HPP_INCLUDED

#include "DistrhoUIOpenGL.hpp"

#include "dgl/ImageAboutWindow.hpp"
#include "dgl/ImageButton.hpp"
#include "dgl/ImageKnob.hpp"
#include "dgl/ImageSlider.hpp"

#include "DistrhoArtwork3BandEQ.hpp"
#include "DistrhoPlugin3BandEQ.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoUI3BandEQ : public OpenGLUI,
                         public ImageButton::Callback,
                         public ImageKnob::Callback,
                         public ImageSlider::Callback
{
public:
    DistrhoUI3BandEQ();
    ~DistrhoUI3BandEQ() override;

protected:
    // -------------------------------------------------------------------
    // Information

    unsigned int d_getWidth() const noexcept override
    {
        return DistrhoArtwork3BandEQ::backgroundWidth;
    }

    unsigned int d_getHeight() const noexcept override
    {
        return DistrhoArtwork3BandEQ::backgroundHeight;
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
    void imageSliderDragStarted(ImageSlider* slider) override;
    void imageSliderDragFinished(ImageSlider* slider) override;
    void imageSliderValueChanged(ImageSlider* slider, float value) override;

    void onDisplay() override;

private:
    Image fImgBackground;
    ImageAboutWindow fAboutWindow;

    ImageSlider* fSliderLow;
    ImageSlider* fSliderMid;
    ImageSlider* fSliderHigh;
    ImageSlider* fSliderMaster;
    ImageKnob*   fKnobLowMid;
    ImageKnob*   fKnobMidHigh;
    ImageButton* fButtonAbout;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_3BANDEQ_HPP_INCLUDED
