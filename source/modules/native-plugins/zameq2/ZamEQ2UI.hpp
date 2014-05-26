/*
 * ZamEQ2 2 band parametric equaliser
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

#ifndef ZAMEQ2UI_HPP_INCLUDED
#define ZAMEQ2UI_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "ZamEQ2Artwork.hpp"
#include "ZamEQ2Plugin.hpp"

#define EQPOINTS 1000
#include <complex>

using DGL::Image;
using DGL::ImageKnob;
using DGL::ImageSlider;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZamEQ2UI : public UI,
                 public ImageKnob::Callback,
                 public ImageSlider::Callback
{
public:
    ZamEQ2UI();

protected:
    // -------------------------------------------------------------------
    // Information

    uint d_getWidth() const noexcept override
    {
        return ZamEQ2Artwork::zameq2Width;
    }

    uint d_getHeight() const noexcept override
    {
        return ZamEQ2Artwork::zameq2Height;
    }

	inline double
	to_dB(double g) {
	        return (20.*log10(g));
	}

	inline double
	from_dB(double gdb) {
	        return (exp(gdb/20.*log(10.)));
	}

	inline double
	sanitize_denormal(double value) {
	        if (!std::isnormal(value)) {
	                return (0.);
	        }
	        return value;
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

	void calceqcurve(float x[], float y[]);
	void peq(int i, int ch, float srate, float fc, float g, float bw);
	void lowshelf(int i, int ch, float srate, float fc, float g);
	void highshelf(int i, int ch, float srate, float fc, float g);

    // -------------------------------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value) override;
    void d_programChanged(uint32_t index) override;

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
    ScopedPointer<ImageKnob> fKnobGain1;
    ScopedPointer<ImageKnob> fKnobQ1, fKnobFreq1, fKnobGain2;
    ScopedPointer<ImageKnob> fKnobQ2, fKnobFreq2;
    ScopedPointer<ImageKnob> fKnobGainL, fKnobFreqL;
    ScopedPointer<ImageKnob> fKnobGainH, fKnobFreqH;
    ScopedPointer<ImageSlider> fSliderMaster;
    float eqx[EQPOINTS];
    float eqy[EQPOINTS];
    DGL::Rectangle<int> fCanvasArea;
    double a1[1][MAX_FILT], a2[1][MAX_FILT], b0[1][MAX_FILT], b1[1][MAX_FILT], b2[1][MAX_FILT];
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // ZAMEQ2UI_HPP_INCLUDED
