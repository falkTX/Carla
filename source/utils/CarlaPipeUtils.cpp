// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPipeUtils.hpp"
#include "CarlaProcessUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaMIDI.h"

#include "extra/Sleep.hpp"
#include "extra/String.hpp"
#include "extra/Time.hpp"

// needed for atom-util
#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

#ifdef BUILDING_CARLA
# include "lv2/atom-util.h"
#else
# include "lv2/atom/util.h"
#endif

#include <fcntl.h>

#ifdef CARLA_OS_WIN
# include "water/text/String.h"
# include <ctime>
#else
# include <cerrno>
# include <signal.h>
# include <sys/wait.h>
# ifdef CARLA_OS_LINUX
#  include <sys/prctl.h>
#  ifndef F_SETPIPE_SZ
#   define F_SETPIPE_SZ 1031
#  endif
# endif
#endif

#ifdef CARLA_OS_WIN
# define INVALID_PIPE_VALUE INVALID_HANDLE_VALUE
#else
# define INVALID_PIPE_VALUE -1
#endif

#ifdef CARLA_OS_WIN
// -----------------------------------------------------------------------
// win32 stuff

static inline
bool waitForAsyncObject(const HANDLE object, const HANDLE process = INVALID_HANDLE_VALUE)
{
    DWORD dw, dw2;
    MSG msg;

    // we give it a max
    for (int i=20000; --i>=0;)
    {
        if (process != INVALID_HANDLE_VALUE)
        {
            switch (::WaitForSingleObject(process, 0))
            {
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                carla_stderr("waitForAsyncObject process has stopped");
                return false;
            }
        }

        carla_debug("waitForAsyncObject loop start");
        dw = ::MsgWaitForMultipleObjectsEx(1, &object, INFINITE, QS_POSTMESSAGE|QS_TIMER, 0);
        carla_debug("waitForAsyncObject initial code is: %u", dw);

        if (dw == WAIT_OBJECT_0)
        {
            carla_debug("waitForAsyncObject WAIT_OBJECT_0");
            return true;
        }

        dw2 = ::GetLastError();

        if (dw == WAIT_OBJECT_0 + 1)
        {
            carla_debug("waitForAsyncObject loop +1");

            while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                ::DispatchMessage(&msg);

            continue;
        }

        if (dw2 == 0)
        {
            carla_debug("waitForAsyncObject loop stop");
            return true;
        }

        carla_stderr2("waitForAsyncObject loop end reached, error was: %u", dw2);
        d_msleep(5);
    }

    carla_stderr2("waitForAsyncObject reached the end, this should not happen");
    return false;
}

static inline
ssize_t ReadFileWin32(const HANDLE pipeh, const HANDLE event, void* const buf, const DWORD numBytes)
{
    DWORD dw, dsize = numBytes;
    DWORD available = 0;

    if (::PeekNamedPipe(pipeh, nullptr, 0, nullptr, &available, nullptr) == FALSE || available == 0)
        return -1;

    OVERLAPPED ov;
    carla_zeroStruct(ov);
    ov.hEvent = event;

    if (::ReadFile(pipeh, buf, dsize, nullptr, &ov))
    {
        if (! ::GetOverlappedResult(pipeh, &ov, &dw, FALSE))
        {
            carla_stderr("ReadFileWin32 GetOverlappedResult failed, error was: %u", ::GetLastError());
            return -1;
        }

        return static_cast<ssize_t>(dsize);
    }

    dw = ::GetLastError();

    if (dw == ERROR_IO_PENDING)
    {
        if (! waitForAsyncObject(event))
        {
            carla_stderr("ReadFileWin32 waitForAsyncObject failed, error was: %u", ::GetLastError());
            return -1;
        }

        if (! ::GetOverlappedResult(pipeh, &ov, &dw, FALSE))
        {
            carla_stderr("ReadFileWin32 GetOverlappedResult of pending failed, error was: %u", ::GetLastError());
            return -1;
        }

        return static_cast<ssize_t>(dsize);
    }

    carla_stderr("ReadFileWin32 failed, error was: %u", dw);
    return -1;
}

static inline
ssize_t WriteFileWin32(const HANDLE pipeh, const HANDLE event, const void* const buf, const DWORD numBytes)
{
    DWORD dw, dsize = numBytes;

    OVERLAPPED ov;
    carla_zeroStruct(ov);
    ov.hEvent = event;

    if (::WriteFile(pipeh, buf, dsize, nullptr, &ov))
    {
        if (! ::GetOverlappedResult(pipeh, &ov, &dw, FALSE))
        {
            carla_stderr("WriteFileWin32 GetOverlappedResult failed, error was: %u", ::GetLastError());
            return -1;
        }

        return static_cast<ssize_t>(dsize);
    }

    dw = ::GetLastError();

    if (dw == ERROR_IO_PENDING)
    {
        if (! waitForAsyncObject(event))
        {
            carla_stderr("WriteFileWin32 waitForAsyncObject failed, error was: %u", ::GetLastError());
            return -1;
        }

        if (! ::GetOverlappedResult(pipeh, &ov, &dw, FALSE))
        {
            carla_stderr("WriteFileWin32 GetOverlappedResult of pending failed, error was: %u", ::GetLastError());
            return -1;
        }

        return static_cast<ssize_t>(dsize);
    }

    if (dw == ERROR_PIPE_NOT_CONNECTED)
    {
        carla_stdout("WriteFileWin32 failed, client has closed");
        return -2;
    }

    carla_stderr("WriteFileWin32 failed, error was: %u", dw);
    return -1;
}
#endif // CARLA_OS_WIN

// -----------------------------------------------------------------------
// startProcess

#ifdef CARLA_OS_WIN
static inline
bool startProcess(const char* const argv[], PROCESS_INFORMATION* const processInfo)
{
    CARLA_SAFE_ASSERT_RETURN(processInfo != nullptr, false);

    using water::String;

    String command;

    for (int i=0; argv[i] != nullptr; ++i)
    {
        String arg(argv[i]);

        // If there are spaces, surround it with quotes. If there are quotes,
        // replace them with \" so that CommandLineToArgv will correctly parse them.
        if (arg.containsAnyOf("\" "))
            arg = arg.replace("\"", "\\\"").quoted();

        command << arg << ' ';
    }

    command = command.trim();

    STARTUPINFOA startupInfo;
    carla_zeroStruct(startupInfo);
    startupInfo.cb = sizeof(startupInfo);

    if (::CreateProcessA(nullptr, const_cast<LPSTR>(command.toRawUTF8()),
                         nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                         nullptr, nullptr, &startupInfo, processInfo) != FALSE)
        return true;

    carla_stderr2("startProcess failed, error was: %u", ::GetLastError());
    return false;
}

static inline
bool waitForClientConnect(const HANDLE pipe, const HANDLE event, const HANDLE process, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pipe != INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    DWORD dw;

    OVERLAPPED ov;
    carla_zeroStruct(ov);
    ov.hEvent = event;

    const uint32_t timeoutEnd = d_gettime_ms() + timeOutMilliseconds;

    for (;;)
    {
        if (::ConnectNamedPipe(pipe, &ov))
        {
            if (! ::GetOverlappedResult(pipe, &ov, &dw, FALSE))
            {
                carla_stderr2("ConnectNamedPipe GetOverlappedResult failed, error was: %u", ::GetLastError());
                return false;
            }

            return true;
        }

        const DWORD err = ::GetLastError();

        switch (err)
        {
        case ERROR_PIPE_CONNECTED:
            return true;

        case ERROR_IO_PENDING:
            if (! waitForAsyncObject(event, process))
            {
                carla_stderr2("ConnectNamedPipe waitForAsyncObject failed, error was: %u", ::GetLastError());
                return false;
            }

            if (! ::GetOverlappedResult(pipe, &ov, &dw, FALSE))
            {
                carla_stderr2("ConnectNamedPipe GetOverlappedResult of pending failed, error was: %u", ::GetLastError());
                return false;
            }

            return true;

        case ERROR_PIPE_LISTENING:
            if (d_gettime_ms() < timeoutEnd)
            {
                d_msleep(5);
                continue;
            }
            carla_stderr2("ConnectNamedPipe listening timed out");
            return false;

        default:
            carla_stderr2("ConnectNamedPipe failed, error was: %u", err);
            return false;
        }
    }

    return true;
}
#else
static inline
bool startProcess(const char* const argv[], pid_t& pidinst) noexcept
{
    const CarlaScopedEnvVar sev1("LD_LIBRARY_PATH", nullptr);
    const CarlaScopedEnvVar sev2("LD_PRELOAD", nullptr);

  #ifdef __clang__
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wdeprecated-declarations"
  #endif
    const pid_t ret = pidinst = vfork();
  #ifdef __clang__
   #pragma clang diagnostic pop
  #endif

    switch (ret)
    {
    case 0: { // child process
        execvp(argv[0], const_cast<char* const*>(argv));

        String error(std::strerror(errno));
        carla_stderr2("exec failed: %s", error.buffer());

        _exit(1); // this is not noexcept safe but doesn't matter anyway
    }   break;

    case -1: { // error
        String error(std::strerror(errno));
        carla_stderr2("vfork() failed: %s", error.buffer());
    }   break;
    }

    return (ret > 0);
}
#endif

// -----------------------------------------------------------------------
// waitForClientFirstMessage

template<typename P>
static inline
bool waitForClientFirstMessage(const P& pipe, void* const ovRecv, void* const process, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pipe != INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    char c;
    ssize_t ret;
    const uint32_t timeoutEnd = d_gettime_ms() + timeOutMilliseconds;

#ifdef CARLA_OS_WIN
    if (! waitForClientConnect(pipe, (HANDLE)ovRecv, (HANDLE)process, timeOutMilliseconds))
        return false;
#endif

    for (;;)
    {
        try {
#ifdef CARLA_OS_WIN
            ret = ReadFileWin32(pipe, (HANDLE)ovRecv, &c, 1);
#else
            ret = ::read(pipe, &c, 1);
#endif
        } CARLA_SAFE_EXCEPTION_RETURN("read pipe", false);

        switch (ret)
        {
        case 1:
            if (c == '\n')
                return true;

            carla_stderr("waitForClientFirstMessage() - read has wrong first char '%c'", c);return false;
            return false;

        case -1: // failed to read
#ifdef CARLA_OS_WIN
            if (::GetLastError() == ERROR_NO_DATA)
#else
            if (errno == EAGAIN)
#endif
            {
                if (d_gettime_ms() < timeoutEnd)
                {
                    d_msleep(5);
                    continue;
                }
                carla_stderr("waitForClientFirstMessage() - read timed out");
            }
            else
            {
#ifdef CARLA_OS_WIN
                carla_stderr("waitForClientFirstMessage() - read failed");
#else
                String error(std::strerror(errno));
                carla_stderr("waitForClientFirstMessage() - read failed: %s", error.buffer());
#endif
            }
            return false;

        default: // ???
            carla_stderr("waitForClientFirstMessage() - read returned %i", int(ret));
            return false;
        }
    }

    // maybe unused
    (void)ovRecv; (void)process;
}

// -----------------------------------------------------------------------
// waitForChildToStop / waitForProcessToStop

#ifdef CARLA_OS_WIN
static inline
bool waitForProcessToStop(const HANDLE process, const uint32_t timeOutMilliseconds, bool sendTerminate) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(process != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    const uint32_t timeoutEnd = d_gettime_ms() + timeOutMilliseconds;

    for (;;)
    {
        switch (::WaitForSingleObject(process, 0))
        {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return true;
        }

        if (sendTerminate)
        {
            sendTerminate = false;
            ::TerminateProcess(process, 15);
        }

        if (d_gettime_ms() >= timeoutEnd)
            break;

        d_msleep(5);
    }

    return false;
}

static inline
void waitForProcessToStopOrKillIt(const HANDLE process, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(process != INVALID_HANDLE_VALUE,);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0,);

    if (! waitForProcessToStop(process, timeOutMilliseconds, true))
    {
        carla_stderr("waitForProcessToStopOrKillIt() - process didn't stop, force termination");

        if (::TerminateProcess(process, 9) != FALSE)
        {
            // wait for process to stop
            waitForProcessToStop(process, timeOutMilliseconds, false);
        }
    }
}
#else
static inline
bool waitForChildToStop(const pid_t pid, const uint32_t timeOutMilliseconds, bool sendTerminate) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pid > 0, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    pid_t ret;
    const uint32_t timeoutEnd = d_gettime_ms() + timeOutMilliseconds;

    for (;;)
    {
        try {
            ret = ::waitpid(pid, nullptr, WNOHANG);
        } CARLA_SAFE_EXCEPTION_BREAK("waitpid");

        switch (ret)
        {
        case -1:
            if (errno == ECHILD)
            {
                // success, child doesn't exist
                return true;
            }
            else
            {
                String error(std::strerror(errno));
                carla_stderr("waitForChildToStop() - waitpid failed: %s", error.buffer());
                return false;
            }
            break;

        case 0:
            if (sendTerminate)
            {
                sendTerminate = false;
                ::kill(pid, SIGTERM);
            }
            if (d_gettime_ms() < timeoutEnd)
            {
                d_msleep(5);
                continue;
            }
            carla_stderr("waitForChildToStop() - timed out");
            break;

        default:
            if (ret == pid)
            {
                // success
                return true;
            }
            else
            {
                carla_stderr("waitForChildToStop() - got wrong pid %i (requested was %i)", int(ret), int(pid));
                return false;
            }
        }

        break;
    }

    return false;
}

static inline
void waitForChildToStopOrKillIt(pid_t& pid, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pid > 0,);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0,);

    if (! waitForChildToStop(pid, timeOutMilliseconds, true))
    {
        carla_stderr("waitForChildToStopOrKillIt() - process didn't stop, force killing");

        if (::kill(pid, SIGKILL) != -1)
        {
            // wait for killing to take place
            waitForChildToStop(pid, timeOutMilliseconds, false);
        }
        else
        {
            String error(std::strerror(errno));
            carla_stderr("waitForChildToStopOrKillIt() - kill failed: %s", error.buffer());
        }
    }
}
#endif

// -----------------------------------------------------------------------

struct CarlaPipeCommon::PrivateData {
    // pipes
#ifdef CARLA_OS_WIN
    PROCESS_INFORMATION processInfo;
    HANDLE pipeRecv;
    HANDLE pipeSend;
    HANDLE ovRecv;
    HANDLE ovSend;
#else
    pid_t pid;
    int pipeRecv;
    int pipeSend;
#endif

    // read functions must only be called in context of idlePipe()
    bool isReading;

    // the client side is closing down, only waiting for response from server
    bool clientClosingDown;

    // other side of pipe has closed
    bool pipeClosed;

    // print error only once
    bool lastMessageFailed;

    // for debugging
    bool isServer;

    // common write lock
    CarlaMutex writeLock;

    // temporary buffers for _readline()
    mutable char   tmpBuf[0xffff];
    mutable String tmpStr;

    PrivateData() noexcept
#ifdef CARLA_OS_WIN
        : processInfo(),
#else
        : pid(-1),
#endif
          pipeRecv(INVALID_PIPE_VALUE),
          pipeSend(INVALID_PIPE_VALUE),
          isReading(false),
          clientClosingDown(false),
          pipeClosed(true),
          lastMessageFailed(false),
          isServer(false),
          writeLock(),
          tmpBuf(),
          tmpStr()
    {
#ifdef CARLA_OS_WIN
        carla_zeroStruct(processInfo);
        processInfo.hProcess = INVALID_HANDLE_VALUE;
        processInfo.hThread  = INVALID_HANDLE_VALUE;

        ovRecv = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ovSend = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
#endif

        carla_zeroChars(tmpBuf, 0xffff);
    }

    CARLA_DECLARE_NON_COPYABLE(PrivateData)
};

// -----------------------------------------------------------------------

CarlaPipeCommon::CarlaPipeCommon() noexcept
    : pData(new PrivateData())
{
    carla_debug("CarlaPipeCommon::CarlaPipeCommon()");
}

CarlaPipeCommon::~CarlaPipeCommon() /*noexcept*/
{
    carla_debug("CarlaPipeCommon::~CarlaPipeCommon()");

    delete pData;
}

// -------------------------------------------------------------------

bool CarlaPipeCommon::isPipeRunning() const noexcept
{
    return (pData->pipeRecv != INVALID_PIPE_VALUE && pData->pipeSend != INVALID_PIPE_VALUE && ! pData->pipeClosed);
}

void CarlaPipeCommon::idlePipe(const bool onlyOnce) noexcept
{
    bool readSucess;

    for (;;)
    {
        readSucess = false;
        const char* const msg = _readline(true, 0, readSucess);

        if (! readSucess)
            break;
        if (msg == nullptr)
            continue;

        pData->isReading = true;

        if (std::strcmp(msg, "__carla-quit__") == 0)
        {
            pData->pipeClosed = true;
        }
        else if (! pData->clientClosingDown)
        {
            try {
                msgReceived(msg);
            } CARLA_SAFE_EXCEPTION("msgReceived");
        }

        pData->isReading = false;

        std::free(const_cast<char*>(msg));

        if (onlyOnce || pData->pipeRecv == INVALID_PIPE_VALUE)
            break;
    }
}

// -------------------------------------------------------------------

void CarlaPipeCommon::lockPipe() const noexcept
{
    pData->writeLock.lock();
}

bool CarlaPipeCommon::tryLockPipe() const noexcept
{
    return pData->writeLock.tryLock();
}

void CarlaPipeCommon::unlockPipe() const noexcept
{
    pData->writeLock.unlock();
}

CarlaMutex& CarlaPipeCommon::getPipeLock() const noexcept
{
    return pData->writeLock;
}

// -------------------------------------------------------------------

bool CarlaPipeCommon::readNextLineAsBool(bool& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        value = (std::strcmp(msg, "true") == 0);
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsByte(uint8_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        const int asint = std::atoi(msg);

        if (asint >= 0 && asint <= 0xFF)
        {
            value = static_cast<uint8_t>(asint);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsInt(int32_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        value = std::atoi(msg);
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsUInt(uint32_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
#if (defined(__WORDSIZE) && __WORDSIZE < 64) || (defined(__SIZE_WIDTH__) && __SIZE_WIDTH__ < 64) || \
      defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
        const long long aslong = std::atoll(msg);
#else
        const long aslong = std::atol(msg);
#endif

        if (aslong >= 0)
        {
            value = static_cast<uint32_t>(aslong);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsLong(int64_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        value = std::atol(msg);
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsULong(uint64_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        const int64_t asint64 = std::atol(msg);

        if (asint64 >= 0)
        {
            value = static_cast<uint64_t>(asint64);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsFloat(float& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        {
            const ScopedSafeLocale ssl;
            value = static_cast<float>(std::atof(msg));
        }
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsDouble(double& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock(false))
    {
        {
            const ScopedSafeLocale ssl;
            value = std::atof(msg);
        }
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsString(const char*& value, const bool allocateString, uint32_t size) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (size >= 0xffff)
        size = 0;

    if (const char* const msg = _readlineblock(allocateString, static_cast<uint16_t>(size)))
    {
        value = msg;
        return true;
    }

    return false;
}

char* CarlaPipeCommon::readNextLineAsString() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, nullptr);

    return const_cast<char*>(_readlineblock(true, 0));
}

// -------------------------------------------------------------------
// must be locked before calling

bool CarlaPipeCommon::writeMessage(const char* const msg) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msg != nullptr && msg[0] != '\0', false);

    if (pData->pipeClosed)
        return false;

    const std::size_t size(std::strlen(msg));
    CARLA_SAFE_ASSERT_RETURN(size > 0, false);
    CARLA_SAFE_ASSERT_RETURN(msg[size-1] == '\n', false);

    return _writeMsgBuffer(msg, size);
}

bool CarlaPipeCommon::writeMessage(const char* const msg, std::size_t size) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msg != nullptr && msg[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(size > 0, false);
    CARLA_SAFE_ASSERT_RETURN(msg[size-1] == '\n', false);

    if (pData->pipeClosed)
        return false;

    return _writeMsgBuffer(msg, size);
}

bool CarlaPipeCommon::writeAndFixMessage(const char* const msg) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msg != nullptr, false);

    if (pData->pipeClosed)
        return false;

    const std::size_t size(std::strlen(msg));

    char* const fixedMsg = static_cast<char*>(std::malloc(size+2));
    CARLA_SAFE_ASSERT_RETURN(fixedMsg != nullptr, false);

    if (size > 0)
    {
        std::strcpy(fixedMsg, msg);

        for (std::size_t i=0; i<size; ++i)
        {
            if (fixedMsg[i] == '\n')
                fixedMsg[i] = '\r';
        }

        if (fixedMsg[size-1] == '\r')
        {
            fixedMsg[size-1] = '\n';
            fixedMsg[size  ] = '\0';
            fixedMsg[size+1] = '\0';
        }
        else
        {
            fixedMsg[size  ] = '\n';
            fixedMsg[size+1] = '\0';
        }
    }
    else
    {
        fixedMsg[0] = '\n';
        fixedMsg[1] = '\0';
    }

    const bool ret = _writeMsgBuffer(fixedMsg, size+1);
    std::free(fixedMsg);
    return ret;
}

bool CarlaPipeCommon::writeEmptyMessage() const noexcept
{
    if (pData->pipeClosed)
        return false;

    return _writeMsgBuffer("\n", 1);
}

bool CarlaPipeCommon::syncMessages() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend != INVALID_PIPE_VALUE, false);

#if defined(CARLA_OS_LINUX) || defined(CARLA_OS_GNU_HURD)
# if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2014
    // the only call that seems to do something
    return ::syncfs(pData->pipeSend) == 0;
# endif
#elif 0 // defined(CARLA_OS_WIN)
    // FIXME causes issues
    try {
        return (::FlushFileBuffers(pData->pipeSend) != FALSE);
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);
#endif

    return true;
}

// -------------------------------------------------------------------

bool CarlaPipeCommon::writeErrorMessage(const char* const error) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(error != nullptr && error[0] != '\0', false);

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("error\n", 6))
        return false;
    if (! writeAndFixMessage(error))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeControlMessage(const uint32_t index, const float value, const bool withWriteLock) const noexcept
{
    if (withWriteLock)
    {
        const CarlaMutexLocker cml(pData->writeLock);
        return writeControlMessage(index, value, false);
    }

    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    if (! _writeMsgBuffer("control\n", 8))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", index);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    {
        const ScopedSafeLocale ssl;
        std::snprintf(tmpBuf, 0xfe, "%.12g\n", static_cast<double>(value));
    }

    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeConfigureMessage(const char* const key, const char* const value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr, false);

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("configure\n", 10))
        return false;
    if (! writeAndFixMessage(key))
        return false;
    if (! writeAndFixMessage(value))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeProgramMessage(const uint32_t index) const noexcept
{
    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("program\n", 8))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", index);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeProgramMessage(const uint8_t channel, const uint32_t bank, const uint32_t program) const noexcept
{
    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("program\n", 8))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", channel);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", bank);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", program);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeMidiProgramMessage(const uint32_t bank, const uint32_t program) const noexcept
{
    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("midiprogram\n", 12))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", bank);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", program);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeReloadProgramsMessage(const int32_t index) const noexcept
{
    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("reloadprograms\n", 15))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", index);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeMidiNoteMessage(const bool onOff, const uint8_t channel, const uint8_t note, const uint8_t velocity) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE, false);
    CARLA_SAFE_ASSERT_RETURN(velocity < MAX_MIDI_VALUE, false);

    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("note\n", 5))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%s\n", bool2str(onOff));
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", channel);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", note);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", velocity);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeLv2AtomMessage(const uint32_t index, const LV2_Atom* const atom) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(atom != nullptr, false);

    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const uint32_t atomTotalSize(lv2_atom_total_size(atom));
    String base64atom(String::asBase64(atom, atomTotalSize));

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("atom\n", 5))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", index);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", atomTotalSize);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%lu\n", static_cast<long unsigned>(base64atom.length()));
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    if (! writeAndFixMessage(base64atom.buffer()))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeLv2ParameterMessage(const char* const uri, const float value, const bool withWriteLock) const noexcept
{
    if (withWriteLock)
    {
        const CarlaMutexLocker cml(pData->writeLock);
        return writeLv2ParameterMessage(uri, value, false);
    }

    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    if (! _writeMsgBuffer("parameter\n", 10))
        return false;

    if (! writeAndFixMessage(uri))
        return false;

    {
        const ScopedSafeLocale ssl;
        std::snprintf(tmpBuf, 0xfe, "%.12g\n", static_cast<double>(value));
    }

    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    syncMessages();
    return true;
}

bool CarlaPipeCommon::writeLv2UridMessage(const uint32_t urid, const char* const uri) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(urid != 0, false);
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', false);

    char tmpBuf[0xff];
    tmpBuf[0xfe] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("urid\n", 5))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%i\n", urid);
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    std::snprintf(tmpBuf, 0xfe, "%lu\n", static_cast<long unsigned>(std::strlen(uri)));
    if (! _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf)))
        return false;

    if (! writeAndFixMessage(uri))
        return false;

    syncMessages();
    return true;
}

// -------------------------------------------------------------------

// internal
const char* CarlaPipeCommon::_readline(const bool allocReturn, const uint16_t size, bool& readSucess) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->pipeRecv != INVALID_PIPE_VALUE, nullptr);

    char    c;
    char*   ptr = pData->tmpBuf;
    ssize_t ret = -1;
    bool tooBig = false;

    pData->tmpStr.clear();

    if (size == 0 || size == 1)
    {
        for (int i=0; i<0xfffe; ++i)
        {
            try {
    #ifdef CARLA_OS_WIN
                ret = ReadFileWin32(pData->pipeRecv, pData->ovRecv, &c, 1);
    #else
                ret = ::read(pData->pipeRecv, &c, 1);
    #endif
            } CARLA_SAFE_EXCEPTION_BREAK("CarlaPipeCommon::readline() - read");

            if (ret != 1)
                break;

            if (c == '\n')
            {
                *ptr = '\0';
                break;
            }

            if (c == '\r')
                c = '\n';

            *ptr++ = c;

            if (i+1 == 0xfffe)
            {
                i = 0;
                *ptr = '\0';
                tooBig = true;
                pData->tmpStr += pData->tmpBuf;
                ptr = pData->tmpBuf;
            }
        }
    }
    else
    {
        uint16_t remaining = size;
        readSucess = false;

        for (;;)
        {
            try {
    #ifdef CARLA_OS_WIN
                ret = ReadFileWin32(pData->pipeRecv, pData->ovRecv, ptr, remaining);
    #else
                ret = ::read(pData->pipeRecv, ptr, remaining);
    #endif
            } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::readline() - read", nullptr);

            if (ret == -1 && errno == EAGAIN)
                continue;

            CARLA_SAFE_ASSERT_INT2_RETURN(ret > 0, ret, remaining, nullptr);
            CARLA_SAFE_ASSERT_INT2_RETURN(ret <= (ssize_t)remaining, ret, remaining, nullptr);

            for (ssize_t i=0; i<ret; ++i)
            {
                if (ptr[i] == '\r')
                    ptr[i] = '\n';
            }

            ptr += ret;
            *ptr = '\0';
            remaining = static_cast<uint16_t>(remaining - ret);

            if (remaining != 0)
                continue;

            readSucess = true;

            if (allocReturn)
            {
                pData->tmpStr = pData->tmpBuf;
                return pData->tmpStr.getAndReleaseBuffer();
            }

            return pData->tmpBuf;
        }
    }

    if (ptr != pData->tmpBuf)
    {
        *ptr = '\0';

        if (! allocReturn && ! tooBig)
        {
            readSucess = true;
            return pData->tmpBuf;
        }

        pData->tmpStr += pData->tmpBuf;
    }
    else if (pData->tmpStr.isEmpty() && ret != 1)
    {
        // some error
        return nullptr;
    }

    readSucess = true;

    if (! allocReturn && ! tooBig)
        return pData->tmpBuf;

    return allocReturn ? pData->tmpStr.getAndReleaseBuffer() : pData->tmpStr.buffer();
}

const char* CarlaPipeCommon::_readlineblock(const bool allocReturn,
                                            const uint16_t size,
                                            const uint32_t timeOutMilliseconds) const noexcept
{
    const uint32_t timeoutEnd = d_gettime_ms() + timeOutMilliseconds;
    bool readSucess;

    for (;;)
    {
        readSucess = false;
        const char* const msg = _readline(allocReturn, size, readSucess);

        if (readSucess)
            return msg;

        if (d_gettime_ms() >= timeoutEnd)
            break;

        d_msleep(5);
    }

    static const bool testingForValgrind = std::getenv("CARLA_VALGRIND_TEST") != nullptr;

    if (testingForValgrind)
    {
        const uint32_t timeoutEnd2 = d_gettime_ms() + 1000;

        for (;;)
        {
            readSucess = false;
            const char* const msg = _readline(allocReturn, size, readSucess);

            if (readSucess)
                return msg;

            if (d_gettime_ms() >= timeoutEnd2)
                break;

            d_msleep(100);
        }
    }

    carla_stderr("readlineblock timed out");
    return nullptr;
}

bool CarlaPipeCommon::_writeMsgBuffer(const char* const msg, const std::size_t size) const noexcept
{
    if (pData->pipeClosed)
        return false;

    if (pData->pipeSend == INVALID_PIPE_VALUE)
    {
        carla_stderr2("CarlaPipe write error, isServer:%s, message was:\n%s", bool2str(pData->isServer), msg);
        return false;
    }

    ssize_t ret;

    try {
       #ifdef CARLA_OS_WIN
        ret = WriteFileWin32(pData->pipeSend, pData->ovSend, msg, static_cast<DWORD>(size));
       #else
        ret = ::write(pData->pipeSend, msg, size);
       #endif
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);

   #ifdef CARLA_OS_WIN
    if (ret == -2)
    {
        pData->pipeClosed = true;
        return false;
    }
   #endif

    if (ret == static_cast<ssize_t>(size))
    {
        if (pData->lastMessageFailed)
            pData->lastMessageFailed = false;
        return true;
    }

    if (! pData->lastMessageFailed)
    {
        pData->lastMessageFailed = true;
        fprintf(stderr,
                "CarlaPipeCommon::_writeMsgBuffer(..., " P_SIZE ") - failed with " P_SSIZE " (%s), message was:\n%s",
                size, ret, bool2str(pData->isServer), msg);
    }

    return false;
}

// -----------------------------------------------------------------------

CarlaPipeServer::CarlaPipeServer() noexcept
    : CarlaPipeCommon()
{
    carla_debug("CarlaPipeServer::CarlaPipeServer()");
    pData->isServer = true;
}

CarlaPipeServer::~CarlaPipeServer() /*noexcept*/
{
    carla_debug("CarlaPipeServer::~CarlaPipeServer()");

    stopPipeServer(5*1000);
}

uintptr_t CarlaPipeServer::getPID() const noexcept
{
#ifndef CARLA_OS_WIN
    return static_cast<uintptr_t>(pData->pid);
#else
    return 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

bool CarlaPipeServer::startPipeServer(const char* const helperTool,
                                      const char* const filename,
                                      const char* const arg1,
                                      const char* const arg2,
                                      const int size,
                                      int timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->pipeRecv == INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend == INVALID_PIPE_VALUE, false);
#ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(pData->processInfo.hThread  == INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pData->processInfo.hProcess == INVALID_HANDLE_VALUE, false);
#else
    CARLA_SAFE_ASSERT_RETURN(pData->pid == -1, false);
#endif
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(arg1 != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(arg2 != nullptr, false);
    carla_debug("CarlaPipeServer::startPipeServer(\"%s\", \"%s\", \"%s\")", filename, arg1, arg2);

    if (timeOutMilliseconds < 0)
        timeOutMilliseconds = 10 * 1000;

    char pipeRecvServerStr[100+1];
    char pipeSendServerStr[100+1];
    char pipeRecvClientStr[100+1];
    char pipeSendClientStr[100+1];

    pipeRecvServerStr[100] = '\0';
    pipeSendServerStr[100] = '\0';
    pipeRecvClientStr[100] = '\0';
    pipeSendClientStr[100] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    //-----------------------------------------------------------------------------------------------------------------
    // create pipes

#ifdef CARLA_OS_WIN
    HANDLE pipe1, pipe2;

    std::srand(static_cast<uint>(std::time(nullptr)));

    static ulong sCounter = 0;
    ++sCounter;

    const int randint = std::rand();

    std::snprintf(pipeRecvServerStr, 100, "\\\\.\\pipe\\carla-pipe1-%i-%li", randint, sCounter);
    std::snprintf(pipeSendServerStr, 100, "\\\\.\\pipe\\carla-pipe2-%i-%li", randint, sCounter);
    std::snprintf(pipeRecvClientStr, 100, "ignored");
    std::snprintf(pipeSendClientStr, 100, "ignored");

    SECURITY_ATTRIBUTES sa;
    carla_zeroStruct(sa);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    pipe1 = ::CreateNamedPipeA(pipeRecvServerStr, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE|PIPE_READMODE_BYTE, 1, size, size, 0, &sa);

    if (pipe1 == INVALID_HANDLE_VALUE)
    {
        fail("pipe creation failed");
        return false;
    }

    pipe2 = ::CreateNamedPipeA(pipeSendServerStr, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE|PIPE_READMODE_BYTE, 1, size, size, 0, &sa);

    if (pipe2 == INVALID_HANDLE_VALUE)
    {
        try { ::CloseHandle(pipe1); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1)");
        fail("pipe creation failed");
        return false;
    }

    const HANDLE pipeRecvClient = pipe2;
    const HANDLE pipeSendClient = pipe1;
#else
    int pipe1[2]; // read by server, written by client
    int pipe2[2]; // read by client, written by server

    if (::pipe(pipe1) != 0)
    {
        fail("pipe1 creation failed");
        return false;
    }

    if (::pipe(pipe2) != 0)
    {
        try { ::close(pipe1[0]); } CARLA_SAFE_EXCEPTION("close(pipe1[0])");
        try { ::close(pipe1[1]); } CARLA_SAFE_EXCEPTION("close(pipe1[1])");
        fail("pipe2 creation failed");
        return false;
    }

    /* */ int pipeRecvServer = pipe1[0];
    /* */ int pipeSendServer = pipe2[1];
    const int pipeRecvClient = pipe2[0];
    const int pipeSendClient = pipe1[1];

    std::snprintf(pipeRecvServerStr, 100, "%i", pipeRecvServer);
    std::snprintf(pipeSendServerStr, 100, "%i", pipeSendServer);
    std::snprintf(pipeRecvClientStr, 100, "%i", pipeRecvClient);
    std::snprintf(pipeSendClientStr, 100, "%i", pipeSendClient);

    //-----------------------------------------------------------------------------------------------------------------
    // set size, non-fatal

# ifdef CARLA_OS_LINUX
    try {
        ::fcntl(pipeRecvClient, F_SETPIPE_SZ, size);
    } CARLA_SAFE_EXCEPTION("Set pipe size");

    try {
        ::fcntl(pipeRecvServer, F_SETPIPE_SZ, size);
    } CARLA_SAFE_EXCEPTION("Set pipe size");
# endif

    //-----------------------------------------------------------------------------------------------------------------
    // set non-block

    int ret;

    try {
        ret = ::fcntl(pipeRecvClient, F_SETFL, ::fcntl(pipeRecvClient, F_GETFL) | O_NONBLOCK);
    } catch (...) {
        ret = -1;
        fail("failed to set pipe as non-block");
    }

    if (ret == 0)
    {
        try {
            ret = ::fcntl(pipeRecvServer, F_SETFL, ::fcntl(pipeRecvServer, F_GETFL) | O_NONBLOCK);
        } catch (...) {
            ret = -1;
            fail("failed to set pipe as non-block");
        }
    }

    if (ret < 0)
    {
        try { ::close(pipe1[0]); } CARLA_SAFE_EXCEPTION("close(pipe1[0])");
        try { ::close(pipe1[1]); } CARLA_SAFE_EXCEPTION("close(pipe1[1])");
        try { ::close(pipe2[0]); } CARLA_SAFE_EXCEPTION("close(pipe2[0])");
        try { ::close(pipe2[1]); } CARLA_SAFE_EXCEPTION("close(pipe2[1])");
        return false;
    }
#endif

    //-----------------------------------------------------------------------------------------------------------------
    // set arguments

    const char* argv[9] = {};
    int i = 0;

    if (helperTool != nullptr)
        argv[i++] = helperTool;

    //-----------------------------------------------------------------------------------------------------------------
    // argv[0] => filename

    argv[i++] = filename;

    //-----------------------------------------------------------------------------------------------------------------
    // argv[1-2] => args

    argv[i++] = arg1;
    argv[i++] = arg2;

    //-----------------------------------------------------------------------------------------------------------------
    // argv[3-6] => pipes

    argv[i++] = pipeRecvServerStr;
    argv[i++] = pipeSendServerStr;
    argv[i++] = pipeRecvClientStr;
    argv[i++] = pipeSendClientStr;

    //-----------------------------------------------------------------------------------------------------------------
    // start process

#ifdef CARLA_OS_WIN
    if (! startProcess(argv, &pData->processInfo))
    {
        carla_zeroStruct(pData->processInfo);
        pData->processInfo.hProcess = INVALID_HANDLE_VALUE;
        pData->processInfo.hThread  = INVALID_HANDLE_VALUE;
        try { ::CloseHandle(pipe1); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1)");
        try { ::CloseHandle(pipe2); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2)");
        return false;
    }

    // just to make sure
    CARLA_SAFE_ASSERT(pData->processInfo.hThread  != INVALID_HANDLE_VALUE);
    CARLA_SAFE_ASSERT(pData->processInfo.hProcess != INVALID_HANDLE_VALUE);
#else
    if (! startProcess(argv, pData->pid))
    {
        pData->pid = -1;
        try { ::close(pipe1[0]); } CARLA_SAFE_EXCEPTION("close(pipe1[0])");
        try { ::close(pipe1[1]); } CARLA_SAFE_EXCEPTION("close(pipe1[1])");
        try { ::close(pipe2[0]); } CARLA_SAFE_EXCEPTION("close(pipe2[0])");
        try { ::close(pipe2[1]); } CARLA_SAFE_EXCEPTION("close(pipe2[1])");
        fail("startProcess() failed");
        return false;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // close duplicated handles used by the client

    try { ::close(pipeRecvServer); } CARLA_SAFE_EXCEPTION("close(pipeRecvServer)");
    try { ::close(pipeSendServer); } CARLA_SAFE_EXCEPTION("close(pipeSendServer)");
#endif

    //-----------------------------------------------------------------------------------------------------------------
    // wait for client to say something

#ifdef CARLA_OS_WIN
    void* const ovRecv  = pData->ovRecv;
    void* const process = pData->processInfo.hProcess;
#else
    void* const ovRecv  = nullptr;
    void* const process = nullptr;
#endif

    if (waitForClientFirstMessage(pipeRecvClient, ovRecv, process, timeOutMilliseconds))
    {
        pData->pipeRecv = pipeRecvClient;
        pData->pipeSend = pipeSendClient;
        pData->pipeClosed = false;
        carla_debug("ALL OK!");
        return true;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // failed to set non-block or get first child message, cannot continue

#ifdef CARLA_OS_WIN
    if (::TerminateProcess(pData->processInfo.hProcess, 9) != FALSE)
    {
        // wait for process to stop
        waitForProcessToStop(pData->processInfo.hProcess, 2*1000, false);
    }

    // clear pData->processInfo
    try { ::CloseHandle(pData->processInfo.hThread);  } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hThread)");
    try { ::CloseHandle(pData->processInfo.hProcess); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hProcess)");
    carla_zeroStruct(pData->processInfo);
    pData->processInfo.hProcess = INVALID_HANDLE_VALUE;
    pData->processInfo.hThread  = INVALID_HANDLE_VALUE;
#else
    if (::kill(pData->pid, SIGKILL) != -1)
    {
        // wait for killing to take place
        waitForChildToStop(pData->pid, 2*1000, false);
    }
    pData->pid = -1;
#endif

    //-----------------------------------------------------------------------------------------------------------------
    // close pipes

#ifdef CARLA_OS_WIN
    try { ::CloseHandle(pipeRecvClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeRecvClient)");
    try { ::CloseHandle(pipeSendClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeSendClient)");
#else
    try { ::close      (pipeRecvClient); } CARLA_SAFE_EXCEPTION("close(pipeRecvClient)");
    try { ::close      (pipeSendClient); } CARLA_SAFE_EXCEPTION("close(pipeSendClient)");
#endif

    return false;

    // maybe unused
    (void)size; (void)ovRecv; (void)process;
}

bool CarlaPipeServer::startPipeServer(const char* const filename,
                                      const char* const arg1,
                                      const char* const arg2,
                                      const int size,
                                      const int timeOutMilliseconds) noexcept
{
    return startPipeServer(nullptr, filename, arg1, arg2, size, timeOutMilliseconds);
}

void CarlaPipeServer::stopPipeServer(const uint32_t timeOutMilliseconds) noexcept
{
    carla_debug("CarlaPipeServer::stopPipeServer(%i)", timeOutMilliseconds);

#ifdef CARLA_OS_WIN
    if (pData->processInfo.hThread != INVALID_HANDLE_VALUE || pData->processInfo.hProcess != INVALID_HANDLE_VALUE)
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (pData->pipeSend != INVALID_PIPE_VALUE && ! pData->pipeClosed)
        {
            if (_writeMsgBuffer("__carla-quit__\n", 15))
                syncMessages();
        }

        waitForProcessToStopOrKillIt(pData->processInfo.hProcess, timeOutMilliseconds);
        try { ::CloseHandle(pData->processInfo.hThread);  } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hThread)");
        try { ::CloseHandle(pData->processInfo.hProcess); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hProcess)");
        carla_zeroStruct(pData->processInfo);
        pData->processInfo.hProcess = INVALID_HANDLE_VALUE;
        pData->processInfo.hThread  = INVALID_HANDLE_VALUE;
    }
#else
    if (pData->pid != -1)
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (pData->pipeSend != INVALID_PIPE_VALUE && ! pData->pipeClosed)
        {
            if (_writeMsgBuffer("__carla-quit__\n", 15))
                syncMessages();
        }

        waitForChildToStopOrKillIt(pData->pid, timeOutMilliseconds);
        pData->pid = -1;
    }
#endif

    closePipeServer();
}

void CarlaPipeServer::closePipeServer() noexcept
{
    carla_debug("CarlaPipeServer::closePipeServer()");

    pData->pipeClosed = true;

    const CarlaMutexLocker cml(pData->writeLock);

    if (pData->pipeRecv != INVALID_PIPE_VALUE)
    {
#ifdef CARLA_OS_WIN
        DisconnectNamedPipe(pData->pipeRecv);

        try { ::CloseHandle(pData->pipeRecv); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->pipeRecv)");
#else
        try { ::close      (pData->pipeRecv); } CARLA_SAFE_EXCEPTION("close(pData->pipeRecv)");
#endif
        pData->pipeRecv = INVALID_PIPE_VALUE;
    }

    if (pData->pipeSend != INVALID_PIPE_VALUE)
    {
#ifdef CARLA_OS_WIN
        DisconnectNamedPipe(pData->pipeSend);

        try { ::CloseHandle(pData->pipeSend); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->pipeSend)");
#else
        try { ::close      (pData->pipeSend); } CARLA_SAFE_EXCEPTION("close(pData->pipeSend)");
#endif
        pData->pipeSend = INVALID_PIPE_VALUE;
    }
}

void CarlaPipeServer::writeShowMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("show\n", 5))
        return;

    syncMessages();
}

void CarlaPipeServer::writeFocusMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("focus\n", 6))
        return;

    syncMessages();
}

void CarlaPipeServer::writeHideMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);

    if (! _writeMsgBuffer("show\n", 5))
        return;

    syncMessages();
}

// -----------------------------------------------------------------------

CarlaPipeClient::CarlaPipeClient() noexcept
    : CarlaPipeCommon()
{
    carla_debug("CarlaPipeClient::CarlaPipeClient()");
}

CarlaPipeClient::~CarlaPipeClient() /*noexcept*/
{
    carla_debug("CarlaPipeClient::~CarlaPipeClient()");

    closePipeClient();
}

bool CarlaPipeClient::initPipeClient(const char* argv[]) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->pipeRecv == INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend == INVALID_PIPE_VALUE, false);
    carla_debug("CarlaPipeClient::initPipeClient(%p)", argv);

    const CarlaMutexLocker cml(pData->writeLock);

    //----------------------------------------------------------------
    // read arguments

#ifdef CARLA_OS_WIN
    const char* const pipeRecvServerStr = argv[3];
    const char* const pipeSendServerStr = argv[4];

    HANDLE pipeRecvServer = ::CreateFileA(pipeRecvServerStr, GENERIC_READ, 0x0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE pipeSendServer = ::CreateFileA(pipeSendServerStr, GENERIC_WRITE, 0x0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    CARLA_SAFE_ASSERT_RETURN(pipeRecvServer != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendServer != INVALID_HANDLE_VALUE, false);
#else
    const int pipeRecvServer = std::atoi(argv[3]);
    const int pipeSendServer = std::atoi(argv[4]);
    /* */ int pipeRecvClient = std::atoi(argv[5]);
    /* */ int pipeSendClient = std::atoi(argv[6]);

    CARLA_SAFE_ASSERT_RETURN(pipeRecvServer > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendServer > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeRecvClient > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendClient > 0, false);

    //----------------------------------------------------------------
    // close duplicated handles used by the client

    try { ::close(pipeRecvClient); } CARLA_SAFE_EXCEPTION("close(pipeRecvClient)");
    try { ::close(pipeSendClient); } CARLA_SAFE_EXCEPTION("close(pipeSendClient)");

    //----------------------------------------------------------------
    // kill ourselves if parent dies

    carla_terminateProcessOnParentExit(false);
#endif

    //----------------------------------------------------------------
    // done

    pData->pipeRecv = pipeRecvServer;
    pData->pipeSend = pipeSendServer;
    pData->pipeClosed = false;
    pData->clientClosingDown = false;

    if (writeMessage("\n", 1))
        syncMessages();

    return true;
}

void CarlaPipeClient::closePipeClient() noexcept
{
    carla_debug("CarlaPipeClient::closePipeClient()");

    pData->pipeClosed = true;

    const CarlaMutexLocker cml(pData->writeLock);

    if (pData->pipeRecv != INVALID_PIPE_VALUE)
    {
#ifdef CARLA_OS_WIN
        try { ::CloseHandle(pData->pipeRecv); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->pipeRecv)");
#else
        try { ::close      (pData->pipeRecv); } CARLA_SAFE_EXCEPTION("close(pData->pipeRecv)");
#endif
        pData->pipeRecv = INVALID_PIPE_VALUE;
    }

    if (pData->pipeSend != INVALID_PIPE_VALUE)
    {
#ifdef CARLA_OS_WIN
        try { ::CloseHandle(pData->pipeSend); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->pipeSend)");
#else
        try { ::close      (pData->pipeSend); } CARLA_SAFE_EXCEPTION("close(pData->pipeSend)");
#endif
        pData->pipeSend = INVALID_PIPE_VALUE;
    }
}

void CarlaPipeClient::writeExitingMessageAndWait() noexcept
{
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (_writeMsgBuffer("exiting\n", 8))
            syncMessages();
    }

    // NOTE: no more messages are handled after this point
    pData->clientClosingDown = true;

    for (int i=0; i < 100 && ! pData->pipeClosed; ++i)
    {
        d_msleep(50);
        idlePipe(true);
    }

    if (! pData->pipeClosed)
        carla_stderr2("writeExitingMessageAndWait pipe is still running!");
}

// -----------------------------------------------------------------------

#undef INVALID_PIPE_VALUE
