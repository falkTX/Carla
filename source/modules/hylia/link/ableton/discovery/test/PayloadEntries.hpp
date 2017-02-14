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
#include <cstdint>
#include <utility>

namespace ableton
{
namespace discovery
{
namespace test
{

// Test payload entries

// A fixed-size entry type
struct Foo
{
  enum
  {
    key = '_foo'
  };

  std::int32_t fooVal;

  friend std::uint32_t sizeInByteStream(const Foo& foo)
  {
    // Namespace qualification is needed to avoid ambiguous function definitions
    return discovery::sizeInByteStream(foo.fooVal);
  }

  template <typename It>
  friend It toNetworkByteStream(const Foo& foo, It out)
  {
    return discovery::toNetworkByteStream(foo.fooVal, std::move(out));
  }

  template <typename It>
  static std::pair<Foo, It> fromNetworkByteStream(It begin, It end)
  {
    auto result = Deserialize<decltype(fooVal)>::fromNetworkByteStream(
      std::move(begin), std::move(end));
    return std::make_pair(Foo{std::move(result.first)}, std::move(result.second));
  }
};

// A variable-size entry type
struct Bar
{
  enum
  {
    key = '_bar'
  };

  std::vector<std::uint64_t> barVals;

  friend std::uint32_t sizeInByteStream(const Bar& bar)
  {
    return discovery::sizeInByteStream(bar.barVals);
  }

  template <typename It>
  friend It toNetworkByteStream(const Bar& bar, It out)
  {
    return discovery::toNetworkByteStream(bar.barVals, out);
  }

  template <typename It>
  static std::pair<Bar, It> fromNetworkByteStream(It begin, It end)
  {
    auto result = Deserialize<decltype(barVals)>::fromNetworkByteStream(
      std::move(begin), std::move(end));
    return std::make_pair(Bar{std::move(result.first)}, std::move(result.second));
  }
};

} // namespace test
} // namespace discovery
} // namespace ableton
