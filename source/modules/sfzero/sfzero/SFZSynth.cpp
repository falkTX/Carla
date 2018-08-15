/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#include "SFZSynth.h"
#include "SFZSound.h"
#include "SFZVoice.h"

sfzero::Synth::Synth() : Synthesiser() {}

#if 0
void sfzero::Synth::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
  int i;

  const water::ScopedLock locker(lock);

  int midiVelocity = static_cast<int>(velocity * 127);

  // First, stop any currently-playing sounds in the group.
  //*** Currently, this only pays attention to the first matching region.
  int group = 0;
  sfzero::Sound *sound = dynamic_cast<sfzero::Sound *>(getSound(0));

  if (sound)
  {
    sfzero::Region *region = sound->getRegionFor(midiNoteNumber, midiVelocity);
    if (region)
    {
      group = region->group;
    }
  }
  if (group != 0)
  {
    for (i = voices.size(); --i >= 0;)
    {
      sfzero::Voice *voice = dynamic_cast<sfzero::Voice *>(voices.getUnchecked(i));
      if (voice == nullptr)
      {
        continue;
      }
      if (voice->getOffBy() == group)
      {
        voice->stopNoteForGroup();
      }
    }
  }

  // Are any notes playing?  (Needed for first/legato trigger handling.)
  // Also stop any voices still playing this note.
  bool anyNotesPlaying = false;
  for (i = voices.size(); --i >= 0;)
  {
    sfzero::Voice *voice = dynamic_cast<sfzero::Voice *>(voices.getUnchecked(i));
    if (voice == nullptr)
    {
      continue;
    }
    if (voice->isPlayingChannel(midiChannel))
    {
      if (voice->isPlayingNoteDown())
      {
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
        {
          if (!voice->isPlayingOneShot())
          {
            voice->stopNoteQuick();
          }
        }
        else
        {
          anyNotesPlaying = true;
        }
      }
    }
  }

  // Play *all* matching regions.
  sfzero::Region::Trigger trigger = (anyNotesPlaying ? sfzero::Region::legato : sfzero::Region::first);
  if (sound)
  {
    int numRegions = sound->getNumRegions();
    for (i = 0; i < numRegions; ++i)
    {
      sfzero::Region *region = sound->regionAt(i);
      if (region->matches(midiNoteNumber, midiVelocity, trigger))
      {
        sfzero::Voice *voice =
            dynamic_cast<sfzero::Voice *>(findFreeVoice(sound, midiNoteNumber, midiChannel, isNoteStealingEnabled()));
        if (voice)
        {
          voice->setRegion(region);
          startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
        }
      }
    }
  }

  noteVelocities_[midiNoteNumber] = midiVelocity;
}
#endif

#if 0
void sfzero::Synth::noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
  const water::ScopedLock locker(lock);

  Synthesiser::noteOff(midiChannel, midiNoteNumber, velocity, allowTailOff);

  // Start release region.
  sfzero::Sound *sound = dynamic_cast<sfzero::Sound *>(getSound(0));
  if (sound)
  {
    sfzero::Region *region = sound->getRegionFor(midiNoteNumber, noteVelocities_[midiNoteNumber], sfzero::Region::release);
    if (region)
    {
      sfzero::Voice *voice = dynamic_cast<sfzero::Voice *>(findFreeVoice(sound, midiNoteNumber, midiChannel, false));
      if (voice)
      {
        // Synthesiser is too locked-down (ivars are private rt protected), so
        // we have to use a "setRegion()" mechanism.
        voice->setRegion(region);
        startVoice(voice, sound, midiChannel, midiNoteNumber, noteVelocities_[midiNoteNumber] / 127.0f);
      }
    }
  }
}
#endif

int sfzero::Synth::numVoicesUsed()
{
  int numUsed = 0;

#if 0
  for (int i = voices.size(); --i >= 0;)
  {
    if (voices.getUnchecked(i)->getCurrentlyPlayingNote() >= 0)
    {
      numUsed += 1;
    }
  }
#endif
  return numUsed;
}

water::String sfzero::Synth::voiceInfoString()
{
  enum
  {
    maxShownVoices = 20,
  };

  water::StringArray lines;
  int numUsed = 0, numShown = 0;
#if 0
  for (int i = voices.size(); --i >= 0;)
  {
    sfzero::Voice *voice = dynamic_cast<sfzero::Voice *>(voices.getUnchecked(i));
    if (voice->getCurrentlyPlayingNote() < 0)
    {
      continue;
    }
    numUsed += 1;
    if (numShown >= maxShownVoices)
    {
      continue;
    }
    lines.add(voice->infoString());
  }
#endif
  lines.insert(0, "voices used: " + water::String(numUsed));
  return lines.joinIntoString("\n");
}
