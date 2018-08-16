/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/

#include "SFZSample.h"
#include "SFZDebug.h"

extern "C" {
#include "audio_decoder/ad.h"
}

namespace sfzero
{

bool Sample::load()
{
    const water::String filename(file_.getFullPathName());

    struct adinfo info;
    carla_zeroStruct(info);

    void* const handle = ad_open(filename.toRawUTF8(), &info);
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    if (info.frames >= std::numeric_limits<int>::max())
    {
        carla_stderr2("sfzero::Sample::load() - file is too big!");
        ad_close(handle);
        return false;
    }

    sampleRate_ = info.sample_rate;
    sampleLength_ = info.frames/info.channels;
    // TODO loopStart_, loopEnd_

    // read interleaved buffer
    float* const rbuffer = (float*)std::calloc(1, sizeof(float)*info.frames);

    if (rbuffer == nullptr)
    {
        carla_stderr2("sfzero::Sample::load() - out of memory");
        ad_close(handle);
        return false;
    }

    const ssize_t r = ad_read(handle, rbuffer, info.frames);
    if (r+1 < info.frames)
    {
        carla_stderr2("sfzero::Sample::load() - failed to read complete file: " P_SSIZE " vs " P_INT64, r, info.frames);
        ad_close(handle);
        return false;
    }

    // NOTE: We add some extra samples, which will be filled with zeros,
    // so interpolation can be done without having to check for the edge all the time.

    buffer_ = new water::AudioSampleBuffer(info.channels, sampleLength_ + 4, true);

    for (int i=info.channels; --i >= 0;)
        buffer_->copyFromInterleavedSource(i, rbuffer, r);

    std::free(rbuffer);
    ad_close(handle);

    return true;
}

Sample::~Sample() { }

water::String Sample::getShortName() { return (file_.getFileName()); }

void Sample::setBuffer(water::AudioSampleBuffer *newBuffer)
{
  buffer_ = newBuffer;
  sampleLength_ = buffer_->getNumSamples();
}

water::AudioSampleBuffer *Sample::detachBuffer()
{
  return buffer_.release();
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
