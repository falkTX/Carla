/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZREADER_H_INCLUDED
#define SFZREADER_H_INCLUDED

#include "SFZCommon.h"

#include "CarlaJuceUtils.hpp"

namespace sfzero
{

struct Region;
class Sound;

class Reader
{
public:
  explicit Reader(Sound *sound);
  ~Reader();

  void read(const water::File &file);
  void read(const char *text, unsigned int length);

private:
  const char *handleLineEnd(const char *p);
  const char *readPathInto(water::String *pathOut, const char *p, const char *end);
  int keyValue(const water::String &str);
  int triggerValue(const water::String &str);
  int loopModeValue(const water::String &str);
  void finishRegion(Region *region);
  void error(const water::String &message);

  Sound *sound_;
  int line_;

  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Reader)
};

class StringSlice
{
public:
  StringSlice(const char *startIn, const char *endIn) : start_(startIn), end_(endIn) {}
  virtual ~StringSlice() {}

  unsigned int length() { return static_cast<int>(end_ - start_); }
  bool operator==(const char *other) { return (strncmp(start_, other, length()) == 0); }
  bool operator!=(const char *other) { return (strncmp(start_, other, length()) != 0); }
  const char *getStart() const { return start_; }
  const char *getEnd() const { return end_; }
private:
  const char *start_;
  const char *end_;
  CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSlice)
};
}

#endif // SFZREADER_H_INCLUDED
