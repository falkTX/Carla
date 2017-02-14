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

#include <ableton/discovery/NetworkByteStreamSerializable.hpp>
#include <cmath>
#include <cstdint>
#include <tuple>

namespace ableton
{
namespace link
{

struct Beats : std::tuple<std::int64_t>
{
  Beats() = default;

  explicit Beats(const double beats)
    : std::tuple<std::int64_t>(llround(beats * 1e6))
  {
  }

  explicit Beats(const std::int64_t microBeats)
    : std::tuple<std::int64_t>(microBeats)
  {
  }

  double floating() const
  {
    return microBeats() / 1e6;
  }

  std::int64_t microBeats() const
  {
    return std::get<0>(*this);
  }

  Beats operator-() const
  {
    return Beats{-microBeats()};
  }

  friend Beats abs(const Beats b)
  {
    return Beats{std::abs(b.microBeats())};
  }

  friend Beats operator+(const Beats lhs, const Beats rhs)
  {
    return Beats{lhs.microBeats() + rhs.microBeats()};
  }

  friend Beats operator-(const Beats lhs, const Beats rhs)
  {
    return Beats{lhs.microBeats() - rhs.microBeats()};
  }

  friend Beats operator%(const Beats lhs, const Beats rhs)
  {
    return rhs == Beats{0.} ? Beats{0.} : Beats{lhs.microBeats() % rhs.microBeats()};
  }

  // Model the NetworkByteStreamSerializable concept
  friend std::uint32_t sizeInByteStream(const Beats beats)
  {
    return discovery::sizeInByteStream(beats.microBeats());
  }

  template <typename It>
  friend It toNetworkByteStream(const Beats beats, It out)
  {
    return discovery::toNetworkByteStream(beats.microBeats(), std::move(out));
  }

  template <typename It>
  static std::pair<Beats, It> fromNetworkByteStream(It begin, It end)
  {
    auto result = discovery::Deserialize<std::int64_t>::fromNetworkByteStream(
      std::move(begin), std::move(end));
    return std::make_pair(Beats{result.first}, std::move(result.second));
  }
};

} // namespace link
} // namespace ableton
