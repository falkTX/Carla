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

#include <array>
#include <atomic>
#include <cassert>

namespace ableton
{
namespace link
{

// Single producer, single consumer lockfree Fifo

template <typename Type, std::size_t size>
class CircularFifo
{
public:
  struct PoppedItem
  {
    Type item;
    bool valid;
  };

  CircularFifo()
    : tail(0)
    , head(0)
  {
    assert(head.is_lock_free() && tail.is_lock_free());
  }

  bool push(Type item)
  {
    const auto currentTail = tail.load();
    const auto nextTail = nextIndex(currentTail);
    if (nextTail != head.load())
    {
      data[currentTail] = std::move(item);
      tail.store(nextTail);
      return true;
    }
    return false;
  }

  PoppedItem pop()
  {
    const auto currentHead = head.load();
    if (currentHead == tail.load())
    {
      return {Type{}, false};
    }

    auto item = data[currentHead];
    head.store(nextIndex(currentHead));
    return {std::move(item), true};
  }

  PoppedItem clearAndPopLast()
  {
    auto hasData = false;
    auto currentHead = head.load();
    while (currentHead != tail.load())
    {
      currentHead = nextIndex(currentHead);
      hasData = true;
    }

    auto item = data[previousIndex(currentHead)];
    head.store(currentHead);
    return {std::move(item), hasData};
  }

  bool isEmpty() const
  {
    return tail == head;
  }

private:
  size_t nextIndex(const size_t index) const
  {
    return (index + 1) % (size + 1);
  }

  size_t previousIndex(const size_t index) const
  {
    return (index + size) % (size + 1);
  }

  std::atomic_size_t tail;
  std::atomic_size_t head;
  std::array<Type, size + 1> data;
};

} // link
} // ableton
