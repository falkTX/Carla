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

#include "AudioEngine.hpp"

// Make sure to define this before <cmath> is included for Windows
#define _USE_MATH_DEFINES
#include <cmath>

namespace ableton
{
namespace linkaudio
{

AudioEngine::AudioEngine(Link& link)
  : mLink(link)
  , mSharedEngineData({0., false, 4.})
{
}

double AudioEngine::beatTime() const
{
  const auto timeline = mLink.captureAppTimeline();
  return timeline.beatAtTime(mLink.clock().micros(), mSharedEngineData.quantum);
}

void AudioEngine::setTempo(double tempo)
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestedTempo = tempo;
}

double AudioEngine::quantum() const
{
  return mSharedEngineData.quantum;
}

void AudioEngine::setQuantum(double quantum)
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.quantum = quantum;
  mSharedEngineData.resetBeatTime = true;
}

AudioEngine::EngineData AudioEngine::pullEngineData()
{
  auto engineData = EngineData{};
  if (mEngineDataGuard.try_lock())
  {
    engineData.requestedTempo = mSharedEngineData.requestedTempo;
    mSharedEngineData.requestedTempo = 0;
    engineData.resetBeatTime = mSharedEngineData.resetBeatTime;
    engineData.quantum = mSharedEngineData.quantum;
    mSharedEngineData.resetBeatTime = false;

    mEngineDataGuard.unlock();
  }

  return engineData;
}

void AudioEngine::timelineCallback(const std::chrono::microseconds hostTime, LinkTimeInfo* const info)
{
  const auto engineData = pullEngineData();

  auto timeline = mLink.captureAudioTimeline();

  if (engineData.resetBeatTime)
  {
    // Reset the timeline so that beat 0 lands at the beginning of
    // this buffer and clear the flag.
    timeline.requestBeatAtTime(0, hostTime, engineData.quantum);
  }

  if (engineData.requestedTempo > 0)
  {
    // Set the newly requested tempo from the beginning of this buffer
    timeline.setTempo(engineData.requestedTempo, hostTime);
  }

  // Timeline modifications are complete, commit the results
  mLink.commitAudioTimeline(timeline);

  // Save timeline info
  info->bpm   = timeline.tempo();
  info->beats = timeline.beatAtTime(hostTime, engineData.quantum);
  info->phase = timeline.phaseAtTime(hostTime, engineData.quantum);
}

} // namespace linkaudio
} // namespace ableton
