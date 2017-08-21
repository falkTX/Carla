/*
  ZynAddSubFX - a software synthesizer

  AudioOut.h - Audio Output superclass
  Copyright (C) 2009-2010 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <iostream>
#include <cstring>
#include "SafeQueue.h"

#include "OutMgr.h"
#include "../Misc/Master.h"
#include "AudioOut.h"

using namespace std;

namespace zyncarla {

AudioOut::AudioOut(const SYNTH_T &synth_)
    :synth(synth_), samplerate(synth.samplerate), bufferSize(synth.buffersize)
{}

AudioOut::~AudioOut()
{}

void AudioOut::setSamplerate(int _samplerate)
{
    samplerate = _samplerate;
}

int AudioOut::getSampleRate()
{
    return samplerate;
}

void AudioOut::setBufferSize(int _bufferSize)
{
    bufferSize = _bufferSize;
}

const Stereo<float *> AudioOut::getNext()
{
    return OutMgr::getInstance().tick(bufferSize);
}

}
