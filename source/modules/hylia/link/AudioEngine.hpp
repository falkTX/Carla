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

#ifdef LINK_PLATFORM_WINDOWS
#define _USE_MATH_DEFINES
#if __BIG_ENDIAN__
# define htonll(x) (x)
# define ntohll(x) (x)
#else
# define htonll(x) ((uint64_t)htonl(((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
# define ntohll(x) ((uint64_t)ntohl(((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif
#endif

#include <cmath>

#include "ableton/Link.hpp"

struct LinkTimeInfo {
    double beatsPerBar, beatsPerMinute, beat, phase;
    bool playing;
};

namespace ableton
{
namespace link
{

class AudioEngine
{
public:
  AudioEngine(Link& link);
  void startPlaying();
  void stopPlaying();
  bool isPlaying() const;
  double beatTime() const;
  void setTempo(double tempo);
  double quantum() const;
  void setQuantum(double quantum);
  bool isStartStopSyncEnabled() const;
  void setStartStopSyncEnabled(bool enabled);

  void timelineCallback(const std::chrono::microseconds hostTime, LinkTimeInfo* const info);

private:
  struct EngineData
  {
    double requestedTempo;
    bool requestStart;
    bool requestStop;
    double quantum;
    bool startStopSyncOn;
  };

  EngineData pullEngineData();

  Link& mLink;
  EngineData mSharedEngineData;
  EngineData mLockfreeEngineData;
  bool mIsPlaying;
  std::mutex mEngineDataGuard;
};


} // namespace link
} // namespace ableton
