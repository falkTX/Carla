/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/

#include "SFZSample.h"
#include "SFZDebug.h"

#if 0
#include "water/audioformat/AudioFormatManager.h"
#include "water/audioformat/AudioFormatReader.h"
#include "water/text/StringPairArray.h"
#else
extern "C" {
#include "audio_decoder/ad.h"
}
#endif

namespace sfzero
{

bool Sample::load()
{
#if 0
    static water::AudioFormatManager afm;
    static bool needsInit = true;
    if (needsInit)
    {
        needsInit = false;
        afm.registerBasicFormats();
    }

    water::AudioFormatReader* reader = afm.createReaderFor(file_);
    CARLA_SAFE_ASSERT_RETURN(reader != nullptr, false);

    sampleRate_ = reader->sampleRate;
    sampleLength_ = (water::uint64) reader->lengthInSamples;

    // Read some extra samples, which will be filled with zeros, so interpolation
    // can be done without having to check for the edge all the time.
    CARLA_SAFE_ASSERT_RETURN(sampleLength_ < std::numeric_limits<int>::max(), false);

    water::AudioSampleBuffer* buf = new water::AudioSampleBuffer((int) reader->numChannels, static_cast<int>(sampleLength_ + 4), true);
    reader->read(buf, 0, static_cast<int>(sampleLength_ + 4), 0, true, true);
    buffer_ = buf;

    water::StringPairArray *metadata = &reader->metadataValues;
    int numLoops = metadata->getValue("NumSampleLoops", "0").getIntValue();
    if (numLoops > 0)
    {
        loopStart_ = (water::uint64) metadata->getValue("Loop0Start", "0").getLargeIntValue();
        loopEnd_ = (water::uint64) metadata->getValue("Loop0End", "0").getLargeIntValue();
    }
    delete reader;
#else
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
#endif

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
#endif // DEBUG

}
