/*
 * Power Juice Plugin
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
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

#ifndef POWERJUICEUI_HPP_INCLUDED
#define POWERJUICEUI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageAboutWindow.hpp"
#include "ImageButton.hpp"
#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "PowerJuiceArtwork.hpp"
#include "PowerJuicePlugin.hpp"

#include <cmath>

using DGL::Image;
using DGL::ImageAboutWindow;
using DGL::ImageButton;
using DGL::ImageKnob;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class PowerJuiceUI : public UI,
                     public ImageButton::Callback,
                     public ImageKnob::Callback
{
public:
    PowerJuiceUI();
    ~PowerJuiceUI() override;

protected:
    // -------------------------------------------------------------------
    // Information

    unsigned int d_getWidth() const noexcept override
    {
        return PowerJuiceArtwork::backgroundWidth;
    }

    unsigned int d_getHeight() const noexcept override
    {
        return PowerJuiceArtwork::backgroundHeight;
    }

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;

    // -------------------------------------------------------------------
    // UI Callbacks

    void d_uiIdle() override;

    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void imageKnobDragStarted(ImageKnob* knob) override;
    void imageKnobDragFinished(ImageKnob* knob) override;
    void imageKnobValueChanged(ImageKnob* knob, float value) override;
    void onDisplay() override;

private:
    Image fImgBackground;
    ImageAboutWindow fAboutWindow;

    ImageKnob* fKnobAttack;
    ImageKnob* fKnobRelease;
    ImageKnob* fKnobThreshold;
    ImageKnob* fKnobRatio;
    ImageKnob* fKnobMakeup;
    ImageKnob* fKnobMix;
    ImageButton* fButtonAbout;

    PowerJuicePlugin* dsp;

    float fromDB(float gdb) {
        return (std::exp(gdb/20.f*std::log(10.f)));
    };

    float toDB(float g) {
        return (20.f*std::log10(g));
    }

    float toIEC(float db) {
         float def = 0.0f; /* Meter deflection %age */

         if (db < -70.0f) {
                 def = 0.0f;
         } else if (db < -60.0f) {
                 def = (db + 70.0f) * 0.25f;
         } else if (db < -50.0f) {
                 def = (db + 60.0f) * 0.5f + 5.0f;
         } else if (db < -40.0f) {
                 def = (db + 50.0f) * 0.75f + 7.5;
         } else if (db < -30.0f) {
                 def = (db + 40.0f) * 1.5f + 15.0f;
         } else if (db < -20.0f) {
                 def = (db + 30.0f) * 2.0f + 30.0f;
         } else if (db < 0.0f) {
                 def = (db + 20.0f) * 2.5f + 50.0f;
         } else {
                 def = 100.0f;
         }

         return (def * 2.0f);
    }
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // POWERJUICEUI_HPP_INCLUDED
