/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZVOICE_H_INCLUDED
#define SFZVOICE_H_INCLUDED

#include "SFZEG.h"

#include "CarlaJuceUtils.hpp"

#if 1
namespace water {
class SynthesiserVoice {
public:
  virtual bool canPlaySound(water::SynthesiserSound *sound) = 0;
  virtual void startNote(int midiNoteNumber, float velocity, water::SynthesiserSound *sound, int currentPitchWheelPosition) = 0;
  virtual void stopNote(float velocity, bool allowTailOff) = 0;
  virtual void pitchWheelMoved(int newValue) = 0;
  virtual void controllerMoved(int controllerNumber, int newValue) = 0;
  virtual void renderNextBlock(water::AudioSampleBuffer &outputBuffer, int startSample, int numSamples) = 0;
};
}
#endif

namespace sfzero
{
struct Region;

class Voice : public water::SynthesiserVoice
{
public:
  Voice();
  virtual ~Voice();

  bool canPlaySound(water::SynthesiserSound *sound) override;
  void startNote(int midiNoteNumber, float velocity, water::SynthesiserSound *sound, int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;
  void stopNoteForGroup();
  void stopNoteQuick();
  void pitchWheelMoved(int newValue) override;
  void controllerMoved(int controllerNumber, int newValue) override;
  void renderNextBlock(water::AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override;
  bool isPlayingNoteDown();
  bool isPlayingOneShot();

  int getGroup();
  water::uint64 getOffBy();

  // Set the region to be used by the next startNote().
  void setRegion(Region *nextRegion);

  water::String infoString();

private:
  Region *region_;
  int trigger_;
  int curMidiNote_, curPitchWheel_;
  double pitchRatio_;
  float noteGainLeft_, noteGainRight_;
  double sourceSamplePosition_;
  EG ampeg_;
  water::int64 sampleEnd_;
  water::int64 loopStart_, loopEnd_;

  // Info only.
  int numLoops_;
  int curVelocity_;

  void calcPitchRatio();
  void killNote();
  double fractionalMidiNoteInHz(double note, double freqOfA = 440.0);

  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)
};
}

#endif // SFZVOICE_H_INCLUDED
