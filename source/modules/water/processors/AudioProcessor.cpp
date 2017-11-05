/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

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
    cachedTotalIns  = 0;
    cachedTotalOuts = 0;

//     playHead = nullptr;
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
// void AudioProcessor::setPlayHead (AudioPlayHead* const newPlayHead)
// {
//     playHead = newPlayHead;
// }

void AudioProcessor::setPlayConfigDetails (const int newNumIns,
                                           const int newNumOuts,
                                           const double newSampleRate,
                                           const int newBlockSize)
{
    cachedTotalIns = newNumIns;
    cachedTotalOuts = newNumOuts;
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

void AudioProcessor::processBypassed (AudioSampleBuffer& buffer, MidiBuffer&)
{
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

void AudioProcessor::processBlockBypassed (AudioSampleBuffer&  buffer, MidiBuffer& midi)    { processBypassed (buffer, midi); }

#if 0
//==============================================================================
bool AudioPlayHead::CurrentPositionInfo::operator== (const CurrentPositionInfo& other) const noexcept
{
    return timeInSamples == other.timeInSamples
        && ppqPosition == other.ppqPosition
        && editOriginTime == other.editOriginTime
        && ppqPositionOfLastBarStart == other.ppqPositionOfLastBarStart
        && frameRate == other.frameRate
        && isPlaying == other.isPlaying
        && isRecording == other.isRecording
        && bpm == other.bpm
        && timeSigNumerator == other.timeSigNumerator
        && timeSigDenominator == other.timeSigDenominator
        && ppqLoopStart == other.ppqLoopStart
        && ppqLoopEnd == other.ppqLoopEnd
        && isLooping == other.isLooping;
}

bool AudioPlayHead::CurrentPositionInfo::operator!= (const CurrentPositionInfo& other) const noexcept
{
    return ! operator== (other);
}

void AudioPlayHead::CurrentPositionInfo::resetToDefault()
{
    zerostruct (*this);
    timeSigNumerator = 4;
    timeSigDenominator = 4;
    bpm = 120;
}

#endif

}
