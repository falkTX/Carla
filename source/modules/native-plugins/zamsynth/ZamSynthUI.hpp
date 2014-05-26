/*
 * ZamSynth polyphonic synthesiser
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

#ifndef ZAMSYNTHUI_HPP_INCLUDED
#define ZAMSYNTHUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageButton.hpp"
#include "ImageKnob.hpp"
#include "ImageToggle.hpp"

#include "ZamSynthArtwork.hpp"

#define AREAHEIGHT 250

using DGL::Image;
using DGL::ImageButton;
using DGL::ImageKnob;
using DGL::ImageToggle;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZamSynthUI : public UI,
                   public ImageButton::Callback,
                   public ImageKnob::Callback,
                   public ImageToggle::Callback
{
public:
    ZamSynthUI();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return ZamSynthArtwork::zamsynthWidth;
    }

    uint d_getHeight() const noexcept override
    {
        return ZamSynthArtwork::zamsynthHeight;
    }

    void gaussiansmooth(float* smoothed, float* xs, float* ys, int n, int radius);

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;
    void d_stateChanged(const char*, const char*) override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void imageButtonClicked(ImageButton* button, int) override;
    void imageToggleClicked(ImageToggle* toggle, int) override;

    void onDisplay() override;
    bool onMouse(const MouseEvent&) override;
    bool onMotion(const MotionEvent&) override;

private:
    Image fImgBackground;
    ScopedPointer<ImageKnob> fKnobGain, fKnobSpeed;
    ScopedPointer<ImageButton> fButtonSmooth;
    ScopedPointer<ImageToggle> fToggleGraph;
    float wave_y[AREAHEIGHT];
    float env_y[AREAHEIGHT];
    bool fGraph;

    bool fDragging;
    bool fDragValid;
    DGL::Rectangle<int> fCanvasArea;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // ZAMSYNTHUI_HPP_INCLUDED
