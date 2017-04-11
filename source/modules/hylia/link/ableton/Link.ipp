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

#include <ableton/link/Phase.hpp>

namespace ableton
{

template <typename Clock>
inline BasicLink<Clock>::BasicLink(const double bpm)
  : mPeerCountCallback([](std::size_t) {})
  , mTempoCallback([](link::Tempo) {})
  , mController(link::Tempo(bpm),
      [this](const std::size_t peers) {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        mPeerCountCallback(peers);
      },
      [this](const link::Tempo tempo) {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        mTempoCallback(tempo);
      },
      mClock,
      util::injectVal(link::platform::IoContext{}))
{
}

template <typename Clock>
inline bool BasicLink<Clock>::isEnabled() const
{
  return mController.isEnabled();
}

template <typename Clock>
inline void BasicLink<Clock>::enable(const bool bEnable)
{
  mController.enable(bEnable);
}

template <typename Clock>
inline std::size_t BasicLink<Clock>::numPeers() const
{
  return mController.numPeers();
}

template <typename Clock>
template <typename Callback>
void BasicLink<Clock>::setNumPeersCallback(Callback callback)
{
  std::lock_guard<std::mutex> lock(mCallbackMutex);
  mPeerCountCallback = [callback](const std::size_t numPeers) { callback(numPeers); };
}

template <typename Clock>
template <typename Callback>
void BasicLink<Clock>::setTempoCallback(Callback callback)
{
  std::lock_guard<std::mutex> lock(mCallbackMutex);
  mTempoCallback = [callback](const link::Tempo tempo) { callback(tempo.bpm()); };
}

template <typename Clock>
inline Clock BasicLink<Clock>::clock() const
{
  return mClock;
}

template <typename Clock>
inline typename BasicLink<Clock>::Timeline BasicLink<Clock>::captureAudioTimeline() const
{
  return BasicLink<Clock>::Timeline{mController.timelineRtSafe(), numPeers() > 0};
}

template <typename Clock>
inline void BasicLink<Clock>::commitAudioTimeline(const Timeline timeline)
{
  if (timeline.mOriginalTimeline != timeline.mTimeline)
  {
    mController.setTimelineRtSafe(timeline.mTimeline, mClock.micros());
  }
}

template <typename Clock>
inline typename BasicLink<Clock>::Timeline BasicLink<Clock>::captureAppTimeline() const
{
  return Timeline{mController.timeline(), numPeers() > 0};
}

template <typename Clock>
inline void BasicLink<Clock>::commitAppTimeline(const Timeline timeline)
{
  if (timeline.mOriginalTimeline != timeline.mTimeline)
  {
    mController.setTimeline(timeline.mTimeline, mClock.micros());
  }
}

////////////////////
// Link::Timeline //
////////////////////

template <typename Clock>
inline BasicLink<Clock>::Timeline::Timeline(
  const link::Timeline timeline, const bool bRespectQuantum)
  : mOriginalTimeline(timeline)
  , mbRespectQuantum(bRespectQuantum)
  , mTimeline(timeline)
{
}

template <typename Clock>
inline double BasicLink<Clock>::Timeline::tempo() const
{
  return mTimeline.tempo.bpm();
}

template <typename Clock>
inline void BasicLink<Clock>::Timeline::setTempo(
  const double bpm, const std::chrono::microseconds atTime)
{
  const auto desiredTl =
    link::clampTempo(link::Timeline{link::Tempo(bpm), mTimeline.toBeats(atTime), atTime});
  mTimeline.tempo = desiredTl.tempo;
  mTimeline.timeOrigin = desiredTl.fromBeats(mTimeline.beatOrigin);
}

template <typename Clock>
inline double BasicLink<Clock>::Timeline::beatAtTime(
  const std::chrono::microseconds time, const double quantum) const
{
  return link::toPhaseEncodedBeats(mTimeline, time, link::Beats{quantum}).floating();
}

template <typename Clock>
inline double BasicLink<Clock>::Timeline::phaseAtTime(
  const std::chrono::microseconds time, const double quantum) const
{
  return link::phase(link::Beats{beatAtTime(time, quantum)}, link::Beats{quantum})
    .floating();
}

template <typename Clock>
inline std::chrono::microseconds BasicLink<Clock>::Timeline::timeAtBeat(
  const double beat, const double quantum) const
{
  return link::fromPhaseEncodedBeats(mTimeline, link::Beats{beat}, link::Beats{quantum});
}

template <typename Clock>
inline void BasicLink<Clock>::Timeline::requestBeatAtTime(
  const double beat, std::chrono::microseconds time, const double quantum)
{
  if (mbRespectQuantum)
  {
    time = timeAtBeat(link::nextPhaseMatch(link::Beats{beatAtTime(time, quantum)},
                        link::Beats{beat}, link::Beats{quantum})
                        .floating(),
      quantum);
  }
  forceBeatAtTime(beat, time, quantum);
}

template <typename Clock>
inline void BasicLink<Clock>::Timeline::forceBeatAtTime(
  const double beat, const std::chrono::microseconds time, const double quantum)
{
  // There are two components to the beat adjustment: a phase shift
  // and a beat magnitude adjustment.
  const auto curBeatAtTime = link::Beats{beatAtTime(time, quantum)};
  const auto closestInPhase =
    link::closestPhaseMatch(curBeatAtTime, link::Beats{beat}, link::Beats{quantum});
  mTimeline = shiftClientTimeline(mTimeline, closestInPhase - curBeatAtTime);
  // Now adjust the magnitude
  mTimeline.beatOrigin = mTimeline.beatOrigin + (link::Beats{beat} - closestInPhase);
}

} // ableton
