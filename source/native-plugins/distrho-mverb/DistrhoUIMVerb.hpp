/*
 * DISTRHO MVerb, a DPF'ied MVerb.
 * Copyright (c) 2010 Martin Eastwood
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#ifndef DISTRHO_UI_MVERB_HPP_INCLUDED
#define DISTRHO_UI_MVERB_HPP_INCLUDED

#include "DistrhoUI.hpp"
#include "NanoVG.hpp"
#include "ImageWidgets.hpp"

#include "DistrhoArtworkMVerb.hpp"

#include <vector>

using DGL::Image;
using DGL::ImageKnob;
using DGL::NanoVG;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoUIMVerb : public UI,
                       public ImageKnob::Callback
{
public:
    DistrhoUIMVerb();
    ~DistrhoUIMVerb() override;

protected:
    // -------------------------------------------------------------------
    // DSP Callbacks

    void parameterChanged(uint32_t index, float value) override;
    void programLoaded(uint32_t index) override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;

    void onDisplay() override;

private:
    Image  fImgBackground;
    NanoVG fNanoText;
    std::vector<ImageKnob*> fKnobs;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoUIMVerb)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_MVERB_HPP_INCLUDED
