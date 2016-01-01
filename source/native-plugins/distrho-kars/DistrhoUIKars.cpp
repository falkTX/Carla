/*
 * DISTRHO Kars Plugin, based on karplong by Chris Cannam.
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginKars.hpp"
#include "DistrhoUIKars.hpp"

START_NAMESPACE_DISTRHO

namespace Art = DistrhoArtworkKars;

// -----------------------------------------------------------------------

DistrhoUIKars::DistrhoUIKars()
    : UI(Art::backgroundWidth, Art::backgroundHeight),
      fImgBackground(Art::backgroundData, Art::backgroundWidth, Art::backgroundHeight)
{
    // sustain switch
    Image switchImageNormal(Art::switchData, Art::switchWidth, Art::switchHeight/2);
    Image switchImageDown(Art::switchData+(Art::switchWidth*Art::switchHeight/2*4), Art::switchWidth, Art::switchHeight/2);
    fSwitchSustain = new ImageSwitch(this, switchImageNormal, switchImageDown);
    fSwitchSustain->setAbsolutePos(Art::backgroundWidth/2-Art::switchWidth/2, Art::backgroundHeight/2-Art::switchHeight/4);
    fSwitchSustain->setId(DistrhoPluginKars::paramSustain);
    fSwitchSustain->setCallback(this);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUIKars::parameterChanged(uint32_t index, float value)
{
    if (index != 0)
        return;

    fSwitchSustain->setDown(value > 0.5f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUIKars::imageSwitchClicked(ImageSwitch* imageSwitch, bool down)
{
    if (imageSwitch != fSwitchSustain)
        return;

    editParameter(DistrhoPluginKars::paramSustain, true);
    setParameterValue(DistrhoPluginKars::paramSustain, down ? 1.0f : 0.0f);
    editParameter(DistrhoPluginKars::paramSustain, false);
}

void DistrhoUIKars::onDisplay()
{
    fImgBackground.draw();
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUIKars();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
