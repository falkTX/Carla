/**
* @file mingw.thread.h
* @brief std::thread implementation for MinGW
* (c) 2013-2016 by Mega Limited, Auckland, New Zealand
* @author Alexander Vassilev
*
* @copyright Simplified (2-clause) BSD License.
* You should have received a copy of the license along with this
* program.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* @note
* This file may become part of the mingw-w64 runtime package. If/when this happens,
* the appropriate license will be added, i.e. this code will become dual-licensed,
* and the current BSD 2-clause license will stay.
*/

#ifndef WIN32STDTHREAD_H
#define WIN32STDTHREAD_H

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error A C++11 compiler is required!
#endif

//  Use the standard classes for std::, if available.
#include <thread>

#include <cstddef>      //  For std::size_t
#include <cerrno>       //  Detect error type.
#include <exception>    //  For std::terminate
#include <system_error> //  For std::system_error
#include <functional>   //  For std::hash
#include <tuple>        //  For std::tuple
#include <chrono>       //  For sleep timing.
#include <memory>       //  For std::unique_ptr
#include <iosfwd>       //  Stream output for thread ids.
#include <utility>      //  For std::swap, std::forward

#include "mingw.invoke.h"

#include <synchapi.h>   //  For WaitForSingleObject
#include <handleapi.h>  //  For CloseHandle, etc.
#include <sysinfoapi.h> //  For GetNativeSystemInfo
#include <processthreadsapi.h>  //  For GetCurrentThreadId
#include <process.h>  //  For _beginthreadex

#ifndef NDEBUG
#include <cstdio>
#endif

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
#error To use the MinGW-std-threads library, you will need to define the macro _WIN32_WINNT to be 0x0501 (Windows XP) or higher.
#endif

//  Instead of INVALID_HANDLE_VALUE, _beginthreadex returns 0.
namespace mingw_stdthread
{
namespace detail
{
    template<std::size_t...>
    struct IntSeq {};

    template<std::size_t N, std::size_t... S>
    struct GenIntSeq : GenIntSeq<N-1, N-1, S...> { };

    template<std::size_t... S>
    struct GenIntSeq<0, S...> { typedef IntSeq<S...> type; };

    // We can't define the Call struct in the function - the standard forbids template methods in that case
    template<class Func, typename... Args>
    class ThreadFuncCall
    {
        using Tuple = std::tuple<typename std::decay<Args>::type...>;
        typename std::decay<Func>::type mFunc;
        Tuple mArgs;

        template <std::size_t... S>
        void callFunc(detail::IntSeq<S...>)
        {
//  Note: Only called once (per thread)
            detail::invoke(std::move(mFunc), std::move(std::get<S>(mArgs)) ...);
        }
    public:
        ThreadFuncCall(Func&& aFunc, Args&&... aArgs)
          : mFunc(std::forward<Func>(aFunc)),
            mArgs(std::forward<Args>(aArgs)...)
        {
        }

        void callFunc()
        {
            callFunc(typename detail::GenIntSeq<sizeof...(Args)>::type());
        }
    };

} //  Namespace "detail"

class thread
{
public:
    class id
    {
        DWORD mId;
        void clear() {mId = 0;}
        friend class thread;
        friend class std::hash<id>;
    public:
        explicit id(DWORD aId=0) noexcept : mId(aId){}
        friend bool operator==(id x, id y) noexcept {return x.mId == y.mId; }
        friend bool operator!=(id x, id y) noexcept {return x.mId != y.mId; }
        friend bool operator< (id x, id y) noexcept {return x.mId <  y.mId; }
        friend bool operator<=(id x, id y) noexcept {return x.mId <= y.mId; }
        friend bool operator> (id x, id y) noexcept {return x.mId >  y.mId; }
        friend bool operator>=(id x, id y) noexcept {return x.mId >= y.mId; }

        template<class _CharT, class _Traits>
        friend std::basic_ostream<_CharT, _Traits>&
        operator<<(std::basic_ostream<_CharT, _Traits>& __out, id __id)
        {
            if (__id.mId == 0)
            {
                return __out << "(invalid std::thread::id)";
            }
            else
            {
                return __out << __id.mId;
            }
        }
    };
private:
    static constexpr HANDLE kInvalidHandle = nullptr;
    static constexpr DWORD kInfinite = 0xffffffffl;
    HANDLE mHandle;
    id mThreadId;

    template <class Call>
    static unsigned __stdcall threadfunc(void* arg)
    {
        std::unique_ptr<Call> call(static_cast<Call*>(arg));
        call->callFunc();
        return 0;
    }

    static unsigned int _hardware_concurrency_helper() noexcept
    {
        SYSTEM_INFO sysinfo;
//    This is one of the few functions used by the library which has a nearly-
//  equivalent function defined in earlier versions of Windows. Include the
//  workaround, just as a reminder that it does exist.
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0501)
        ::GetNativeSystemInfo(&sysinfo);
#else
        ::GetSystemInfo(&sysinfo);
#endif
        return sysinfo.dwNumberOfProcessors;
    }
public:
    typedef HANDLE native_handle_type;
    id get_id() const noexcept {return mThreadId;}
    native_handle_type native_handle() const {return mHandle;}
    thread(): mHandle(kInvalidHandle), mThreadId(){}

    thread(thread&& other)
    :mHandle(other.mHandle), mThreadId(other.mThreadId)
    {
        other.mHandle = kInvalidHandle;
        other.mThreadId.clear();
    }

    thread(const thread &other)=delete;

    template<class Func, typename... Args>
    explicit thread(Func&& func, Args&&... args) : mHandle(), mThreadId()
    {
        typedef detail::ThreadFuncCall<Func, Args...> Call;
        auto call = new Call(
            std::forward<Func>(func), std::forward<Args>(args)...);
        auto int_handle = _beginthreadex(NULL, 0, threadfunc<Call>,
            static_cast<LPVOID>(call), 0,
            reinterpret_cast<unsigned*>(&(mThreadId.mId)));
        if (int_handle == 0)
        {
            mHandle = kInvalidHandle;
            int errnum = errno;
            delete call;
//  Note: Should only throw EINVAL, EAGAIN, EACCES
            throw std::system_error(errnum, std::generic_category());
        } else
            mHandle = reinterpret_cast<HANDLE>(int_handle);
    }

    bool joinable() const {return mHandle != kInvalidHandle;}

//    Note: Due to lack of synchronization, this function has a race condition
//  if called concurrently, which leads to undefined behavior. The same applies
//  to all other member functions of this class, but this one is mentioned
//  explicitly.
    void join()
    {
        using namespace std;
        if (get_id() == id(GetCurrentThreadId()))
            throw system_error(make_error_code(errc::resource_deadlock_would_occur));
        if (mHandle == kInvalidHandle)
            throw system_error(make_error_code(errc::no_such_process));
        if (!joinable())
            throw system_error(make_error_code(errc::invalid_argument));
        WaitForSingleObject(mHandle, kInfinite);
        CloseHandle(mHandle);
        mHandle = kInvalidHandle;
        mThreadId.clear();
    }

    ~thread()
    {
        if (joinable())
        {
#ifndef NDEBUG
            std::printf("Error: Must join() or detach() a thread before \
destroying it.\n");
#endif
            std::terminate();
        }
    }
    thread& operator=(const thread&) = delete;
    thread& operator=(thread&& other) noexcept
    {
        if (joinable())
        {
#ifndef NDEBUG
            std::printf("Error: Must join() or detach() a thread before \
moving another thread to it.\n");
#endif
            std::terminate();
        }
        swap(std::forward<thread>(other));
        return *this;
    }
    void swap(thread&& other) noexcept
    {
        std::swap(mHandle, other.mHandle);
        std::swap(mThreadId.mId, other.mThreadId.mId);
    }

    static unsigned int hardware_concurrency() noexcept
    {
        static unsigned int cached = _hardware_concurrency_helper();
        return cached;
    }

    void detach()
    {
        if (!joinable())
        {
            using namespace std;
            throw system_error(make_error_code(errc::invalid_argument));
        }
        if (mHandle != kInvalidHandle)
        {
            CloseHandle(mHandle);
            mHandle = kInvalidHandle;
        }
        mThreadId.clear();
    }
};

namespace this_thread
{
    inline thread::id get_id() noexcept {return thread::id(GetCurrentThreadId());}
    inline void yield() noexcept {Sleep(0);}
    template< class Rep, class Period >
    void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration)
    {
        static constexpr DWORD kInfinite = 0xffffffffl;
        using namespace std::chrono;
        using rep = milliseconds::rep;
        rep ms = duration_cast<milliseconds>(sleep_duration).count();
        while (ms > 0)
        {
            constexpr rep kMaxRep = static_cast<rep>(kInfinite - 1);
            auto sleepTime = (ms < kMaxRep) ? ms : kMaxRep;
            Sleep(static_cast<DWORD>(sleepTime));
            ms -= sleepTime;
        }
    }
    template <class Clock, class Duration>
    void sleep_until(const std::chrono::time_point<Clock,Duration>& sleep_time)
    {
        sleep_for(sleep_time-Clock::now());
    }
}
} //  Namespace mingw_stdthread

namespace std
{
//    Because of quirks of the compiler, the common "using namespace std;"
//  directive would flatten the namespaces and introduce ambiguity where there
//  was none. Direct specification (std::), however, would be unaffected.
//    Take the safe option, and include only in the presence of MinGW's win32
//  implementation.
#if defined(__MINGW32__ ) && !defined(_GLIBCXX_HAS_GTHREADS)
using mingw_stdthread::thread;
//    Remove ambiguity immediately, to avoid problems arising from the above.
//using std::thread;
namespace this_thread
{
using namespace mingw_stdthread::this_thread;
}
#elif !defined(MINGW_STDTHREAD_REDUNDANCY_WARNING)  //  Skip repetition
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#pragma message "This version of MinGW seems to include a win32 port of\
 pthreads, and probably already has C++11 std threading classes implemented,\
 based on pthreads. These classes, found in namespace std, are not overridden\
 by the mingw-std-thread library. If you would still like to use this\
 implementation (as it is more lightweight), use the classes provided in\
 namespace mingw_stdthread."
#endif

//    Specialize hash for this implementation's thread::id, even if the
//  std::thread::id already has a hash.
template<>
struct hash<mingw_stdthread::thread::id>
{
    typedef mingw_stdthread::thread::id argument_type;
    typedef size_t result_type;
    size_t operator() (const argument_type & i) const noexcept
    {
        return i.mId;
    }
};
}
#endif // WIN32STDTHREAD_H
