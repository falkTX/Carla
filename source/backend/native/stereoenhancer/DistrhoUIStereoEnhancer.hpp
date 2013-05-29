/*
 * DISTRHO StereoEnhancer Plugin, based on StereoEnhancer by Michael Gruhn
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

#include "dgl/ImageAboutWindow.hpp"
#include "dgl/ImageButton.hpp"
#include "dgl/ImageKnob.hpp"

#include "DistrhoArtworkStereoEnhancer.hpp"
#include "DistrhoPluginStereoEnhancer.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class DistrhoUIStereoEnhancer : public OpenGLUI,
                                public ImageButton::Callback,
                                public ImageKnob::Callback
{
public:
    DistrhoUIStereoEnhancer();
    ~DistrhoUIStereoEnhancer() override;

protected:
    // ---------------------------------------------
    // Information

    unsigned int d_width() const override
    {
        return DistrhoArtworkStereoEnhancer::backgroundWidth;
    }

    unsigned int d_height() const override
    {
        return DistrhoArtworkStereoEnhancer::backgroundHeight;
    }

    // ---------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;

    // ---------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void onDisplay() override;

private:
    Image fImgBackground;
    ImageAboutWindow fAboutWindow;

    ImageKnob* fKnobWidthLows;
    ImageKnob* fKnobWidthHighs;
    ImageKnob* fKnobCrossover;
    ImageButton* fButtonAbout;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_3BANDEQ_HPP__
