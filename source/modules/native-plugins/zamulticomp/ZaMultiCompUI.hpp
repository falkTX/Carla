/*
 * ZaMultiComp mono multiband compressor
 * Copyright (C) 2014  Damien Zammit <damien@zamaudio.com> 
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

#ifndef ZAMULTICOMPUI_HPP_INCLUDED
#define ZAMULTICOMPUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageKnob.hpp"
#include "ImageToggle.hpp"

#include "ZaMultiCompArtwork.hpp"
#include "ZaMultiCompPlugin.hpp"

using DGL::Image;
using DGL::ImageKnob;
using DGL::ImageToggle;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZaMultiCompUI : public UI,
                  public ImageKnob::Callback,
                  public ImageToggle::Callback
{
public:
    ZaMultiCompUI();
    ~ZaMultiCompUI() override;

protected:
    // -------------------------------------------------------------------
    // Information

    unsigned int d_getWidth() const noexcept override
    {
        return ZaMultiCompArtwork::zamulticompWidth;
    }

    unsigned int d_getHeight() const noexcept override
    {
        return ZaMultiCompArtwork::zamulticompHeight;
    }

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void imageToggleClicked(ImageToggle* slider, int button) override;

    void onDisplay() override;

private:
    Image fImgBackground;
    ImageKnob* fKnobAttack;
    ImageKnob* fKnobRelease;
    ImageKnob* fKnobThresh;
    ImageKnob* fKnobRatio;
    ImageKnob* fKnobKnee;
    ImageKnob* fKnobGlobalGain;
    ImageKnob* fKnobMakeup1;
    ImageKnob* fKnobMakeup2;
    ImageKnob* fKnobMakeup3;
    ImageKnob* fKnobXover1;
    ImageKnob* fKnobXover2;
    ImageToggle* fToggleBypass1;
    ImageToggle* fToggleBypass2;
    ImageToggle* fToggleBypass3;
    ImageToggle* fToggleListen1;
    ImageToggle* fToggleListen2;
    ImageToggle* fToggleListen3;

    Image fLedRedImg;
    float fLedRedValue1;
    float fLedRedValue2;
    float fLedRedValue3;
    Image fLedYellowImg;
    float fLedYellowValue;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // ZAMULTICOMPUI_HPP_INCLUDED
