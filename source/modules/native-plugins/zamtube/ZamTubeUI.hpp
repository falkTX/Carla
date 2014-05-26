/*
 * ZamTube triode WDF distortion model
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

#ifndef ZAMTUBEUI_HPP_INCLUDED
#define ZAMTUBEUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "ZamTubeArtwork.hpp"

using DGL::Image;
using DGL::ImageKnob;
using DGL::ImageSlider;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZamTubeUI : public UI,
                  public ImageKnob::Callback,
                  public ImageSlider::Callback
{
public:
    ZamTubeUI();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return ZamTubeArtwork::zamtubeWidth;
    }

    uint d_getHeight() const noexcept override
    {
        return ZamTubeArtwork::zamtubeHeight;
    }

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) ;
    void d_programChanged(uint32_t index) ;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void imageSliderDragStarted(ImageSlider* slider) override;
    void imageSliderDragFinished(ImageSlider* slider) override;
    void imageSliderValueChanged(ImageSlider* slider, float value) override;

    void onDisplay() override;

private:
    Image fImgBackground;
    ScopedPointer<ImageSlider> fSliderNotch;
    ScopedPointer<ImageKnob> fKnobTube, fKnobBass, fKnobMids, fKnobTreb, fKnobGain;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // ZAMTUBEUI_HPP_INCLUDED
