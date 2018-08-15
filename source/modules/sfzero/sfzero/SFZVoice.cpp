/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#include "SFZDebug.h"
#include "SFZRegion.h"
#include "SFZSample.h"
#include "SFZSound.h"
#include "SFZVoice.h"
#include <math.h>

static const float globalGain = -1.0;

sfzero::Voice::Voice()
    : region_(nullptr), trigger_(0), curMidiNote_(0), curPitchWheel_(0), pitchRatio_(0), noteGainLeft_(0), noteGainRight_(0),
      sourceSamplePosition_(0), sampleEnd_(0), loopStart_(0), loopEnd_(0), numLoops_(0), curVelocity_(0)
{
  ampeg_.setExponentialDecay(true);
}

sfzero::Voice::~Voice() {}

bool sfzero::Voice::canPlaySound(water::SynthesiserSound *sound) { return dynamic_cast<sfzero::Sound *>(sound) != nullptr; }

void sfzero::Voice::startNote(int midiNoteNumber, float floatVelocity, water::SynthesiserSound *soundIn,
                              int currentPitchWheelPosition)
{
  sfzero::Sound *sound = dynamic_cast<sfzero::Sound *>(soundIn);

  if (sound == nullptr)
  {
    killNote();
    return;
  }

  int velocity = static_cast<int>(floatVelocity * 127.0);
  curVelocity_ = velocity;
  if (region_ == nullptr)
  {
    region_ = sound->getRegionFor(midiNoteNumber, velocity);
  }
  if ((region_ == nullptr) || (region_->sample == nullptr) || (region_->sample->getBuffer() == nullptr))
  {
    killNote();
    return;
  }
  if (region_->negative_end)
  {
    killNote();
    return;
  }

  // Pitch.
  curMidiNote_ = midiNoteNumber;
  curPitchWheel_ = currentPitchWheelPosition;
  calcPitchRatio();

  // Gain.
  double noteGainDB = globalGain + region_->volume;
  // Thanks to <http:://www.drealm.info/sfz/plj-sfz.xhtml> for explaining the
  // velocity curve in a way that I could understand, although they mean
  // "log10" when they say "log".
  double velocityGainDB = -20.0 * log10((127.0 * 127.0) / (velocity * velocity));
  velocityGainDB *= region_->amp_veltrack / 100.0;
  noteGainDB += velocityGainDB;
  noteGainLeft_ = noteGainRight_ = static_cast<float>(water::Decibels::decibelsToGain(noteGainDB));
  // The SFZ spec is silent about the pan curve, but a 3dB pan law seems
  // common.  This sqrt() curve matches what Dimension LE does; Alchemy Free
  // seems closer to sin(adjustedPan * pi/2).
  double adjustedPan = (region_->pan + 100.0) / 200.0;
  noteGainLeft_ *= static_cast<float>(sqrt(1.0 - adjustedPan));
  noteGainRight_ *= static_cast<float>(sqrt(adjustedPan));
  ampeg_.startNote(&region_->ampeg, floatVelocity, getSampleRate(), &region_->ampeg_veltrack);

  // Offset/end.
  sourceSamplePosition_ = static_cast<double>(region_->offset);
  sampleEnd_ = region_->sample->getSampleLength();
  if ((region_->end > 0) && (region_->end < sampleEnd_))
  {
    sampleEnd_ = region_->end + 1;
  }

  // Loop.
  loopStart_ = loopEnd_ = 0;
  sfzero::Region::LoopMode loopMode = region_->loop_mode;
  if (loopMode == sfzero::Region::sample_loop)
  {
    if (region_->sample->getLoopStart() < region_->sample->getLoopEnd())
    {
      loopMode = sfzero::Region::loop_continuous;
    }
    else
    {
      loopMode = sfzero::Region::no_loop;
    }
  }
  if ((loopMode != sfzero::Region::no_loop) && (loopMode != sfzero::Region::one_shot))
  {
    if (region_->loop_start < region_->loop_end)
    {
      loopStart_ = region_->loop_start;
      loopEnd_ = region_->loop_end;
    }
    else
    {
      loopStart_ = region_->sample->getLoopStart();
      loopEnd_ = region_->sample->getLoopEnd();
    }
  }
  numLoops_ = 0;
}

void sfzero::Voice::stopNote(float /*velocity*/, bool allowTailOff)
{
  if (!allowTailOff || (region_ == nullptr))
  {
    killNote();
    return;
  }

  if (region_->loop_mode != sfzero::Region::one_shot)
  {
    ampeg_.noteOff();
  }
  if (region_->loop_mode == sfzero::Region::loop_sustain)
  {
    // Continue playing, but stop looping.
    loopEnd_ = loopStart_;
  }
}

void sfzero::Voice::stopNoteForGroup()
{
  if (region_->off_mode == sfzero::Region::fast)
  {
    ampeg_.fastRelease();
  }
  else
  {
    ampeg_.noteOff();
  }
}

void sfzero::Voice::stopNoteQuick() { ampeg_.fastRelease(); }
void sfzero::Voice::pitchWheelMoved(int newValue)
{
  if (region_ == nullptr)
  {
    return;
  }

  curPitchWheel_ = newValue;
  calcPitchRatio();
}

void sfzero::Voice::controllerMoved(int /*controllerNumber*/, int /*newValue*/) { /***/}
void sfzero::Voice::renderNextBlock(water::AudioSampleBuffer &outputBuffer, int startSample, int numSamples)
{
  if (region_ == nullptr)
  {
    return;
  }

  water::AudioSampleBuffer *buffer = region_->sample->getBuffer();
  const float *inL = buffer->getReadPointer(0, 0);
  const float *inR = buffer->getNumChannels() > 1 ? buffer->getReadPointer(1, 0) : nullptr;

  float *outL = outputBuffer.getWritePointer(0, startSample);
  float *outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

  int bufferNumSamples = buffer->getNumSamples(); // leoo

  // Cache some values, to give them at least some chance of ending up in
  // registers.
  double sourceSamplePosition = this->sourceSamplePosition_;
  float ampegGain = ampeg_.getLevel();
  float ampegSlope = ampeg_.getSlope();
  int samplesUntilNextAmpSegment = ampeg_.getSamplesUntilNextSegment();
  bool ampSegmentIsExponential = ampeg_.getSegmentIsExponential();
  float loopStart = static_cast<float>(this->loopStart_);
  float loopEnd = static_cast<float>(this->loopEnd_);
  float sampleEnd = static_cast<float>(this->sampleEnd_);

  while (--numSamples >= 0)
  {
    int pos = static_cast<int>(sourceSamplePosition);
    jassert(pos >= 0 && pos < bufferNumSamples); // leoo
    float alpha = static_cast<float>(sourceSamplePosition - pos);
    float invAlpha = 1.0f - alpha;
    int nextPos = pos + 1;
    if ((loopStart < loopEnd) && (nextPos > loopEnd))
    {
      nextPos = static_cast<int>(loopStart);
    }

    // Simple linear interpolation with buffer overrun check
    float nextL = nextPos < bufferNumSamples ? inL[nextPos] : inL[pos];
    float nextR = inR ? (nextPos < bufferNumSamples ? inR[nextPos] : inR[pos]) : nextL;
    float l = (inL[pos] * invAlpha + nextL * alpha);
    float r = inR ? (inR[pos] * invAlpha + nextR * alpha) : l;

    //// Simple linear interpolation, old version (possible buffer overrun with non-loop??)
    // float l = (inL[pos] * invAlpha + inL[nextPos] * alpha);
    // float r = inR ? (inR[pos] * invAlpha + inR[nextPos] * alpha) : l;

    float gainLeft = noteGainLeft_ * ampegGain;
    float gainRight = noteGainRight_ * ampegGain;
    l *= gainLeft;
    r *= gainRight;
    // Shouldn't we dither here?

    if (outR)
    {
      *outL++ += l;
      *outR++ += r;
    }
    else
    {
      *outL++ += (l + r) * 0.5f;
    }

    // Next sample.
    sourceSamplePosition += pitchRatio_;
    if ((loopStart < loopEnd) && (sourceSamplePosition > loopEnd))
    {
      sourceSamplePosition = loopStart;
      numLoops_ += 1;
    }

    // Update EG.
    if (ampSegmentIsExponential)
    {
      ampegGain *= ampegSlope;
    }
    else
    {
      ampegGain += ampegSlope;
    }
    if (--samplesUntilNextAmpSegment < 0)
    {
      ampeg_.setLevel(ampegGain);
      ampeg_.nextSegment();
      ampegGain = ampeg_.getLevel();
      ampegSlope = ampeg_.getSlope();
      samplesUntilNextAmpSegment = ampeg_.getSamplesUntilNextSegment();
      ampSegmentIsExponential = ampeg_.getSegmentIsExponential();
    }

    if ((sourceSamplePosition >= sampleEnd) || ampeg_.isDone())
    {
      killNote();
      break;
    }
  }

  this->sourceSamplePosition_ = sourceSamplePosition;
  ampeg_.setLevel(ampegGain);
  ampeg_.setSamplesUntilNextSegment(samplesUntilNextAmpSegment);
}

bool sfzero::Voice::isPlayingNoteDown() { return region_ && region_->trigger != sfzero::Region::release; }

bool sfzero::Voice::isPlayingOneShot() { return region_ && region_->loop_mode == sfzero::Region::one_shot; }

int sfzero::Voice::getGroup() { return region_ ? region_->group : 0; }

water::uint64 sfzero::Voice::getOffBy() { return region_ ? region_->off_by : 0; }

void sfzero::Voice::setRegion(sfzero::Region *nextRegion) { region_ = nextRegion; }

water::String sfzero::Voice::infoString()
{
  const char *egSegmentNames[] = {"delay", "attack", "hold", "decay", "sustain", "release", "done"};

  const static int numEGSegments(sizeof(egSegmentNames) / sizeof(egSegmentNames[0]));

  const char *egSegmentName = "-Invalid-";
  int egSegmentIndex = ampeg_.segmentIndex();
  if ((egSegmentIndex >= 0) && (egSegmentIndex < numEGSegments))
  {
    egSegmentName = egSegmentNames[egSegmentIndex];
  }

  water::String info;
  info << "note: " << curMidiNote_ << ", vel: " << curVelocity_ << ", pan: " << region_->pan << ", eg: " << egSegmentName
       << ", loops: " << numLoops_;
  return info;
}

void sfzero::Voice::calcPitchRatio()
{
  double note = curMidiNote_;

  note += region_->transpose;
  note += region_->tune / 100.0;

  double adjustedPitch = region_->pitch_keycenter + (note - region_->pitch_keycenter) * (region_->pitch_keytrack / 100.0);
  if (curPitchWheel_ != 8192)
  {
    double wheel = ((2.0 * curPitchWheel_ / 16383.0) - 1.0);
    if (wheel > 0)
    {
      adjustedPitch += wheel * region_->bend_up / 100.0;
    }
    else
    {
      adjustedPitch += wheel * region_->bend_down / -100.0;
    }
  }
  double targetFreq = fractionalMidiNoteInHz(adjustedPitch);
  double naturalFreq = water::MidiMessage::getMidiNoteInHertz(region_->pitch_keycenter);
  pitchRatio_ = (targetFreq * region_->sample->getSampleRate()) / (naturalFreq * getSampleRate());
}

void sfzero::Voice::killNote()
{
  region_ = nullptr;
  clearCurrentNote();
}

double sfzero::Voice::fractionalMidiNoteInHz(double note, double freqOfA)
{
  // Like MidiMessage::getMidiNoteInHertz(), but with a float note.
  note -= 69;
  // Now 0 = A
  return freqOfA * pow(2.0, note / 12.0);
}
