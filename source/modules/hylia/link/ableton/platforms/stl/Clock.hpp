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

#include <chrono>

namespace ableton
{
namespace platforms
{
namespace stl
{

struct Clock
{
  using Ticks = std::uint64_t;

  Clock()
  {
    mStartTime = std::chrono::high_resolution_clock::now();
  }

  std::chrono::microseconds micros() const
  {
    using namespace std::chrono;
    return duration_cast<microseconds>(high_resolution_clock::now() - mStartTime);
  }

  std::chrono::high_resolution_clock::time_point mStartTime;
};

} // namespace stl
} // namespace platforms
} // namespace ableton
