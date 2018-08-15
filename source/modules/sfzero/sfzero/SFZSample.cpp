/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/

#include "SFZSample.h"
#include "SFZDebug.h"

namespace sfzero
{

#if 0
bool Sample::load(water::AudioFormatManager *formatManager)
{
  water::AudioFormatReader *reader = formatManager->createReaderFor(file_);

  if (reader == nullptr)
  {
    return false;
  }
  sampleRate_ = reader->sampleRate;
  sampleLength_ = reader->lengthInSamples;
  // Read some extra samples, which will be filled with zeros, so interpolation
  // can be done without having to check for the edge all the time.
  jassert(sampleLength_ < std::numeric_limits<int>::max());

  buffer_ = new water::AudioSampleBuffer(reader->numChannels, static_cast<int>(sampleLength_ + 4));
  reader->read(buffer_, 0, static_cast<int>(sampleLength_ + 4), 0, true, true);

  water::StringPairArray *metadata = &reader->metadataValues;
  int numLoops = metadata->getValue("NumSampleLoops", "0").getIntValue();
  if (numLoops > 0)
  {
    loopStart_ = metadata->getValue("Loop0Start", "0").getLargeIntValue();
    loopEnd_ = metadata->getValue("Loop0End", "0").getLargeIntValue();
  }
  delete reader;
  return true;
}
#endif

Sample::~Sample() { delete buffer_; }

water::String Sample::getShortName() { return (file_.getFileName()); }

void Sample::setBuffer(water::AudioSampleBuffer *newBuffer)
{
  buffer_ = newBuffer;
  sampleLength_ = buffer_->getNumSamples();
}

water::AudioSampleBuffer *Sample::detachBuffer()
{
  water::AudioSampleBuffer *result = buffer_;
  buffer_ = nullptr;
  return result;
}

water::String Sample::dump() { return file_.getFullPathName() + "\n"; }

#ifdef DEBUG
void Sample::checkIfZeroed(const char *where)
{
  if (buffer_ == nullptr)
  {
    dbgprintf("SFZSample::checkIfZeroed(%s): no buffer!", where);
    return;
  }

  int samplesLeft = buffer_->getNumSamples();
  water::int64 nonzero = 0, zero = 0;
  const float *p = buffer_->getReadPointer(0);
  for (; samplesLeft > 0; --samplesLeft)
  {
    if (*p++ == 0.0)
    {
      zero += 1;
    }
    else
    {
      nonzero += 1;
    }
  }
  if (nonzero > 0)
  {
    dbgprintf("Buffer not zeroed at %s (%lu vs. %lu).", where, nonzero, zero);
  }
  else
  {
    dbgprintf("Buffer zeroed at %s!  (%lu zeros)", where, zero);
  }
}
#endif // JUCE_DEBUG

}
