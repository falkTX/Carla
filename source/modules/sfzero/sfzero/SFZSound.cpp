/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/

#include "SFZSound.h"
#include "SFZReader.h"
#include "SFZRegion.h"
#include "SFZSample.h"

namespace sfzero
{

Sound::Sound(const water::File &fileIn) : file_(fileIn) {}
Sound::~Sound()
{
  int numRegions = regions_.size();

  for (int i = 0; i < numRegions; ++i)
  {
    delete regions_[i];
    regions_.set(i, nullptr);
  }

  for (water::HashMap<water::String, Sample *>::Iterator i(samples_); i.next();)
  {
    delete i.getValue();
  }
}

bool Sound::appliesToNote(int /*midiNoteNumber*/)
{
  // Just say yes; we can't truly know unless we're told the velocity as well.
  return true;
}

bool Sound::appliesToChannel(int /*midiChannel*/) { return true; }
void Sound::addRegion(Region *region) { regions_.add(region); }
Sample *Sound::addSample(water::String path, water::String defaultPath)
{
  path = path.replaceCharacter('\\', '/');
  defaultPath = defaultPath.replaceCharacter('\\', '/');
  water::File sampleFile;
  if (defaultPath.isEmpty())
  {
    sampleFile = file_.getSiblingFile(path);
  }
  else
  {
    water::File defaultDir = file_.getSiblingFile(defaultPath);
    sampleFile = defaultDir.getChildFile(path);
  }
  water::String samplePath = sampleFile.getFullPathName();
  Sample *sample = samples_[samplePath];
  if (sample == nullptr)
  {
    sample = new Sample(sampleFile);
    samples_.set(samplePath, sample);
  }
  return sample;
}

void Sound::addError(const water::String &message) { errors_.add(message); }

void Sound::addUnsupportedOpcode(const water::String &opcode)
{
  if (!unsupportedOpcodes_.contains(opcode))
  {
    unsupportedOpcodes_.set(opcode, opcode);
    water::String warning = "unsupported opcode: ";
    warning << opcode;
    warnings_.add(warning);
  }
}

void Sound::loadRegions()
{
  Reader reader(this);

  reader.read(file_);
}

void Sound::loadSamples(const LoadingIdleCallback& cb)
{
    for (water::HashMap<water::String, Sample *>::Iterator i(samples_); i.next();)
    {
        Sample* const sample = i.getValue();

        if (sample->load())
        {
            carla_debug("Loaded sample '%s'", sample->getShortName().toRawUTF8());
            cb.callback(cb.callbackPtr);
        }
        else
        {
            addError("Couldn't load sample \"" + sample->getShortName() + "\"");
        }
    }
}

Region *Sound::getRegionFor(int note, int velocity, Region::Trigger trigger)
{
  int numRegions = regions_.size();

  for (int i = 0; i < numRegions; ++i)
  {
    Region *region = regions_[i];
    if (region->matches(note, velocity, trigger))
    {
      return region;
    }
  }

  return nullptr;
}

int Sound::getNumRegions() { return regions_.size(); }

Region *Sound::regionAt(int index) { return regions_[index]; }

water::String Sound::dump()
{
  water::String info;
  auto &errors = getErrors();
  if (errors.size() > 0)
  {
    info << errors.size() << " errors: \n";
    info << errors.joinIntoString("\n");
    info << "\n";
  }
  else
  {
    info << "no errors.\n\n";
  }

  auto &warnings = getWarnings();
  if (warnings.size() > 0)
  {
    info << warnings.size() << " warnings: \n";
    info << warnings.joinIntoString("\n");
  }
  else
  {
    info << "no warnings.\n";
  }

  if (regions_.size() > 0)
  {
    info << regions_.size() << " regions: \n";
    for (int i = 0; i < regions_.size(); ++i)
    {
      info << regions_[i]->dump();
    }
  }
  else
  {
    info << "no regions.\n";
  }

  if (samples_.size() > 0)
  {
    info << samples_.size() << " samples: \n";
    for (water::HashMap<water::String, Sample *>::Iterator i(samples_); i.next();)
    {
      info << i.getValue()->dump();
    }
  }
  else
  {
    info << "no samples.\n";
  }
  return info;
}

}
