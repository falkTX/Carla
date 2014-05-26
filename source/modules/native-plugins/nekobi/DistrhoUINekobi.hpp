/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#ifndef DISTRHO_UI_NEKOBI_HPP_INCLUDED
#define DISTRHO_UI_NEKOBI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageAboutWindow.hpp"
#include "ImageButton.hpp"
#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "DistrhoArtworkNekobi.hpp"
#include "NekoWidget.hpp"

using DGL::ImageAboutWindow;
using DGL::ImageButton;
using DGL::ImageKnob;
using DGL::ImageSlider;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoUINekobi : public UI,
                        public ImageButton::Callback,
                        public ImageKnob::Callback,
                        public ImageSlider::Callback
{
public:
    DistrhoUINekobi();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return DistrhoArtworkNekobi::backgroundWidth;
    }

    uint d_getHeight() const noexcept override
    {
        return DistrhoArtworkNekobi::backgroundHeight;
    }

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // UI Callbacks

    void d_uiIdle() override;

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
    Image            fImgBackground;
    ImageAboutWindow fAboutWindow;
    NekoWidget       fNeko;

    ScopedPointer<ImageButton> fButtonAbout;
    ScopedPointer<ImageSlider> fSliderWaveform;
    ScopedPointer<ImageKnob> fKnobTuning, fKnobCutoff, fKnobResonance;
    ScopedPointer<ImageKnob> fKnobEnvMod, fKnobDecay, fKnobAccent, fKnobVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoUINekobi)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_NEKOBI_HPP_INCLUDED
