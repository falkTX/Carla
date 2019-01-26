/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#include "Synthesiser.h"

#include "../buffers/AudioSampleBuffer.h"
#include "../containers/Array.h"
#include "../midi/MidiBuffer.h"
#include "../midi/MidiMessage.h"

namespace water {
   
SynthesiserSound::SynthesiserSound() {}
SynthesiserSound::~SynthesiserSound() {}

//==============================================================================
SynthesiserVoice::SynthesiserVoice()
    : currentSampleRate (44100.0),
      currentlyPlayingNote (-1),
      currentPlayingMidiChannel (0),
      noteOnTime (0),
      keyIsDown (false),
      sustainPedalDown (false),
      sostenutoPedalDown (false)
{
}

SynthesiserVoice::~SynthesiserVoice()
{
}

bool SynthesiserVoice::isPlayingChannel (const int midiChannel) const
{
    return currentPlayingMidiChannel == midiChannel;
}

void SynthesiserVoice::setCurrentPlaybackSampleRate (const double newRate)
{
    currentSampleRate = newRate;
}

bool SynthesiserVoice::isVoiceActive() const
{
    return getCurrentlyPlayingNote() >= 0;
}

void SynthesiserVoice::clearCurrentNote()
{
    currentlyPlayingNote = -1;
    currentlyPlayingSound = nullptr;
    currentPlayingMidiChannel = 0;
}

void SynthesiserVoice::aftertouchChanged (int) {}
void SynthesiserVoice::channelPressureChanged (int) {}

bool SynthesiserVoice::wasStartedBefore (const SynthesiserVoice& other) const noexcept
{
    return noteOnTime < other.noteOnTime;
}


//==============================================================================
Synthesiser::Synthesiser()
    : sampleRate (0),
      lastNoteOnCounter (0),
      minimumSubBlockSize (32),
      subBlockSubdivisionIsStrict (false),
      shouldStealNotes (true)
{
    for (size_t i = 0; i < numElementsInArray (lastPitchWheelValues); ++i)
        lastPitchWheelValues[i] = 0x2000;

    zerostruct(sustainPedalsDown);
}

Synthesiser::~Synthesiser()
{
}

//==============================================================================
SynthesiserVoice* Synthesiser::getVoice (const int index) const
{
    return voices [index];
}

void Synthesiser::clearVoices()
{
    voices.clear();
}

SynthesiserVoice* Synthesiser::addVoice (SynthesiserVoice* const newVoice)
{
    newVoice->setCurrentPlaybackSampleRate (sampleRate);
    return voices.add (newVoice);
}

void Synthesiser::removeVoice (const int index)
{
    voices.remove (index);
}

void Synthesiser::clearSounds()
{
    sounds.clear();
}

SynthesiserSound* Synthesiser::addSound (const SynthesiserSound::Ptr& newSound)
{
    return sounds.add (newSound);
}

void Synthesiser::removeSound (const int index)
{
    sounds.remove (index);
}

void Synthesiser::setNoteStealingEnabled (const bool shouldSteal)
{
    shouldStealNotes = shouldSteal;
}

void Synthesiser::setMinimumRenderingSubdivisionSize (int numSamples, bool shouldBeStrict) noexcept
{
    wassert (numSamples > 0); // it wouldn't make much sense for this to be less than 1
    minimumSubBlockSize = numSamples;
    subBlockSubdivisionIsStrict = shouldBeStrict;
}

//==============================================================================
void Synthesiser::setCurrentPlaybackSampleRate (const double newRate)
{
    if (sampleRate != newRate)
    {
        allNotesOff (0, false);

        sampleRate = newRate;

        for (int i = voices.size(); --i >= 0;)
            voices.getUnchecked (i)->setCurrentPlaybackSampleRate (newRate);
    }
}

void Synthesiser::processNextBlock (AudioSampleBuffer& outputAudio,
                                    const MidiBuffer& midiData,
                                    int startSample,
                                    int numSamples)
{
    // must set the sample rate before using this!
    wassert (sampleRate != 0);
    const int targetChannels = outputAudio.getNumChannels();

    MidiBuffer::Iterator midiIterator (midiData);
    midiIterator.setNextSamplePosition (startSample);

    bool firstEvent = true;
    int midiEventPos;
    MidiMessage m;


    while (numSamples > 0)
    {
        if (! midiIterator.getNextEvent (m, midiEventPos))
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            return;
        }

        const int samplesToNextMidiMessage = midiEventPos - startSample;

        if (samplesToNextMidiMessage >= numSamples)
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            handleMidiEvent (m);
            break;
        }

        if (samplesToNextMidiMessage < ((firstEvent && ! subBlockSubdivisionIsStrict) ? 1 : minimumSubBlockSize))
        {
            handleMidiEvent (m);
            continue;
        }

        firstEvent = false;

        if (targetChannels > 0)
            renderVoices (outputAudio, startSample, samplesToNextMidiMessage);

        handleMidiEvent (m);
        startSample += samplesToNextMidiMessage;
        numSamples  -= samplesToNextMidiMessage;
    }

    while (midiIterator.getNextEvent (m, midiEventPos))
        handleMidiEvent (m);
}

void Synthesiser::renderVoices (AudioSampleBuffer& buffer, int startSample, int numSamples)
{
    for (int i = voices.size(); --i >= 0;)
        voices.getUnchecked (i)->renderNextBlock (buffer, startSample, numSamples);
}

void Synthesiser::handleMidiEvent (const MidiMessage& m)
{
    const int channel = m.getChannel();

    if (m.isNoteOn())
    {
        noteOn (channel, m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        noteOff (channel, m.getNoteNumber(), m.getFloatVelocity(), true);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        allNotesOff (channel, true);
    }
    else if (m.isPitchWheel())
    {
        const int wheelPos = m.getPitchWheelValue();
        lastPitchWheelValues [channel - 1] = wheelPos;
        handlePitchWheel (channel, wheelPos);
    }
    else if (m.isAftertouch())
    {
        handleAftertouch (channel, m.getNoteNumber(), m.getAfterTouchValue());
    }
    else if (m.isChannelPressure())
    {
        handleChannelPressure (channel, m.getChannelPressureValue());
    }
    else if (m.isController())
    {
        handleController (channel, m.getControllerNumber(), m.getControllerValue());
    }
    else if (m.isProgramChange())
    {
        handleProgramChange (channel, m.getProgramChangeNumber());
    }
}

//==============================================================================
void Synthesiser::noteOn (const int midiChannel,
                          const int midiNoteNumber,
                          const float velocity)
{

    for (int i = sounds.size(); --i >= 0;)
    {
        SynthesiserSound* const sound = sounds.getUnchecked(i);

        if (sound->appliesToNote (midiNoteNumber)
             && sound->appliesToChannel (midiChannel))
        {
            // If hitting a note that's still ringing, stop it first (it could be
            // still playing because of the sustain or sostenuto pedal).
            for (int j = voices.size(); --j >= 0;)
            {
                SynthesiserVoice* const voice = voices.getUnchecked (j);

                if (voice->getCurrentlyPlayingNote() == midiNoteNumber
                     && voice->isPlayingChannel (midiChannel))
                    stopVoice (voice, 1.0f, true);
            }

            startVoice (findFreeVoice (sound, midiChannel, midiNoteNumber, shouldStealNotes),
                        sound, midiChannel, midiNoteNumber, velocity);
        }
    }
}

void Synthesiser::startVoice (SynthesiserVoice* const voice,
                              SynthesiserSound* const sound,
                              const int midiChannel,
                              const int midiNoteNumber,
                              const float velocity)
{
    if (voice != nullptr && sound != nullptr)
    {
        CARLA_SAFE_ASSERT_RETURN(midiChannel > 0 && midiChannel <= 16,);

        if (voice->currentlyPlayingSound != nullptr)
            voice->stopNote (0.0f, false);

        voice->currentlyPlayingNote = midiNoteNumber;
        voice->currentPlayingMidiChannel = midiChannel;
        voice->noteOnTime = ++lastNoteOnCounter;
        voice->currentlyPlayingSound = sound;
        voice->keyIsDown = true;
        voice->sostenutoPedalDown = false;
        voice->sustainPedalDown = sustainPedalsDown[midiChannel];

        voice->startNote (midiNoteNumber, velocity, sound,
                          lastPitchWheelValues [midiChannel - 1]);
    }
}

void Synthesiser::stopVoice (SynthesiserVoice* voice, float velocity, const bool allowTailOff)
{
    wassert (voice != nullptr);

    voice->stopNote (velocity, allowTailOff);

    // the subclass MUST call clearCurrentNote() if it's not tailing off! RTFM for stopNote()!
    wassert (allowTailOff || (voice->getCurrentlyPlayingNote() < 0 && voice->getCurrentlyPlayingSound() == 0));
}

void Synthesiser::noteOff (const int midiChannel,
                           const int midiNoteNumber,
                           const float velocity,
                           const bool allowTailOff)
{
    CARLA_SAFE_ASSERT_RETURN(midiChannel > 0 && midiChannel <= 16,);

    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (voice->getCurrentlyPlayingNote() == midiNoteNumber
              && voice->isPlayingChannel (midiChannel))
        {
            if (SynthesiserSound* const sound = voice->getCurrentlyPlayingSound())
            {
                if (sound->appliesToNote (midiNoteNumber)
                     && sound->appliesToChannel (midiChannel))
                {
                    CARLA_SAFE_ASSERT_RETURN (! voice->keyIsDown || voice->sustainPedalDown == sustainPedalsDown [midiChannel],);

                    voice->keyIsDown = false;

                    if (! (voice->sustainPedalDown || voice->sostenutoPedalDown))
                        stopVoice (voice, velocity, allowTailOff);
                }
            }
        }
    }
}

void Synthesiser::allNotesOff (const int midiChannel, const bool allowTailOff)
{
    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->stopNote (1.0f, allowTailOff);
    }

    zerostruct(sustainPedalsDown);
}

void Synthesiser::handlePitchWheel (const int midiChannel, const int wheelValue)
{
    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->pitchWheelMoved (wheelValue);
    }
}

void Synthesiser::handleController (const int midiChannel,
                                    const int controllerNumber,
                                    const int controllerValue)
{
    switch (controllerNumber)
    {
        case 0x40:  handleSustainPedal   (midiChannel, controllerValue >= 64); break;
        case 0x42:  handleSostenutoPedal (midiChannel, controllerValue >= 64); break;
        case 0x43:  handleSoftPedal      (midiChannel, controllerValue >= 64); break;
        default:    break;
    }


    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->controllerMoved (controllerNumber, controllerValue);
    }
}

void Synthesiser::handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue)
{

    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (voice->getCurrentlyPlayingNote() == midiNoteNumber
              && (midiChannel <= 0 || voice->isPlayingChannel (midiChannel)))
            voice->aftertouchChanged (aftertouchValue);
    }
}

void Synthesiser::handleChannelPressure (int midiChannel, int channelPressureValue)
{

    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->channelPressureChanged (channelPressureValue);
    }
}

void Synthesiser::handleSustainPedal (int midiChannel, bool isDown)
{
    CARLA_SAFE_ASSERT_RETURN(midiChannel > 0 && midiChannel <= 16,);

    if (isDown)
    {
        sustainPedalsDown[midiChannel] = true;

        for (int i = voices.size(); --i >= 0;)
        {
            SynthesiserVoice* const voice = voices.getUnchecked (i);

            if (voice->isPlayingChannel (midiChannel) && voice->isKeyDown())
                voice->sustainPedalDown = true;
        }
    }
    else
    {
        for (int i = voices.size(); --i >= 0;)
        {
            SynthesiserVoice* const voice = voices.getUnchecked (i);

            if (voice->isPlayingChannel (midiChannel))
            {
                voice->sustainPedalDown = false;

                if (! voice->isKeyDown())
                    stopVoice (voice, 1.0f, true);
            }
        }

        sustainPedalsDown[midiChannel] = false;
    }
}

void Synthesiser::handleSostenutoPedal (int midiChannel, bool isDown)
{
    wassert (midiChannel > 0 && midiChannel <= 16);

    for (int i = voices.size(); --i >= 0;)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (voice->isPlayingChannel (midiChannel))
        {
            if (isDown)
                voice->sostenutoPedalDown = true;
            else if (voice->sostenutoPedalDown)
                stopVoice (voice, 1.0f, true);
        }
    }
}

void Synthesiser::handleSoftPedal (int midiChannel, bool /*isDown*/)
{
    ignoreUnused (midiChannel);
    wassert (midiChannel > 0 && midiChannel <= 16);
}

void Synthesiser::handleProgramChange (int midiChannel, int programNumber)
{
    ignoreUnused (midiChannel, programNumber);
    wassert (midiChannel > 0 && midiChannel <= 16);
}

//==============================================================================
SynthesiserVoice* Synthesiser::findFreeVoice (SynthesiserSound* soundToPlay,
                                              int midiChannel, int midiNoteNumber,
                                              const bool stealIfNoneAvailable) const
{

    for (size_t i = 0; i < voices.size(); ++i)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if ((! voice->isVoiceActive()) && voice->canPlaySound (soundToPlay))
            return voice;
    }

    if (stealIfNoneAvailable)
        return findVoiceToSteal (soundToPlay, midiChannel, midiNoteNumber);

    return nullptr;
}

struct VoiceAgeSorter
{
    static int compareElements (SynthesiserVoice* v1, SynthesiserVoice* v2) noexcept
    {
        return v1->wasStartedBefore (*v2) ? -1 : (v2->wasStartedBefore (*v1) ? 1 : 0);
    }
};

SynthesiserVoice* Synthesiser::findVoiceToSteal (SynthesiserSound* soundToPlay,
                                                 int /*midiChannel*/, int midiNoteNumber) const
{
    // This voice-stealing algorithm applies the following heuristics:
    // - Re-use the oldest notes first
    // - Protect the lowest & topmost notes, even if sustained, but not if they've been released.

    // apparently you are trying to render audio without having any voices...
    wassert (voices.size() > 0);

    // These are the voices we want to protect (ie: only steal if unavoidable)
    SynthesiserVoice* low = nullptr; // Lowest sounding note, might be sustained, but NOT in release phase
    SynthesiserVoice* top = nullptr; // Highest sounding note, might be sustained, but NOT in release phase

    // this is a list of voices we can steal, sorted by how long they've been running
    Array<SynthesiserVoice*> usableVoices;
    usableVoices.ensureStorageAllocated (voices.size());

    for (size_t i = 0; i < voices.size(); ++i)
    {
        SynthesiserVoice* const voice = voices.getUnchecked (i);

        if (voice->canPlaySound (soundToPlay))
        {
            wassert (voice->isVoiceActive()); // We wouldn't be here otherwise

            VoiceAgeSorter sorter;
            usableVoices.addSorted (sorter, voice);

            if (! voice->isPlayingButReleased()) // Don't protect released notes
            {
                const int note = voice->getCurrentlyPlayingNote();

                if (low == nullptr || note < low->getCurrentlyPlayingNote())
                    low = voice;

                if (top == nullptr || note > top->getCurrentlyPlayingNote())
                    top = voice;
            }
        }
    }

    // Eliminate pathological cases (ie: only 1 note playing): we always give precedence to the lowest note(s)
    if (top == low)
        top = nullptr;

    const int numUsableVoices = usableVoices.size();

    // The oldest note that's playing with the target pitch is ideal..
    for (int i = 0; i < numUsableVoices; ++i)
    {
        SynthesiserVoice* const voice = usableVoices.getUnchecked (i);

        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            return voice;
    }

    // Oldest voice that has been released (no finger on it and not held by sustain pedal)
    for (int i = 0; i < numUsableVoices; ++i)
    {
        SynthesiserVoice* const voice = usableVoices.getUnchecked (i);

        if (voice != low && voice != top && voice->isPlayingButReleased())
            return voice;
    }

    // Oldest voice that doesn't have a finger on it:
    for (int i = 0; i < numUsableVoices; ++i)
    {
        SynthesiserVoice* const voice = usableVoices.getUnchecked (i);

        if (voice != low && voice != top && ! voice->isKeyDown())
            return voice;
    }

    // Oldest voice that isn't protected
    for (int i = 0; i < numUsableVoices; ++i)
    {
        SynthesiserVoice* const voice = usableVoices.getUnchecked (i);

        if (voice != low && voice != top)
            return voice;
    }

    // We've only got "protected" voices now: lowest note takes priority
    wassert (low != nullptr);

    // Duophonic synth: give priority to the bass note:
    if (top != nullptr)
        return top;

    return low;
}

}
