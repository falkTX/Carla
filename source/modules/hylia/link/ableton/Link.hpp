/*! @file Link.hpp
 *  @copyright 2016, Ableton AG, Berlin. All rights reserved.
 *  @brief Library for cross-device shared tempo and quantized beat grid
 *
 *  @license:
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

#include <ableton/platforms/Config.hpp>
#include <chrono>
#include <mutex>

namespace ableton
{

/*! @class Link
 *  @brief Class that represents a participant in a Link session.
 *
 *  @discussion Each Link instance has its own beat timeline that
 *  starts running from beat 0 at the initial tempo when
 *  constructed. A Link instance is initially disabled after
 *  construction, which means that it will not communicate on the
 *  network. Once enabled, a Link instance initiates network
 *  communication in an effort to discover other peers. When peers are
 *  discovered, they immediately become part of a shared Link session.
 *
 *  Each method of the Link type documents its thread-safety and
 *  realtime-safety properties. When a method is marked thread-safe,
 *  it means it is safe to call from multiple threads
 *  concurrently. When a method is marked realtime-safe, it means that
 *  it does not block and is appropriate for use in the thread that
 *  performs audio IO.
 *
 *  Link provides one Timeline capture/commit method pair for use in the
 *  audio thread and one for all other application contexts. In
 *  general, modifying the Link timeline should be done in the audio
 *  thread for the most accurate timing results. The ability to modify
 *  the Link timeline from application threads should only be used in
 *  cases where an application's audio thread is not actively running
 *  or if it doesn't generate audio at all. Modifying the Link
 *  timeline from both the audio thread and an application thread
 *  concurrently is not advised and will potentially lead to
 *  unexpected behavior.
 */
class Link
{
public:
  using Clock = link::platform::Clock;
  class Timeline;

  /*! @brief Construct with an initial tempo. */
  Link(double bpm);

  /*! @brief Link instances cannot be copied or moved */
  Link(const Link&) = delete;
  Link& operator=(const Link&) = delete;
  Link(Link&&) = delete;
  Link& operator=(Link&&) = delete;

  /*! @brief Is Link currently enabled?
   *  Thread-safe: yes
   *  Realtime-safe: yes
   */
  bool isEnabled() const;

  /*! @brief Enable/disable Link.
   *  Thread-safe: yes
   *  Realtime-safe: no
   */
  void enable(bool bEnable);

  /*! @brief How many peers are currently connected in a Link session?
   *  Thread-safe: yes
   *  Realtime-safe: yes
   */
  std::size_t numPeers() const;

  /*! @brief Register a callback to be notified when the number of
   *  peers in the Link session changes.
   *  Thread-safe: yes
   *  Realtime-safe: no
   *
   *  @discussion The callback is invoked on a Link-managed thread.
   *
   *  @param callback The callback signature is:
   *  void (std::size_t numPeers)
   */
  template <typename Callback>
  void setNumPeersCallback(Callback callback);

  /*! @brief Register a callback to be notified when the session
   *  tempo changes.
   *  Thread-safe: yes
   *  Realtime-safe: no
   *
   *  @discussion The callback is invoked on a Link-managed thread.
   *
   *  @param callback The callback signature is: void (double bpm)
   */
  template <typename Callback>
  void setTempoCallback(Callback callback);

  /*! @brief The clock used by Link.
   *  Thread-safe: yes
   *  Realtime-safe: yes
   *
   *  @discussion The Clock type is a platform-dependent
   *  representation of the system clock. It exposes a ticks() method
   *  that returns the current ticks of the system clock as well as
   *  micros(), which is a normalized representation of the current system
   *  time in std::chrono::microseconds. It also provides conversion
   *  functions ticksToMicros() and microsToTicks() to faciliate
   *  converting between these units.
   */
  Clock clock() const;

  /*! @brief Capture the current Link timeline from the audio thread.
   *  Thread-safe: no
   *  Realtime-safe: yes
   *
   *  @discussion This method should ONLY be called in the audio thread
   *  and must not be accessed from any other threads. The returned
   *  Timeline stores a snapshot of the current Link state, so it
   *  should be captured and used in a local scope. Storing the
   *  Timeline for later use in a different context is not advised
   *  because it will provide an outdated view on the Link state.
   */
  Timeline captureAudioTimeline() const;

  /*! @brief Commit the given timeline to the Link session from the
   *  audio thread.
   *  Thread-safe: no
   *  Realtime-safe: yes
   *
   *  @discussion This method should ONLY be called in the audio
   *  thread. The given timeline will replace the current Link
   *  timeline. Modifications to the session based on the new timeline
   *  will be communicated to other peers in the session.
   */
  void commitAudioTimeline(Timeline timeline);

  /*! @brief Capture the current Link timeline from an application
   *  thread.
   *  Thread-safe: yes
   *  Realtime-safe: no
   *
   *  @discussion Provides a mechanism for capturing the Link timeline
   *  from an application thread (other than the audio thread). The
   *  returned Timeline stores a snapshot of the current Link state,
   *  so it should be captured and used in a local scope. Storing the
   *  Timeline for later use in a different context is not advised
   *  because it will provide an outdated view on the Link state.
   */
  Timeline captureAppTimeline() const;

  /*! @brief Commit the given timeline to the Link session from an
   *  application thread.
   *  Thread-safe: yes
   *  Realtime-safe: no
   *
   *  @discussion The given timeline will replace the current Link
   *  timeline. Modifications to the session based on the new timeline
   *  will be communicated to other peers in the session.
   */
  void commitAppTimeline(Timeline timeline);

  /*! @class Timeline
   *  @brief Representation of a mapping between time and beats for
   *  varying quanta.
   *
   *  @discussion A Timeline object is intended for use in a local
   *  scope within a single thread - none of its methods are
   *  thread-safe. All of its methods are non-blocking, so it is safe
   *  to use from a realtime thread.
   */
  class Timeline
  {
  public:
    Timeline(const link::Timeline timeline, const bool bRespectQuantum);

    /*! @brief: The tempo of the timeline, in bpm */
    double tempo() const;

    /*! @brief: Set the timeline tempo to the given bpm value, taking
     *  effect at the given time.
     */
    void setTempo(double bpm, std::chrono::microseconds atTime);

    /*! @brief: Get the beat value corresponding to the given time
     *  for the given quantum.
     *
     *  @discussion: The magnitude of the resulting beat value is
     *  unique to this Link instance, but its phase with respect to
     *  the provided quantum is shared among all session
     *  peers. For non-negative beat values, the following
     *  property holds: fmod(beatAtTime(t, q), q) == phaseAtTime(t, q)
     */
    double beatAtTime(std::chrono::microseconds time, double quantum) const;

    /*! @brief: Get the session phase at the given time for the given
     *  quantum.
     *
     *  @discussion: The result is in the interval [0, quantum). The
     *  result is equivalent to fmod(beatAtTime(t, q), q) for
     *  non-negative beat values. This method is convenient if the
     *  client is only interested in the phase and not the beat
     *  magnitude. Also, unlike fmod, it handles negative beat values
     *  correctly.
     */
    double phaseAtTime(std::chrono::microseconds time, double quantum) const;

    /*! @brief: Get the time at which the given beat occurs for the
     *  given quantum.
     *
     *  @discussion: The inverse of beatAtTime, assuming a constant
     *  tempo. beatAtTime(timeAtBeat(b, q), q) === b.
     */
    std::chrono::microseconds timeAtBeat(double beat, double quantum) const;

    /*! @brief: Attempt to map the given beat to the given time in the
     *  context of the given quantum.
     *
     *  @discussion: This method behaves differently depending on the
     *  state of the session. If no other peers are connected,
     *  then this instance is in a session by itself and is free to
     *  re-map the beat/time relationship whenever it pleases. In this
     *  case, beatAtTime(time, quantum) == beat after this method has
     *  been called.
     *
     *  If there are other peers in the session, this instance
     *  should not abruptly re-map the beat/time relationship in the
     *  session because that would lead to beat discontinuities among
     *  the other peers. In this case, the given beat will be mapped
     *  to the next time value greater than the given time with the
     *  same phase as the given beat.
     *
     *  This method is specifically designed to enable the concept of
     *  "quantized launch" in client applications. If there are no other
     *  peers in the session, then an event (such as starting
     *  transport) happens immediately when it is requested. If there
     *  are other peers, however, we wait until the next time at which
     *  the session phase matches the phase of the event, thereby
     *  executing the event in-phase with the other peers in the
     *  session. The client only needs to invoke this method to
     *  achieve this behavior and should not need to explicitly check
     *  the number of peers.
     */
    void requestBeatAtTime(double beat, std::chrono::microseconds time, double quantum);

    /*! @brief: Rudely re-map the beat/time relationship for all peers
     *  in a session.
     *
     *  @discussion: DANGER: This method should only be needed in
     *  certain special circumstances. Most applications should not
     *  use it. It is very similar to requestBeatAtTime except that it
     *  does not fall back to the quantizing behavior when it is in a
     *  session with other peers. Calling this method will
     *  unconditionally map the given beat to the given time and
     *  broadcast the result to the session. This is very anti-social
     *  behavior and should be avoided.
     *
     *  One of the few legitimate uses of this method is to
     *  synchronize a Link session with an external clock source. By
     *  periodically forcing the beat/time mapping according to an
     *  external clock source, a peer can effectively bridge that
     *  clock into a Link session. Much care must be taken at the
     *  application layer when implementing such a feature so that
     *  users do not accidentally disrupt Link sessions that they may
     *  join.
     */
    void forceBeatAtTime(double beat, std::chrono::microseconds time, double quantum);

  private:
    friend Link;
    link::Timeline mOriginalTimeline;
    bool mbRespectQuantum;
    link::Timeline mTimeline;
  };

private:
  std::mutex mCallbackMutex;
  link::PeerCountCallback mPeerCountCallback;
  link::TempoCallback mTempoCallback;
  Clock mClock;
  link::platform::Controller mController;
};

} // ableton

#include <ableton/Link.ipp>
