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

#include <ableton/platforms/AsioWrapper.hpp>
#include <functional>

namespace ableton
{
namespace test
{
namespace serial_io
{

struct Socket
{
  using SendFn = std::function<std::size_t(
    const uint8_t* const, const size_t, const asio::ip::udp::endpoint&)>;

  struct ReceiveHandler
  {
    template <typename It>
    void operator()(const asio::ip::udp::endpoint& from, const It begin, const It end)
    {
      std::vector<uint8_t> buffer{begin, end};
      mReceive(from, buffer);
    }

    std::function<const asio::ip::udp::endpoint&, const std::vector<uint8_t>&> mReceive;
  };

  using ReceiveFn = std::function<void(ReceiveHandler)>;

  Socket(SendFn sendFn, ReceiveFn receiveFn)
    : mSendFn(std::move(sendFn))
    , mReceiveFn(std::move(receiveFn))
  {
  }

  std::size_t send(
    const uint8_t* const pData, const size_t numBytes, const asio::ip::udp::endpoint& to)
  {
    return mSendFn(pData, numBytes, to);
  }

  template <typename Handler>
  void receive(Handler handler)
  {
    mReceiveFn(ReceiveHandler{
      [handler](const asio::ip::udp::endpoint& from, const std::vector<uint8_t>& buffer) {
        handler(from, begin(buffer), end(buffer));
      }});
  }

  asio::ip::udp::endpoint endpoint() const
  {
    return asio::ip::udp::endpoint({}, 0);
  }

  SendFn mSendFn;
  ReceiveFn mReceiveFn;
};

} // namespace test
} // namespace platforms
} // namespace ableton
