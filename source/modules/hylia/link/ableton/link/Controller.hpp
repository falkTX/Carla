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

#include <ableton/discovery/Service.hpp>
#include <ableton/link/ClientSessionTimelines.hpp>
#include <ableton/link/Gateway.hpp>
#include <ableton/link/GhostXForm.hpp>
#include <ableton/link/NodeState.hpp>
#include <ableton/link/Peers.hpp>
#include <ableton/link/Sessions.hpp>
#include <mutex>

namespace ableton
{
namespace link
{
namespace detail
{

template <typename Clock>
GhostXForm initXForm(const Clock& clock)
{
  // Make the current time map to a ghost time of 0 with ghost time
  // increasing at the same rate as clock time
  return {1.0, -clock.micros()};
}

// The timespan in which local modifications to the timeline will be
// preferred over any modifications coming from the network.
const auto kLocalModGracePeriod = std::chrono::seconds(1);

} // namespace detail

// function types corresponding to the Controller callback type params
using PeerCountCallback = std::function<void(std::size_t)>;
using TempoCallback = std::function<void(ableton::link::Tempo)>;

// The main Link controller
template <typename PeerCountCallback,
  typename TempoCallback,
  typename Clock,
  typename IoContext>
class Controller
{
public:
  using Ticks = typename Clock::Ticks;

  Controller(Tempo tempo,
    PeerCountCallback peerCallback,
    TempoCallback tempoCallback,
    Clock clock,
    util::Injected<IoContext> io)
    : mTempoCallback(std::move(tempoCallback))
    , mClock(std::move(clock))
    , mNodeId(NodeId::random())
    , mSessionId(mNodeId)
    , mGhostXForm(detail::initXForm(mClock))
    , mSessionTimeline(clampTempo({tempo, Beats{0.}, std::chrono::microseconds{0}}))
    , mClientTimeline({mSessionTimeline.tempo, Beats{0.},
        mGhostXForm.ghostToHost(std::chrono::microseconds{0})})
    , mRtClientTimeline(mClientTimeline)
    , mRtClientTimelineTimestamp(0)
    , mSessionPeerCounter(*this, std::move(peerCallback))
    , mEnabled(false)
    , mIo(std::move(io))
    , mRealtimeIo(util::injectRef(*mIo))
    , mPeers(util::injectRef(*mIo),
        std::ref(mSessionPeerCounter),
        SessionTimelineCallback{*this})
    , mSessions({mSessionId, mSessionTimeline, {mGhostXForm, mClock.micros()}},
        util::injectRef(mPeers),
        MeasurePeer{*this},
        JoinSessionCallback{*this},
        util::injectRef(*mIo),
        mClock)
    , mDiscovery(
        std::make_pair(NodeState{mNodeId, mSessionId, mSessionTimeline}, mGhostXForm),
        GatewayFactory{*this},
        util::injectVal(mIo->clone(UdpSendExceptionHandler{*this})))
  {
  }

  Controller(const Controller&) = delete;
  Controller(Controller&&) = delete;

  Controller& operator=(const Controller&) = delete;
  Controller& operator=(Controller&&) = delete;

  void enable(const bool bEnable)
  {
    const bool bWasEnabled = mEnabled.exchange(bEnable);
    if (bWasEnabled != bEnable)
    {
      mRealtimeIo.async([this, bEnable] {
        if (bEnable)
        {
          // Always reset when first enabling to avoid hijacking
          // tempo in existing sessions
          resetState();
        }
        mDiscovery.enable(bEnable);
      });
    }
  }

  bool isEnabled() const
  {
    return mEnabled;
  }

  std::size_t numPeers() const
  {
    return mSessionPeerCounter.mSessionPeerCount;
  }

  // Get the current Link timeline. Thread-safe but may block, so
  // it cannot be used from audio thread.
  Timeline timeline() const
  {
    std::lock_guard<std::mutex> lock(mClientTimelineGuard);
    return mClientTimeline;
  }

  // Set the timeline to be used, starting at the given
  // time. Thread-safe but may block, so it cannot be used from audio thread.
  void setTimeline(Timeline newTimeline, const std::chrono::microseconds atTime)
  {
    newTimeline = clampTempo(newTimeline);
    {
      std::lock_guard<std::mutex> lock(mClientTimelineGuard);
      mClientTimeline = newTimeline;
    }
    mIo->async([this, newTimeline, atTime] {
      handleTimelineFromClient(updateSessionTimelineFromClient(
        mSessionTimeline, newTimeline, atTime, mGhostXForm));
    });
  }

  // Non-blocking timeline access for a realtime context. NOT
  // thread-safe. Must not be called from multiple threads
  // concurrently and must not be called concurrently with setTimelineRtSafe.
  Timeline timelineRtSafe() const
  {
    // Respect the session timing guard but don't block on it. If we
    // can't access it because it's being modified we fall back to our
    // cached version of the timeline.
    const auto now = mClock.micros();
    if (now - mRtClientTimelineTimestamp > detail::kLocalModGracePeriod
        && mSessionTimingGuard.try_lock())
    {
      const auto clientTimeline = updateClientTimelineFromSession(
        mRtClientTimeline, mSessionTimeline, now, mGhostXForm);

      mSessionTimingGuard.unlock();

      if (clientTimeline != mRtClientTimeline)
      {
        mRtClientTimeline = clientTimeline;
      }
    }

    return mRtClientTimeline;
  }

  // should only be called from the audio thread
  void setTimelineRtSafe(Timeline newTimeline, const std::chrono::microseconds atTime)
  {
    newTimeline = clampTempo(newTimeline);
    // Cache the new timeline for serving back to the client
    mRtClientTimeline = newTimeline;
    mRtClientTimelineTimestamp =
      isEnabled() ? mClock.micros() : std::chrono::microseconds(0);

    // Update the session timeline from the new client timeline
    mRealtimeIo.async([this, newTimeline, atTime] {
      // Synchronize with the non-rt version of the client timeline
      {
        std::lock_guard<std::mutex> lock(mClientTimelineGuard);
        mClientTimeline = newTimeline;
      }
      handleTimelineFromClient(updateSessionTimelineFromClient(
        mSessionTimeline, newTimeline, atTime, mGhostXForm));
    });
  }

private:
  void updateSessionTiming(const Timeline newTimeline, const GhostXForm newXForm)
  {
    const auto oldTimeline = mSessionTimeline;
    const auto oldXForm = mGhostXForm;

    if (oldTimeline != newTimeline || oldXForm != newXForm)
    {
      {
        std::lock_guard<std::mutex> lock(mSessionTimingGuard);
        mSessionTimeline = newTimeline;
        mGhostXForm = newXForm;
      }

      // Update the client timeline based on the new session timing data
      {
        std::lock_guard<std::mutex> lock(mClientTimelineGuard);
        mClientTimeline = updateClientTimelineFromSession(
          mClientTimeline, mSessionTimeline, mClock.micros(), mGhostXForm);
      }

      // Push the change to the discovery service
      mDiscovery.updateNodeState(
        std::make_pair(NodeState{mNodeId, mSessionId, newTimeline}, newXForm));

      if (oldTimeline.tempo != newTimeline.tempo)
      {
        mTempoCallback(newTimeline.tempo);
      }
    }
  }

  void handleTimelineFromClient(Timeline tl)
  {
    mSessions.resetTimeline(tl);
    mPeers.setSessionTimeline(mSessionId, tl);
    updateSessionTiming(std::move(tl), mGhostXForm);
  }

  void handleTimelineFromSession(SessionId id, Timeline timeline)
  {
    debug(mIo->log()) << "Received timeline with tempo: " << timeline.tempo.bpm()
                      << " for session: " << id;
    updateSessionTiming(
      mSessions.sawSessionTimeline(std::move(id), std::move(timeline)), mGhostXForm);
  }

  void joinSession(const Session& session)
  {
    const bool sessionIdChanged = mSessionId != session.sessionId;
    mSessionId = session.sessionId;
    updateSessionTiming(session.timeline, session.measurement.xform);

    if (sessionIdChanged)
    {
      debug(mIo->log()) << "Joining session " << session.sessionId << " with tempo "
                        << session.timeline.tempo.bpm();
      mSessionPeerCounter();
    }
  }

  void resetState()
  {
    mNodeId = NodeId::random();
    mSessionId = mNodeId;

    const auto xform = detail::initXForm(mClock);
    const auto hostTime = -xform.intercept;
    // When creating the new timeline, make it continuous by finding
    // the beat on the old session timeline corresponding to the
    // current host time and mapping it to the new ghost time
    // representation of the current host time.
    const auto newTl = Timeline{mSessionTimeline.tempo,
      mSessionTimeline.toBeats(mGhostXForm.hostToGhost(hostTime)),
      xform.hostToGhost(hostTime)};

    updateSessionTiming(newTl, xform);

    mSessions.resetSession({mNodeId, newTl, {xform, hostTime}});
    mPeers.resetPeers();
  }

  struct SessionTimelineCallback
  {
    void operator()(SessionId id, Timeline timeline)
    {
      mController.handleTimelineFromSession(std::move(id), std::move(timeline));
    }

    Controller& mController;
  };

  struct SessionPeerCounter
  {
    SessionPeerCounter(Controller& controller, PeerCountCallback callback)
      : mController(controller)
      , mCallback(std::move(callback))
      , mSessionPeerCount(0)
    {
    }

    void operator()()
    {
      const auto count =
        mController.mPeers.uniqueSessionPeerCount(mController.mSessionId);
      const auto oldCount = mSessionPeerCount.exchange(count);
      if (oldCount != count)
      {
        if (count == 0)
        {
          // When the count goes down to zero, completely reset the
          // state, effectively founding a new session
          mController.resetState();
        }
        mCallback(count);
      }
    }

    Controller& mController;
    PeerCountCallback mCallback;
    std::atomic<std::size_t> mSessionPeerCount;
  };

  struct MeasurePeer
  {
    template <typename Peer, typename Handler>
    void operator()(Peer peer, Handler handler)
    {
      using It = typename Discovery::ServicePeerGateways::GatewayMap::iterator;
      using ValueType = typename Discovery::ServicePeerGateways::GatewayMap::value_type;
      mController.mDiscovery.withGatewaysAsync([peer, handler](It begin, const It end) {
        const auto addr = peer.second;
        const auto it = std::find_if(
          begin, end, [&addr](const ValueType& vt) { return vt.first == addr; });
        if (it != end)
        {
          it->second->measurePeer(std::move(peer.first), std::move(handler));
        }
        else
        {
          // invoke the handler with an empty result if we couldn't
          // find the peer's gateway
          handler(GhostXForm{});
        }
      });
    }

    Controller& mController;
  };

  struct JoinSessionCallback
  {
    void operator()(Session session)
    {
      mController.joinSession(std::move(session));
    }

    Controller& mController;
  };

  using IoType = typename util::Injected<IoContext>::type;

  using ControllerPeers =
    Peers<IoType&, std::reference_wrapper<SessionPeerCounter>, SessionTimelineCallback>;

  using ControllerGateway =
    Gateway<typename ControllerPeers::GatewayObserver, Clock, IoType&>;
  using GatewayPtr = std::shared_ptr<ControllerGateway>;

  struct GatewayFactory
  {
    GatewayPtr operator()(std::pair<NodeState, GhostXForm> state,
      util::Injected<IoType&> io,
      const asio::ip::address& addr)
    {
      if (addr.is_v4())
      {
        return GatewayPtr{new ControllerGateway{std::move(io), addr.to_v4(),
          util::injectVal(makeGatewayObserver(mController.mPeers, addr)),
          std::move(state.first), std::move(state.second), mController.mClock}};
      }
      else
      {
        throw std::runtime_error("Could not create peer gateway on non-ipV4 address");
      }
    }

    Controller& mController;
  };

  struct UdpSendExceptionHandler
  {
    using Exception = discovery::UdpSendException;

    void operator()(const Exception& exception)
    {
      mController.mDiscovery.repairGateway(exception.interfaceAddr);
    }

    Controller& mController;
  };

  TempoCallback mTempoCallback;
  Clock mClock;
  NodeId mNodeId;
  SessionId mSessionId;

  // Mutex that controls access to mGhostXForm and mSessionTimeline
  mutable std::mutex mSessionTimingGuard;
  GhostXForm mGhostXForm;
  Timeline mSessionTimeline;

  mutable std::mutex mClientTimelineGuard;
  Timeline mClientTimeline;

  mutable Timeline mRtClientTimeline;
  std::chrono::microseconds mRtClientTimelineTimestamp;

  SessionPeerCounter mSessionPeerCounter;

  std::atomic<bool> mEnabled;

  util::Injected<IoContext> mIo;
  // A realtime facade over the provided IoContext. This should only
  // be used by realtime code, non-realtime code should use mIo.
  typename IoType::template RealTimeContext<IoType&> mRealtimeIo;

  ControllerPeers mPeers;

  using ControllerSessions = Sessions<ControllerPeers&,
    MeasurePeer,
    JoinSessionCallback,
    typename util::Injected<IoContext>::type&,
    Clock>;
  ControllerSessions mSessions;

  using Discovery =
    discovery::Service<std::pair<NodeState, GhostXForm>, GatewayFactory, IoContext>;
  Discovery mDiscovery;
};

} // namespace link
} // namespace ableton
