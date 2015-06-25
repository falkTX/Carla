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

#include "DistrhoUIMVerb.hpp"
#include "MVerb.h"

#include "font/Kh-Kangrey.h"

START_NAMESPACE_DISTRHO

namespace Art = DistrhoArtworkMVerb;

using DGL::Color;

// -----------------------------------------------------------------------

DistrhoUIMVerb::DistrhoUIMVerb()
    : UI(Art::backgroundWidth, Art::backgroundHeight),
      fImgBackground(Art::backgroundData, Art::backgroundWidth, Art::backgroundHeight, GL_BGR)
{
    // text
    fNanoText.createFontFromMemory("kh", (const uchar*)khkangrey_ttf, khkangrey_ttfSize, false);

    // knobs
    Image knobImage(Art::knobData, Art::knobWidth, Art::knobHeight);

    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::DAMPINGFREQ);
        knob->setAbsolutePos(56 + 7*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::DENSITY);
        knob->setAbsolutePos(56 + 4*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::BANDWIDTHFREQ);
        knob->setAbsolutePos(56 + 5*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::DECAY);
        knob->setAbsolutePos(56 + 6*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::PREDELAY);
        knob->setAbsolutePos(56 + 1*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::SIZE);
        knob->setAbsolutePos(56 + 3*40, 40);
        knob->setRange(5.0f, 100.0f);
        knob->setDefault(100.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::GAIN);
        knob->setAbsolutePos(56 + 8*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(75.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::MIX);
        knob->setAbsolutePos(56 + 0*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }
    {
        ImageKnob* const knob(new ImageKnob(this, knobImage, ImageKnob::Vertical));
        knob->setId(MVerb<float>::EARLYMIX);
        knob->setAbsolutePos(56 + 2*40, 40);
        knob->setRange(0.0f, 100.0f);
        knob->setDefault(50.0f);
        knob->setCallback(this);
        fKnobs.push_back(knob);
    }

    // set initial values
    programLoaded(0);
}

DistrhoUIMVerb::~DistrhoUIMVerb()
{
    for (std::vector<ImageKnob*>::iterator it=fKnobs.begin(), end=fKnobs.end(); it != end; ++it)
    {
        ImageKnob* const knob(*it);
        delete knob;
    }

    fKnobs.clear();
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUIMVerb::parameterChanged(uint32_t index, float value)
{
    fKnobs[index]->setValue(value);
}

void DistrhoUIMVerb::programLoaded(uint32_t index)
{
    switch(index)
    {
    case 0:
        fKnobs[MVerb<float>::DAMPINGFREQ]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::DENSITY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::BANDWIDTHFREQ]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::DECAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::PREDELAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::GAIN]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::MIX]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::EARLYMIX]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::SIZE]->setValue(0.75f*100.0f);
        break;
    case 1:
        fKnobs[MVerb<float>::DAMPINGFREQ]->setValue(0.9f*100.0f);
        fKnobs[MVerb<float>::DENSITY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::BANDWIDTHFREQ]->setValue(0.1f*100.0f);
        fKnobs[MVerb<float>::DECAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::PREDELAY]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::SIZE]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::GAIN]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::MIX]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::EARLYMIX]->setValue(0.75f*100.0f);
        break;
    case 2:
        fKnobs[MVerb<float>::DAMPINGFREQ]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::DENSITY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::BANDWIDTHFREQ]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::DECAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::PREDELAY]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::SIZE]->setValue(0.25f*100.0f);
        fKnobs[MVerb<float>::GAIN]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::MIX]->setValue(0.35f*100.0f);
        fKnobs[MVerb<float>::EARLYMIX]->setValue(0.75f*100.0f);
        break;
    case 3:
        fKnobs[MVerb<float>::DAMPINGFREQ]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::DENSITY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::BANDWIDTHFREQ]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::DECAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::PREDELAY]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::SIZE]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::GAIN]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::MIX]->setValue(0.35f*100.0f);
        fKnobs[MVerb<float>::EARLYMIX]->setValue(0.75f*100.0f);
        break;
    case 4:
        fKnobs[MVerb<float>::DAMPINGFREQ]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::DENSITY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::BANDWIDTHFREQ]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::DECAY]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::PREDELAY]->setValue(0.0f*100.0f);
        fKnobs[MVerb<float>::SIZE]->setValue(0.5f*100.0f);
        fKnobs[MVerb<float>::GAIN]->setValue(1.0f*100.0f);
        fKnobs[MVerb<float>::MIX]->setValue(0.15f*100.0f);
        fKnobs[MVerb<float>::EARLYMIX]->setValue(0.75f*100.0f);
        break;
    }
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUIMVerb::imageKnobDragStarted(ImageKnob* knob)
{
    editParameter(knob->getId(), true);
}

void DistrhoUIMVerb::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void DistrhoUIMVerb::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);
}

void DistrhoUIMVerb::onDisplay()
{
    fImgBackground.draw();

    // text display
    fNanoText.beginFrame(this);

    fNanoText.fontFace("kh");
    fNanoText.fontSize(20);
    fNanoText.textAlign(NanoVG::ALIGN_CENTER|NanoVG::ALIGN_TOP);
    fNanoText.fillColor(Color(1.0f, 1.0f, 1.0f));

    char strBuf[32+1];
    strBuf[32] = '\0';

    for (std::size_t i=0; i<MVerb<float>::NUM_PARAMS; ++i)
    {
        std::snprintf(strBuf, 32, "%i%%", int(fKnobs[i]->getValue()));
        fNanoText.textBox(58.0f + float(fKnobs[i]->getAbsoluteX()) - 56.0f, 73.0f, 30.0f, strBuf, nullptr);
    }

    fNanoText.endFrame();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUIMVerb();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
