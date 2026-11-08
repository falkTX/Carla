/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZSAMPLE_H_INCLUDED
#define SFZSAMPLE_H_INCLUDED

#include "SFZCommon.h"

#include "extra/ScopedPointer.hpp"
#include "water/buffers/AudioSampleBuffer.h"
#include "water/files/File.h"

namespace sfzero
{

class Sample
{
public:
  explicit Sample(const water::File &fileIn) : file_(fileIn), buffer_(nullptr), sampleRate_(0), sampleLength_(0), loopStart_(0), loopEnd_(0) {}
  virtual ~Sample();

  bool load();

  water::File getFile() { return (file_); }
  water::AudioSampleBuffer *getBuffer() { return (buffer_); }
  double getSampleRate() { return (sampleRate_); }
  water::String getShortName();
  void setBuffer(water::AudioSampleBuffer *newBuffer);
  water::AudioSampleBuffer *detachBuffer();
  water::String dump();
  water::uint64 getSampleLength() const { return sampleLength_; }
  water::uint64 getLoopStart() const { return loopStart_; }
  water::uint64 getLoopEnd() const { return loopEnd_; }

#ifdef DEBUG
  void checkIfZeroed(const char *where);
#endif

private:
  water::File file_;
  ScopedPointer<water::AudioSampleBuffer> buffer_;
  double sampleRate_;
  water::uint64 sampleLength_, loopStart_, loopEnd_;

  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sample)
};
}

#endif // SFZSAMPLE_H_INCLUDED
