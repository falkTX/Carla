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
namespace detail
{

inline Link::SessionState toSessionState(
  const link::ClientState& state, const bool isConnected)
{
  const auto time = state.timeline.fromBeats(state.startStopState.beats);
  const auto startStopState =
    link::ApiStartStopState{state.startStopState.isPlaying, time};
  return {{state.timeline, startStopState}, isConnected};
}

inline link::IncomingClientState toIncomingClientState(const link::ApiState& state,
  const link::ApiState& originalState,
  const std::chrono::microseconds timestamp)
{
  const auto timeline = originalState.timeline != state.timeline
                          ? link::OptionalTimeline{state.timeline}
                          : link::OptionalTimeline{};
  const auto startStopState =
    originalState.startStopState != state.startStopState
      ? link::OptionalStartStopState{{state.startStopState.isPlaying,
          state.timeline.toBeats(state.startStopState.time), timestamp}}
      : link::OptionalStartStopState{};
  return {timeline, startStopState, timestamp};
}

} // namespace detail

inline Link::Link(const double bpm)
  : mPeerCountCallback([](std::size_t) {})
  , mTempoCallback([](link::Tempo) {})
  , mStartStopCallback([](bool) {})
  , mClock{}
  , mController(link::Tempo(bpm),
      [this](const std::size_t peers) {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        mPeerCountCallback(peers);
      },
      [this](const link::Tempo tempo) {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        mTempoCallback(tempo);
      },
      [this](const bool isPlaying) {
        std::lock_guard<std::mutex> lock(mCallbackMutex);
        mStartStopCallback(isPlaying);
      },
      mClock,
      util::injectVal(link::platform::IoContext{}))
{
}

inline bool Link::isEnabled() const
{
  return mController.isEnabled();
}

inline void Link::enable(const bool bEnable)
{
  mController.enable(bEnable);
}

inline bool Link::isStartStopSyncEnabled() const
{
  return mController.isStartStopSyncEnabled();
}

inline void Link::enableStartStopSync(bool bEnable)
{
  mController.enableStartStopSync(bEnable);
}

inline std::size_t Link::numPeers() const
{
  return mController.numPeers();
}

template <typename Callback>
void Link::setNumPeersCallback(Callback callback)
{
  std::lock_guard<std::mutex> lock(mCallbackMutex);
  mPeerCountCallback = [callback](const std::size_t numPeers) { callback(numPeers); };
}

template <typename Callback>
void Link::setTempoCallback(Callback callback)
{
  std::lock_guard<std::mutex> lock(mCallbackMutex);
  mTempoCallback = [callback](const link::Tempo tempo) { callback(tempo.bpm()); };
}

template <typename Callback>
void Link::setStartStopCallback(Callback callback)
{
  std::lock_guard<std::mutex> lock(mCallbackMutex);
  mStartStopCallback = callback;
}

inline Link::Clock Link::clock() const
{
  return mClock;
}

inline Link::SessionState Link::captureAudioSessionState() const
{
  return detail::toSessionState(mController.clientStateRtSafe(), numPeers() > 0);
}

inline void Link::commitAudioSessionState(const Link::SessionState state)
{
  mController.setClientStateRtSafe(
    detail::toIncomingClientState(state.mState, state.mOriginalState, mClock.micros()));
}

inline Link::SessionState Link::captureAppSessionState() const
{
  return detail::toSessionState(mController.clientState(), numPeers() > 0);
}

inline void Link::commitAppSessionState(const Link::SessionState state)
{
  mController.setClientState(
    detail::toIncomingClientState(state.mState, state.mOriginalState, mClock.micros()));
}

// Link::SessionState

inline Link::SessionState::SessionState(
  const link::ApiState state, const bool bRespectQuantum)
  : mOriginalState(state)
  , mState(state)
  , mbRespectQuantum(bRespectQuantum)
{
}

inline double Link::SessionState::tempo() const
{
  return mState.timeline.tempo.bpm();
}

inline void Link::SessionState::setTempo(
  const double bpm, const std::chrono::microseconds atTime)
{
  const auto desiredTl = link::clampTempo(
    link::Timeline{link::Tempo(bpm), mState.timeline.toBeats(atTime), atTime});
  mState.timeline.tempo = desiredTl.tempo;
  mState.timeline.timeOrigin = desiredTl.fromBeats(mState.timeline.beatOrigin);
}

inline double Link::SessionState::beatAtTime(
  const std::chrono::microseconds time, const double quantum) const
{
  return link::toPhaseEncodedBeats(mState.timeline, time, link::Beats{quantum})
    .floating();
}

inline double Link::SessionState::phaseAtTime(
  const std::chrono::microseconds time, const double quantum) const
{
  return link::phase(link::Beats{beatAtTime(time, quantum)}, link::Beats{quantum})
    .floating();
}

inline std::chrono::microseconds Link::SessionState::timeAtBeat(
  const double beat, const double quantum) const
{
  return link::fromPhaseEncodedBeats(
    mState.timeline, link::Beats{beat}, link::Beats{quantum});
}

inline void Link::SessionState::requestBeatAtTime(
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

inline void Link::SessionState::forceBeatAtTime(
  const double beat, const std::chrono::microseconds time, const double quantum)
{
  // There are two components to the beat adjustment: a phase shift
  // and a beat magnitude adjustment.
  const auto curBeatAtTime = link::Beats{beatAtTime(time, quantum)};
  const auto closestInPhase =
    link::closestPhaseMatch(curBeatAtTime, link::Beats{beat}, link::Beats{quantum});
  mState.timeline = shiftClientTimeline(mState.timeline, closestInPhase - curBeatAtTime);
  // Now adjust the magnitude
  mState.timeline.beatOrigin =
    mState.timeline.beatOrigin + (link::Beats{beat} - closestInPhase);
}

inline void Link::SessionState::setIsPlaying(
  const bool isPlaying, const std::chrono::microseconds time)
{
  mState.startStopState = {isPlaying, time};
}

inline bool Link::SessionState::isPlaying() const
{
  return mState.startStopState.isPlaying;
}

inline std::chrono::microseconds Link::SessionState::timeForIsPlaying() const
{
  return mState.startStopState.time;
}

inline void Link::SessionState::requestBeatAtStartPlayingTime(
  const double beat, const double quantum)
{
  if (isPlaying())
  {
    requestBeatAtTime(beat, mState.startStopState.time, quantum);
  }
}

inline void Link::SessionState::setIsPlayingAndRequestBeatAtTime(
  bool isPlaying, std::chrono::microseconds time, double beat, double quantum)
{
  mState.startStopState = {isPlaying, time};
  requestBeatAtStartPlayingTime(beat, quantum);
}

} // namespace ableton
