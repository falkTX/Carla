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

namespace ableton
{
namespace link
{

AudioEngine::AudioEngine(Link& link)
  : mLink(link)
  , mSharedEngineData({0., false, false, 4., false})
  , mLockfreeEngineData(mSharedEngineData)
  , mIsPlaying(false)
{
}

void AudioEngine::startPlaying()
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestStart = true;
}

void AudioEngine::stopPlaying()
{
  std::lock_guard<std::mutex> lock(mEngineDataGuard);
  mSharedEngineData.requestStop = true;
}

bool AudioEngine::isPlaying() const
{
  return mLink.captureAppSessionState().isPlaying();
}

double AudioEngine::beatTime() const
{
  const auto sessionState = mLink.captureAppSessionState();
  return sessionState.beatAtTime(mLink.clock().micros(), mSharedEngineData.quantum);
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
}

bool AudioEngine::isStartStopSyncEnabled() const
{
  return mLink.isStartStopSyncEnabled();
}

void AudioEngine::setStartStopSyncEnabled(const bool enabled)
{
  mLink.enableStartStopSync(enabled);
}

AudioEngine::EngineData AudioEngine::pullEngineData()
{
  auto engineData = EngineData{};
  if (mEngineDataGuard.try_lock())
  {
    engineData.requestedTempo = mSharedEngineData.requestedTempo;
    mSharedEngineData.requestedTempo = 0;
    engineData.requestStart = mSharedEngineData.requestStart;
    mSharedEngineData.requestStart = false;
    engineData.requestStop = mSharedEngineData.requestStop;
    mSharedEngineData.requestStop = false;

    mLockfreeEngineData.quantum = mSharedEngineData.quantum;
    mLockfreeEngineData.startStopSyncOn = mSharedEngineData.startStopSyncOn;

    mEngineDataGuard.unlock();
  }
  engineData.quantum = mLockfreeEngineData.quantum;

  return engineData;
}

void AudioEngine::timelineCallback(const std::chrono::microseconds hostTime, LinkTimeInfo* const info)
{
  const auto engineData = pullEngineData();

  auto sessionState = mLink.captureAudioSessionState();

  if (engineData.requestStart)
  {
    sessionState.setIsPlaying(true, hostTime);
  }

  if (engineData.requestStop)
  {
    sessionState.setIsPlaying(false, hostTime);
  }

  if (!mIsPlaying && sessionState.isPlaying())
  {
    // Reset the timeline so that beat 0 corresponds to the time when transport starts
    sessionState.requestBeatAtStartPlayingTime(0, engineData.quantum);
    mIsPlaying = true;
  }
  else if (mIsPlaying && !sessionState.isPlaying())
  {
    mIsPlaying = false;
  }

  if (engineData.requestedTempo > 0)
  {
    // Set the newly requested tempo from the beginning of this buffer
    sessionState.setTempo(engineData.requestedTempo, hostTime);
  }

  // Timeline modifications are complete, commit the results
  mLink.commitAudioSessionState(sessionState);

  // Save session state
  info->beatsPerBar    = engineData.quantum;
  info->beatsPerMinute = sessionState.tempo();
  info->beat           = sessionState.beatAtTime(hostTime, engineData.quantum);
  info->phase          = sessionState.phaseAtTime(hostTime, engineData.quantum);
  info->playing        = mIsPlaying;
}

} // namespace link
} // namespace ableton
