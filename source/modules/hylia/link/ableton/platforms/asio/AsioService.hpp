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

#include <ableton/platforms/asio/AsioTimer.hpp>
#include <ableton/platforms/asio/AsioWrapper.hpp>
#include <thread>

namespace ableton
{
namespace platforms
{
namespace asio
{

class AsioService
{
public:
  using Timer = AsioTimer;

  AsioService()
    : AsioService(DefaultHandler{})
  {
  }

  template <typename ExceptionHandler>
  explicit AsioService(ExceptionHandler exceptHandler)
    : mpWork(new ::asio::io_service::work(mService))
  {
    mThread =
      std::thread{[](::asio::io_service& service, ExceptionHandler handler) {
                    for (;;)
                    {
                      try
                      {
                        service.run();
                        break;
                      }
                      catch (const typename ExceptionHandler::Exception& exception)
                      {
                        handler(exception);
                      }
                    }
                  },
        std::ref(mService), std::move(exceptHandler)};
  }

  ~AsioService()
  {
    mpWork.reset();
    mThread.join();
  }

  AsioTimer makeTimer()
  {
    return {mService};
  }

  template <typename Handler>
  void post(Handler handler)
  {
    mService.post(std::move(handler));
  }

  ::asio::io_service mService;

private:
  // Default handler is hidden and defines a hidden exception type
  // that will never be thrown by other code, so it effectively does
  // not catch.
  struct DefaultHandler
  {
    struct Exception
    {
    };

    void operator()(const Exception&)
    {
    }
  };

  std::unique_ptr<::asio::io_service::work> mpWork;
  std::thread mThread;
};

} // namespace asio
} // namespace platforms
} // namespace ableton
