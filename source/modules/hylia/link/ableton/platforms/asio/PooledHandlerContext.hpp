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

#include <ableton/platforms/asio/AsioWrapper.hpp>
#include <ableton/util/Injected.hpp>

namespace ableton
{
namespace platforms
{
namespace asio
{

template <typename IoContext,
  std::size_t MaxNumHandlers = 32,
  std::size_t MaxHandlerSize = 128>
struct PooledHandlerContext
{
  PooledHandlerContext(util::Injected<IoContext> io)
    : mIo(std::move(io))
  {
    // initialize the handler free store
    mFreeStack.reserve(MaxNumHandlers);
    for (std::size_t i = 0; i < MaxNumHandlers; ++i)
    {
      mFreeStack.push_back(reinterpret_cast<void*>(mRaw + i));
    }
  }

  template <typename Handler>
  void async(Handler handler)
  {
    try
    {
      mIo->async(HandlerWrapper<Handler>{*this, std::move(handler)});
    }
    catch (std::bad_alloc)
    {
      warning(mIo->log()) << "Handler dropped due to low memory pool";
    }
  }

  template <typename Handler>
  struct HandlerWrapper
  {
    HandlerWrapper(PooledHandlerContext& context, Handler handler)
      : mContext(context)
      , mHandler(std::move(handler))
    {
    }

    void operator()()
    {
      mHandler();
    }

    // Use pooled allocation so that posting handlers will not cause
    // system allocation
    friend void* asio_handler_allocate(
      const std::size_t size, HandlerWrapper* const pHandler)
    {
      if (size > MaxHandlerSize || pHandler->mContext.mFreeStack.empty())
      {
        // Going over the max handler size is a programming error, as
        // this is not a dynamically variable value.
        assert(size <= MaxHandlerSize);
        throw std::bad_alloc();
      }
      else
      {
        const auto p = pHandler->mContext.mFreeStack.back();
        pHandler->mContext.mFreeStack.pop_back();
        return p;
      }
    }

    friend void asio_handler_deallocate(
      void* const p, std::size_t, HandlerWrapper* const pHandler)
    {
      pHandler->mContext.mFreeStack.push_back(p);
    }

    PooledHandlerContext& mContext;
    Handler mHandler;
  };

  using MemChunk = typename std::aligned_storage<MaxHandlerSize,
    std::alignment_of<max_align_t>::value>::type;
  MemChunk mRaw[MaxNumHandlers];
  std::vector<void*> mFreeStack;

  util::Injected<IoContext> mIo;
};

} // namespace asio
} // namespace platforms
} // namespace ableton
