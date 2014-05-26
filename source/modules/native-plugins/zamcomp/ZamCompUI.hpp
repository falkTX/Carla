/*
 * ZamComp mono compressor
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

#ifndef ZAMCOMPUI_HPP_INCLUDED
#define ZAMCOMPUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageKnob.hpp"

#include "ZamCompArtwork.hpp"

using DGL::Image;
using DGL::ImageKnob;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZamCompUI : public UI,
                  public ImageKnob::Callback
{
public:
    ZamCompUI();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return ZamCompArtwork::zamcompWidth;
    }

    uint d_getHeight() const noexcept override
    {
        return ZamCompArtwork::zamcompHeight;
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

    void onDisplay() override;

private:
    Image fImgBackground;
    ScopedPointer<ImageKnob> fKnobAttack, fKnobRelease, fKnobThresh;
    ScopedPointer<ImageKnob> fKnobRatio, fKnobKnee, fKnobMakeup;

    Image fLedRedImg;
    float fLedRedValue;
    Image fLedYellowImg;
    float fLedYellowValue;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // ZAMCOMPUI_HPP_INCLUDED
