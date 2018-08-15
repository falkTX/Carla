/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZSOUND_H_INCLUDED
#define SFZSOUND_H_INCLUDED

#include "SFZRegion.h"

#include "water/containers/HashMap.h"
#include "water/synthesisers/Synthesiser.h"
#include "water/text/StringArray.h"

#include "CarlaThread.hpp"

namespace sfzero
{

class Sample;

class Sound : public water::SynthesiserSound
{
public:
  explicit Sound(const water::File &file);
  virtual ~Sound();

  typedef water::ReferenceCountedObjectPtr<Sound> Ptr;

  bool appliesToNote(int midiNoteNumber) override;
  bool appliesToChannel(int midiChannel) override;

  void addRegion(Region *region); // Takes ownership of the region.
  Sample *addSample(water::String path, water::String defaultPath = water::String());
  void addError(const water::String &message);
  void addUnsupportedOpcode(const water::String &opcode);

  virtual void loadRegions();
  virtual void loadSamples(water::AudioFormatManager* formatManager, double* progressVar = nullptr,
                           CarlaThread* thread = nullptr);

  Region *getRegionFor(int note, int velocity, Region::Trigger trigger = Region::attack);
  int getNumRegions();
  Region *regionAt(int index);

  const water::StringArray &getErrors() { return errors_; }
  const water::StringArray &getWarnings() { return warnings_; }

  water::String dump();
  water::Array<Region *> &getRegions() { return regions_; }
  water::File &getFile() { return file_; }

private:
  water::File file_;
  water::Array<Region *> regions_;
  water::HashMap<water::String, Sample *> samples_;
  water::StringArray errors_;
  water::StringArray warnings_;
  water::HashMap<water::String, water::String> unsupportedOpcodes_;

  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sound)
};
}

#endif // SFZSOUND_H_INCLUDED
