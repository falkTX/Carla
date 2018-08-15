/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZEG_H_INCLUDED
#define SFZEG_H_INCLUDED

#include "SFZRegion.h"

#include "CarlaJuceUtils.hpp"

namespace sfzero
{
class EG
{
public:
  EG();
  virtual ~EG() {}

  void setExponentialDecay(bool newExponentialDecay);
  void startNote(const EGParameters *parameters, float floatVelocity, double sampleRate, const EGParameters *velMod = nullptr);
  void nextSegment();
  void noteOff();
  void fastRelease();
  bool isDone() { return (segment_ == Done); }
  bool isReleasing() { return (segment_ == Release); }
  int segmentIndex() { return static_cast<int>(segment_); }
  float getLevel() const { return level_; }
  void setLevel(float v) { level_ = v; }
  float getSlope() const { return slope_; }
  void setSlope(float v) { slope_ = v; }
  int getSamplesUntilNextSegment() const { return samplesUntilNextSegment_; }
  void setSamplesUntilNextSegment(int v) { samplesUntilNextSegment_ = v; }
  bool getSegmentIsExponential() const { return segmentIsExponential_; }
  void setSegmentIsExponential(bool v) { segmentIsExponential_ = v; }

private:
  enum Segment
  {
    Delay,
    Attack,
    Hold,
    Decay,
    Sustain,
    Release,
    Done
  };

  void startDelay();
  void startAttack();
  void startHold();
  void startDecay();
  void startSustain();
  void startRelease();

  Segment segment_;
  EGParameters parameters_;
  double sampleRate_;
  bool exponentialDecay_;
  float level_;
  float slope_;
  int samplesUntilNextSegment_;
  bool segmentIsExponential_;
  static const float BottomLevel;
  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EG)
};
}

#endif // SFZEG_H_INCLUDED
