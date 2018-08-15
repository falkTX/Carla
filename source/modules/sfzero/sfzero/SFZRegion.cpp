/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/

#include "SFZRegion.h"
#include "SFZSample.h"

namespace sfzero
{

void EGParameters::clear()
{
  delay = 0.0;
  start = 0.0;
  attack = 0.0;
  hold = 0.0;
  decay = 0.0;
  sustain = 100.0;
  release = 0.0;
}

void EGParameters::clearMod()
{
  // Clear for velocity or other modification.
  delay = start = attack = hold = decay = sustain = release = 0.0;
}

Region::Region() { clear(); }

void Region::clear()
{
  memset(this, 0, sizeof(*this));
  hikey = 127;
  hivel = 127;
  pitch_keycenter = 60; // C4
  pitch_keytrack = 100;
  bend_up = 200;
  bend_down = -200;
  volume = pan = 0.0;
  amp_veltrack = 100.0;
  ampeg.clear();
  ampeg_veltrack.clearMod();
}

void Region::clearForSF2()
{
  clear();
  pitch_keycenter = -1;
  loop_mode = no_loop;

  // SF2 defaults in timecents.
  ampeg.delay = -12000.0;
  ampeg.attack = -12000.0;
  ampeg.hold = -12000.0;
  ampeg.decay = -12000.0;
  ampeg.sustain = 0.0;
  ampeg.release = -12000.0;
}

void Region::clearForRelativeSF2()
{
  clear();
  pitch_keytrack = 0;
  amp_veltrack = 0.0;
  ampeg.sustain = 0.0;
}

void Region::addForSF2(Region *other)
{
  offset += other->offset;
  end += other->end;
  loop_start += other->loop_start;
  loop_end += other->loop_end;
  transpose += other->transpose;
  tune += other->tune;
  pitch_keytrack += other->pitch_keytrack;
  volume += other->volume;
  pan += other->pan;

  ampeg.delay += other->ampeg.delay;
  ampeg.attack += other->ampeg.attack;
  ampeg.hold += other->ampeg.hold;
  ampeg.decay += other->ampeg.decay;
  ampeg.sustain += other->ampeg.sustain;
  ampeg.release += other->ampeg.release;
}

void Region::sf2ToSFZ()
{
  // EG times need to be converted from timecents to seconds.
  ampeg.delay = timecents2Secs(static_cast<int>(ampeg.delay));
  ampeg.attack = timecents2Secs(static_cast<int>(ampeg.attack));
  ampeg.hold = timecents2Secs(static_cast<int>(ampeg.hold));
  ampeg.decay = timecents2Secs(static_cast<int>(ampeg.decay));
  if (ampeg.sustain < 0.0f)
  {
    ampeg.sustain = 0.0f;
  }
  ampeg.sustain = 100.0f * decibelsToGain(-ampeg.sustain / 10.0f);
  ampeg.release = timecents2Secs(static_cast<int>(ampeg.release));

  // Pin very short EG segments.  Timecents don't get to zero, and our EG is
  // happier with zero values.
  if (ampeg.delay < 0.01f)
  {
    ampeg.delay = 0.0f;
  }
  if (ampeg.attack < 0.01f)
  {
    ampeg.attack = 0.0f;
  }
  if (ampeg.hold < 0.01f)
  {
    ampeg.hold = 0.0f;
  }
  if (ampeg.decay < 0.01f)
  {
    ampeg.decay = 0.0f;
  }
  if (ampeg.release < 0.01f)
  {
    ampeg.release = 0.0f;
  }

  // Pin values to their ranges.
  if (pan < -100.0f)
  {
    pan = -100.0f;
  }
  else if (pan > 100.0f)
  {
    pan = 100.0f;
  }
}

water::String Region::dump()
{
  water::String info = water::String::formatted("%d - %d, vel %d - %d", lokey, hikey, lovel, hivel);
  if (sample)
  {
    info << sample->getShortName();
  }
  info << "\n";
  return info;
}

float Region::timecents2Secs(int timecents) { return static_cast<float>(pow(2.0, timecents / 1200.0)); }

}
