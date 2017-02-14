/* Copyright 2016, Ableton AG, Berlin. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  If you would like to incorporate Link into a proprietary software application,
 *  please contact <link-devs@ableton.com>.
 */

#pragma once

// Make sure to define this before <cmath> is included for Windows
#define _USE_MATH_DEFINES
#include "ableton/Link.hpp"
#include <mutex>

struct LinkTimeInfo {
    double bpm, beats, phase;
};

namespace ableton
{
namespace linkaudio
{

class AudioEngine
{
public:
  AudioEngine(Link& link);
  double beatTime() const;
  void setTempo(double tempo);
  double quantum() const;
  void setQuantum(double quantum);

  void timelineCallback(const std::chrono::microseconds hostTime, LinkTimeInfo* const info);

private:
  struct EngineData
  {
    double requestedTempo;
    bool resetBeatTime;
    double quantum;
  };

  EngineData pullEngineData();

  Link& mLink;
  EngineData mSharedEngineData;
  std::mutex mEngineDataGuard;

  friend class AudioPlatform;
};


} // namespace linkaudio
} // namespace ableton
