/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZSYNTH_H_INCLUDED
#define SFZSYNTH_H_INCLUDED

#include "SFZCommon.h"

#include "CarlaJuceUtils.hpp"

#if 1
namespace water {
class Synthesiser {
public:
  virtual void noteOn(int midiChannel, int midiNoteNumber, float velocity) = 0;
  virtual void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) = 0;
};
}
#endif

namespace sfzero
{

class Synth : public water::Synthesiser
{
public:
  Synth();
  virtual ~Synth() {}

  void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;
  void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;

  int numVoicesUsed();
  water::String voiceInfoString();

private:
  int noteVelocities_[128];
  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Synth)
};
}

#endif // SFZSYNTH_H_INCLUDED
