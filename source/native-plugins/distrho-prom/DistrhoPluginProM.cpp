/*
 * DISTRHO ProM Plugin
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LICENSE file.
 */

#include "DistrhoPluginProM.hpp"

#include "libprojectM/projectM.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoPluginProM::DistrhoPluginProM()
    : Plugin(0, 0, 0),
      fPM(nullptr)
{
}

DistrhoPluginProM::~DistrhoPluginProM()
{
    DISTRHO_SAFE_ASSERT(fPM == nullptr);
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginProM::initParameter(uint32_t, Parameter&)
{
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginProM::getParameterValue(uint32_t) const
{
    return 0.0f;
}

void DistrhoPluginProM::setParameterValue(uint32_t, float)
{
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginProM::run(const float** inputs, float** outputs, uint32_t frames)
{
    const float* in  = inputs[0];
    float*       out = outputs[0];

    if (out != in)
        std::memcpy(out, in, sizeof(float)*frames);

    const MutexLocker csm(fMutex);

    if (fPM == nullptr)
        return;

    if (PCM* const pcm = const_cast<PCM*>(fPM->pcm()))
        pcm->addPCMfloat(in, frames);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginProM();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
