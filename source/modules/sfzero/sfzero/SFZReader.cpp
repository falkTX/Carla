/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#include "SFZReader.h"
#include "SFZRegion.h"
#include "SFZSound.h"

#include "water/memory/MemoryBlock.h"

namespace sfzero
{

Reader::Reader(Sound *soundIn) : sound_(soundIn), line_(1) {}

Reader::~Reader() {}

void Reader::read(const water::File &file)
{
  water::MemoryBlock contents;
  bool ok = file.loadFileAsData(contents);

  if (!ok)
  {
    sound_->addError("Couldn't read \"" + file.getFullPathName() + "\"");
    return;
  }

  read(static_cast<const char *>(contents.getData()), static_cast<int>(contents.getSize()));
}

void Reader::read(const char *text, unsigned int length)
{
  const char *p = text;
  const char *end = text + length;
  char c = 0;

  Region curGroup;
  Region curRegion;
  Region *buildingRegion = nullptr;
  bool inControl = false;
  water::String defaultPath;

  while (p < end)
  {
    // We're at the start of a line; skip any whitespace.
    while (p < end)
    {
      c = *p;
      if ((c != ' ') && (c != '\t'))
      {
        break;
      }
      p += 1;
    }
    if (p >= end)
    {
      break;
    }

    // Check if it's a comment line.
    if (c == '/')
    {
      // Skip to end of line.
      while (p < end)
      {
        c = *++p;
        if ((c == '\n') || (c == '\r'))
        {
          break;
        }
      }
      p = handleLineEnd(p);
      continue;
    }

    // Check if it's a blank line.
    if ((c == '\r') || (c == '\n'))
    {
      p = handleLineEnd(p);
      continue;
    }

    // Handle elements on the line.
    while (p < end)
    {
      c = *p;

      // Tag.
      if (c == '<')
      {
        p += 1;
        const char *tagStart = p;
        while (p < end)
        {
          c = *p++;
          if ((c == '\n') || (c == '\r'))
          {
            error("Unterminated tag");
            goto fatalError;
          }
          else if (c == '>')
          {
            break;
          }
        }
        if (p >= end)
        {
          error("Unterminated tag");
          goto fatalError;
        }
        StringSlice tag(tagStart, p - 1);
        if (tag == "region")
        {
          if (buildingRegion && (buildingRegion == &curRegion))
          {
            finishRegion(&curRegion);
          }
          curRegion = curGroup;
          buildingRegion = &curRegion;
          inControl = false;
        }
        else if (tag == "group")
        {
          if (buildingRegion && (buildingRegion == &curRegion))
          {
            finishRegion(&curRegion);
          }
          curGroup.clear();
          buildingRegion = &curGroup;
          inControl = false;
        }
        else if (tag == "control")
        {
          if (buildingRegion && (buildingRegion == &curRegion))
          {
            finishRegion(&curRegion);
          }
          curGroup.clear();
          buildingRegion = nullptr;
          inControl = true;
        }
        else
        {
          error("Illegal tag");
        }
      }
      // Comment.
      else if (c == '/')
      {
        // Skip to end of line.
        while (p < end)
        {
          c = *p;
          if ((c == '\r') || (c == '\n'))
          {
            break;
          }
          p += 1;
        }
      }
      // Parameter.
      else
      {
        // Get the parameter name.
        const char *parameterStart = p;
        while (p < end)
        {
          c = *p++;
          if ((c == '=') || (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
          {
            break;
          }
        }
        if ((p >= end) || (c != '='))
        {
          error("Malformed parameter");
          goto nextElement;
        }
        StringSlice opcode(parameterStart, p - 1);
        if (inControl)
        {
          if (opcode == "default_path")
          {
            p = readPathInto(&defaultPath, p, end);
          }
          else
          {
            const char *valueStart = p;
            while (p < end)
            {
              c = *p;
              if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
              {
                break;
              }
              p++;
            }
            water::String value(valueStart, p - valueStart);
            water::String fauxOpcode = water::String(opcode.getStart(), opcode.length()) + " (in <control>)";
            sound_->addUnsupportedOpcode(fauxOpcode);
          }
        }
        else if (opcode == "sample")
        {
          water::String path;
          p = readPathInto(&path, p, end);
          if (!path.isEmpty())
          {
            if (buildingRegion)
            {
              buildingRegion->sample = sound_->addSample(path, defaultPath);
            }
            else
            {
              error("Adding sample outside a group or region");
            }
          }
          else
          {
            error("Empty sample path");
          }
        }
        else
        {
          const char *valueStart = p;
          while (p < end)
          {
            c = *p;
            if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
            {
              break;
            }
            p++;
          }
          water::String value(valueStart, p - valueStart);
          if (buildingRegion == nullptr)
          {
            error("Setting a parameter outside a region or group");
          }
          else if (opcode == "lokey")
          {
            buildingRegion->lokey = keyValue(value);
          }
          else if (opcode == "hikey")
          {
            buildingRegion->hikey = keyValue(value);
          }
          else if (opcode == "key")
          {
            buildingRegion->hikey = buildingRegion->lokey = buildingRegion->pitch_keycenter = keyValue(value);
          }
          else if (opcode == "lovel")
          {
            buildingRegion->lovel = value.getIntValue();
          }
          else if (opcode == "hivel")
          {
            buildingRegion->hivel = value.getIntValue();
          }
          else if (opcode == "trigger")
          {
            buildingRegion->trigger = static_cast<Region::Trigger>(triggerValue(value));
          }
          else if (opcode == "group")
          {
            buildingRegion->group = static_cast<int>(value.getLargeIntValue());
          }
          else if (opcode == "off_by" || opcode == "offby")
          {
            buildingRegion->off_by = value.getLargeIntValue();
          }
          else if (opcode == "offset")
          {
            buildingRegion->offset = value.getLargeIntValue();
          }
          else if (opcode == "end")
          {
            water::int64 end2 = value.getLargeIntValue();
            if (end2 < 0)
            {
              buildingRegion->negative_end = true;
            }
            else
            {
              buildingRegion->end = end2;
            }
          }
          else if (opcode == "loop_mode" || opcode == "loopmode")
          {
            bool modeIsSupported = value == "no_loop" || value == "one_shot" || value == "loop_continuous";
            if (modeIsSupported)
            {
              buildingRegion->loop_mode = static_cast<Region::LoopMode>(loopModeValue(value));
            }
            else
            {
              water::String fauxOpcode = water::String(opcode.getStart(), opcode.length()) + "=" + value;
              sound_->addUnsupportedOpcode(fauxOpcode);
            }
          }
          else if (opcode == "loop_start" || opcode == "loopstart")
          {
            buildingRegion->loop_start = value.getLargeIntValue();
          }
          else if (opcode == "loop_end" || opcode == "loopend")
          {
            buildingRegion->loop_end = value.getLargeIntValue();
          }
          else if (opcode == "transpose")
          {
            buildingRegion->transpose = value.getIntValue();
          }
          else if (opcode == "tune")
          {
            buildingRegion->tune = value.getIntValue();
          }
          else if (opcode == "pitch_keycenter")
          {
            buildingRegion->pitch_keycenter = keyValue(value);
          }
          else if (opcode == "pitch_keytrack")
          {
            buildingRegion->pitch_keytrack = value.getIntValue();
          }
          else if (opcode == "bend_up" || opcode == "bendup")
          {
            buildingRegion->bend_up = value.getIntValue();
          }
          else if (opcode == "bend_down" || opcode == "benddown")
          {
            buildingRegion->bend_down = value.getIntValue();
          }
          else if (opcode == "volume")
          {
            buildingRegion->volume = value.getFloatValue();
          }
          else if (opcode == "pan")
          {
            buildingRegion->pan = value.getFloatValue();
          }
          else if (opcode == "amp_veltrack")
          {
            buildingRegion->amp_veltrack = value.getFloatValue();
          }
          else if (opcode == "ampeg_delay")
          {
            buildingRegion->ampeg.delay = value.getFloatValue();
          }
          else if (opcode == "ampeg_start")
          {
            buildingRegion->ampeg.start = value.getFloatValue();
          }
          else if (opcode == "ampeg_attack")
          {
            buildingRegion->ampeg.attack = value.getFloatValue();
          }
          else if (opcode == "ampeg_hold")
          {
            buildingRegion->ampeg.hold = value.getFloatValue();
          }
          else if (opcode == "ampeg_decay")
          {
            buildingRegion->ampeg.decay = value.getFloatValue();
          }
          else if (opcode == "ampeg_sustain")
          {
            buildingRegion->ampeg.sustain = value.getFloatValue();
          }
          else if (opcode == "ampeg_release")
          {
            buildingRegion->ampeg.release = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2delay")
          {
            buildingRegion->ampeg_veltrack.delay = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2attack")
          {
            buildingRegion->ampeg_veltrack.attack = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2hold")
          {
            buildingRegion->ampeg_veltrack.hold = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2decay")
          {
            buildingRegion->ampeg_veltrack.decay = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2sustain")
          {
            buildingRegion->ampeg_veltrack.sustain = value.getFloatValue();
          }
          else if (opcode == "ampeg_vel2release")
          {
            buildingRegion->ampeg_veltrack.release = value.getFloatValue();
          }
          else if (opcode == "default_path")
          {
            error("\"default_path\" outside of <control> tag");
          }
          else
          {
            sound_->addUnsupportedOpcode(water::String(opcode.getStart(), opcode.length()));
          }
        }
      }

    // Skip to next element.
    nextElement:
      c = 0;
      while (p < end)
      {
        c = *p;
        if ((c != ' ') && (c != '\t'))
        {
          break;
        }
        p += 1;
      }
      if ((c == '\r') || (c == '\n'))
      {
        p = handleLineEnd(p);
        break;
      }
    }
  }

fatalError:
  if (buildingRegion && (buildingRegion == &curRegion))
  {
    finishRegion(buildingRegion);
  }
}

const char *Reader::handleLineEnd(const char *p)
{
  // Check for DOS-style line ending.
  char lineEndChar = *p++;

  if ((lineEndChar == '\r') && (*p == '\n'))
  {
    p += 1;
  }
  line_ += 1;
  return p;
}

const char *Reader::readPathInto(water::String *pathOut, const char *pIn, const char *endIn)
{
  // Paths are kind of funny to parse because they can contain whitespace.
  const char *p = pIn;
  const char *end = endIn;
  const char *pathStart = p;
  const char *potentialEnd = nullptr;

  while (p < end)
  {
    char c = *p;
    if (c == ' ')
    {
      // Is this space part of the path?  Or the start of the next opcode?  We
      // don't know yet.
      potentialEnd = p;
      p += 1;
      // Skip any more spaces.
      while (p < end && *p == ' ')
      {
        p += 1;
      }
    }
    else if ((c == '\n') || (c == '\r') || (c == '\t'))
    {
      break;
    }
    else if (c == '=')
    {
      // We've been looking at an opcode; we need to rewind to
      // potentialEnd.
      p = potentialEnd;
      break;
    }
    p += 1;
  }
  if (p > pathStart)
  {
    // Can't do this:
    //      water::String path(CharPointer_UTF8(pathStart), CharPointer_UTF8(p));
    // It won't compile for some unfathomable reason.
    water::CharPointer_UTF8 end2(p);
    water::String path(water::CharPointer_UTF8(pathStart), end2);
    *pathOut = path;
  }
  else
  {
    *pathOut = water::String();
  }
  return p;
}

int Reader::keyValue(const water::String &str)
{
  auto chars = str.toRawUTF8();

  char c = chars[0];

  if ((c >= '0') && (c <= '9'))
  {
    return str.getIntValue();
  }

  int note = 0;
  static const int notes[] = {
      12 + 0, 12 + 2, 3, 5, 7, 8, 10,
  };
  if ((c >= 'A') && (c <= 'G'))
  {
    note = notes[c - 'A'];
  }
  else if ((c >= 'a') && (c <= 'g'))
  {
    note = notes[c - 'a'];
  }
  int octaveStart = 1;

  c = chars[1];
  if ((c == 'b') || (c == '#'))
  {
    octaveStart += 1;
    if (c == 'b')
    {
      note -= 1;
    }
    else
    {
      note += 1;
    }
  }

  int octave = str.substring(octaveStart).getIntValue();
  // A3 == 57.
  int result = octave * 12 + note + (57 - 4 * 12);
  return result;
}

int Reader::triggerValue(const water::String &str)
{
  if (str == "release")
  {
    return Region::release;
  }
  if (str == "first")
  {
    return Region::first;
  }
  if (str == "legato")
  {
    return Region::legato;
  }
  return Region::attack;
}

int Reader::loopModeValue(const water::String &str)
{
  if (str == "no_loop")
  {
    return Region::no_loop;
  }
  if (str == "one_shot")
  {
    return Region::one_shot;
  }
  if (str == "loop_continuous")
  {
    return Region::loop_continuous;
  }
  if (str == "loop_sustain")
  {
    return Region::loop_sustain;
  }
  return Region::sample_loop;
}

void Reader::finishRegion(Region *region)
{
  Region *newRegion = new Region();

  *newRegion = *region;
  sound_->addRegion(newRegion);
}

void Reader::error(const water::String &message)
{
  water::String fullMessage = message;

  fullMessage += " (line " + water::String(line_) + ").";
  sound_->addError(fullMessage);
}

}
