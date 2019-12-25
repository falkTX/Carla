/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the GNU
   General Public License as published by the Free Software Foundation;
   either version 2 of the License, or any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

   For a full copy of the GNU General Public License see the doc/GPL.txt file.

  ==============================================================================
*/

#include "AudioProcessor.h"

namespace water {

AudioProcessor::AudioProcessor()
{
    numAudioIns = 0;
    numAudioOuts = 0;
    numCVIns = 0;
    numCVOuts = 0;
    numMIDIIns = 0;
    numMIDIOuts = 0;

    currentSampleRate = 0;
    blockSize = 0;
    latencySamples = 0;

    suspended = false;
    nonRealtime = false;
}

AudioProcessor::~AudioProcessor()
{
}

//==============================================================================
void AudioProcessor::setPlayConfigDetails (const uint newNumIns,
                                           const uint newNumOuts,
                                           const uint newNumCVIns,
                                           const uint newNumCVOuts,
                                           const uint newNumMIDIIns,
                                           const uint newNumMIDIOuts,
                                           const double newSampleRate,
                                           const int newBlockSize)
{
    numAudioIns = newNumIns;
    numAudioOuts = newNumOuts;
    numCVIns = newNumCVIns;
    numCVOuts = newNumCVOuts;
    numMIDIIns = newNumMIDIIns;
    numMIDIOuts = newNumMIDIOuts;
    setRateAndBufferSizeDetails (newSampleRate, newBlockSize);
}

void AudioProcessor::setRateAndBufferSizeDetails (double newSampleRate, int newBlockSize) noexcept
{
    currentSampleRate = newSampleRate;
    blockSize = newBlockSize;
}

//==============================================================================
void AudioProcessor::setNonRealtime (const bool newNonRealtime) noexcept
{
    nonRealtime = newNonRealtime;
}

void AudioProcessor::setLatencySamples (const int newLatency)
{
    if (latencySamples != newLatency)
        latencySamples = newLatency;
}

void AudioProcessor::suspendProcessing (const bool shouldBeSuspended)
{
    const CarlaRecursiveMutexLocker cml (callbackLock);
    suspended = shouldBeSuspended;
}

void AudioProcessor::reset() {}
void AudioProcessor::reconfigure() {}

uint AudioProcessor::getTotalNumInputChannels(ChannelType t) const noexcept
{
    switch (t)
    {
    case ChannelTypeAudio:
        return numAudioIns;
    case ChannelTypeCV:
        return numCVIns;
    case ChannelTypeMIDI:
        return numMIDIIns;
    }

    return 0;
}

uint AudioProcessor::getTotalNumOutputChannels(ChannelType t) const noexcept
{
    switch (t)
    {
    case ChannelTypeAudio:
        return numAudioOuts;
    case ChannelTypeCV:
        return numCVOuts;
    case ChannelTypeMIDI:
        return numMIDIOuts;
    }

    return 0;
}

const String AudioProcessor::getInputChannelName(ChannelType t, uint) const
{
    return t == ChannelTypeMIDI ? "events-in" : "";
}

const String AudioProcessor::getOutputChannelName(ChannelType t, uint) const
{
    return t == ChannelTypeMIDI ? "events-out" : "";
}

}
