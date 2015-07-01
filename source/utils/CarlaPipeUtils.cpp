/*
 * Carla Utility Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPipeUtils.hpp"
#include "CarlaString.hpp"
#include "CarlaMIDI.h"

// needed for atom-util
#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

#ifdef BUILDING_CARLA
# include "lv2/atom-util.h"
#else
# include "lv2/lv2plug.in/ns/ext/atom/util.h"
#endif

#include <clocale>

#ifdef CARLA_OS_WIN
# include <ctime>
# include "juce_core.h"
#else
# include <cerrno>
# include <fcntl.h>
# include <signal.h>
# include <sys/wait.h>
#endif

#ifdef CARLA_OS_WIN
# define INVALID_PIPE_VALUE INVALID_HANDLE_VALUE
#else
# define INVALID_PIPE_VALUE -1
#endif

#ifdef CARLA_OS_WIN
// -----------------------------------------------------------------------
// win32 stuff

struct OverlappedEvent {
    OVERLAPPED over;

    OverlappedEvent()
        : over()
    {
        carla_zeroStruct(over);
        over.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
    }

    ~OverlappedEvent()
    {
        ::CloseHandle(over.hEvent);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(OverlappedEvent)
};

// -----------------------------------------------------------------------
// ReadFile

static inline
ssize_t ReadFileBlock(const HANDLE pipeh, void* const buf, const std::size_t numBytes)
{
    DWORD dsize;

    if (::ReadFile(pipeh, buf, numBytes, &dsize, nullptr) != FALSE)
        return static_cast<ssize_t>(dsize);

    return -1;
}

static inline
ssize_t ReadFileNonBlock(const HANDLE pipeh, const HANDLE cancelh, void* const buf, const std::size_t numBytes)
{
    DWORD dsize = numBytes;
    OverlappedEvent over;

    if (::ReadFile(pipeh, buf, numBytes, nullptr /*&dsize*/, &over.over) != FALSE)
        return static_cast<ssize_t>(dsize);

    if (::GetLastError() == ERROR_IO_PENDING)
    {
        HANDLE handles[] = { over.over.hEvent, cancelh };

        if (::WaitForMultipleObjects(2, handles, FALSE, 0) != WAIT_OBJECT_0)
        {
            ::CancelIo(pipeh);
            return -1;
        }

        if (::GetOverlappedResult(pipeh, &over.over, nullptr /*&dsize*/, FALSE) != FALSE)
            return static_cast<ssize_t>(dsize);
    }

    return -1;
}

// -----------------------------------------------------------------------
// WriteFile

static inline
ssize_t WriteFileBlock(const HANDLE pipeh, const void* const buf, const std::size_t numBytes)
{
    DWORD dsize;

    if (::WriteFile(pipeh, buf, numBytes, &dsize, nullptr) != FALSE)
        return static_cast<ssize_t>(dsize);

    return -1;
}

static inline
ssize_t WriteFileNonBlock(const HANDLE pipeh, const HANDLE cancelh, const void* const buf, const std::size_t numBytes)
{
    DWORD dsize = numBytes;
    OverlappedEvent over;

    if (::WriteFile(pipeh, buf, numBytes, nullptr /*&dsize*/, &over.over) != FALSE)
        return static_cast<ssize_t>(dsize);

    if (::GetLastError() == ERROR_IO_PENDING)
    {
        HANDLE handles[] = { over.over.hEvent, cancelh };

        if (::WaitForMultipleObjects(2, handles, FALSE, 0) != WAIT_OBJECT_0)
        {
            ::CancelIo(pipeh);
            return -1;
        }

        if (::GetOverlappedResult(pipeh, &over.over, &dsize, FALSE) != FALSE)
            return static_cast<ssize_t>(dsize);
    }

    return -1;
}
#endif // CARLA_OS_WIN

// -----------------------------------------------------------------------
// getMillisecondCounter

static uint32_t lastMSCounterValue = 0;

static inline
uint32_t getMillisecondCounter() noexcept
{
    uint32_t now;

#ifdef CARLA_OS_WIN
    now = static_cast<uint32_t>(timeGetTime());
#else
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    now =  t.tv_sec * 1000 + t.tv_nsec / 1000000;
#endif

    if (now < lastMSCounterValue)
    {
        // in multi-threaded apps this might be called concurrently, so
        // make sure that our last counter value only increases and doesn't
        // go backwards..
        if (now < lastMSCounterValue - 1000)
            lastMSCounterValue = now;
    }
    else
    {
        lastMSCounterValue = now;
    }

    return now;
}

// -----------------------------------------------------------------------
// startProcess

#ifdef CARLA_OS_WIN
static inline
bool startProcess(const char* const argv[], PROCESS_INFORMATION* const processInfo)
{
    CARLA_SAFE_ASSERT_RETURN(processInfo != nullptr, false);

    using juce::String;

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
    carla_stdout("startProcess() command:\n%s", command.toRawUTF8());

    STARTUPINFOW startupInfo;
    carla_zeroStruct(startupInfo);
# if 0
    startupInfo.hStdInput  = INVALID_HANDLE_VALUE;
    startupInfo.hStdOutput = INVALID_HANDLE_VALUE;
    startupInfo.hStdError  = INVALID_HANDLE_VALUE;
    startupInfo.dwFlags    = STARTF_USESTDHANDLES;
# endif
    startupInfo.cb         = sizeof(STARTUPINFOW);

    return CreateProcessW(nullptr, const_cast<LPWSTR>(command.toWideCharPointer()),
                          nullptr, nullptr, TRUE, 0x0, nullptr, nullptr, &startupInfo, processInfo) != FALSE;
}
#else
static inline
bool startProcess(const char* const argv[], pid_t& pidinst) noexcept
{
    const pid_t ret = pidinst = vfork();

    switch (ret)
    {
    case 0: { // child process
        execvp(argv[0], const_cast<char* const*>(argv));

        CarlaString error(std::strerror(errno));
        carla_stderr2("exec failed: %s", error.buffer());

        _exit(1); // this is not noexcept safe but doesn't matter anyway
    }   break;

    case -1: { // error
        CarlaString error(std::strerror(errno));
        carla_stderr2("fork() failed: %s", error.buffer());
    }   break;
    }

    return (ret > 0);
}
#endif

// -----------------------------------------------------------------------
// waitForClientFirstMessage

template<typename P>
static inline
bool waitForClientFirstMessage(const P& pipe, const uint32_t timeOutMilliseconds) noexcept
{
#ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(pipe.handle != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pipe.cancel != INVALID_HANDLE_VALUE, false);
#else
    CARLA_SAFE_ASSERT_RETURN(pipe > 0, false);
#endif
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    char c;
    ssize_t ret;
    const uint32_t timeoutEnd(getMillisecondCounter() + timeOutMilliseconds);

    for (;;)
    {
        try {
#ifdef CARLA_OS_WIN
            //ret = ::ReadFileBlock(pipe.handle, &c, 1);
            ret = ::ReadFileNonBlock(pipe.handle, pipe.cancel, &c, 1);
#else
            ret = ::read(pipe, &c, 1);
#endif
        } CARLA_SAFE_EXCEPTION_BREAK("read pipefd");

        switch (ret)
        {
        case -1: // failed to read
#ifndef CARLA_OS_WIN
            if (errno == EAGAIN)
#endif
            {
                if (getMillisecondCounter() < timeoutEnd)
                {
                    carla_msleep(5);
                    continue;
                }
                carla_stderr("waitForClientFirstMessage() - timed out");
            }
#ifndef CARLA_OS_WIN
            else
            {
                carla_stderr("waitForClientFirstMessage() - read failed");
                CarlaString error(std::strerror(errno));
                carla_stderr("waitForClientFirstMessage() - read failed: %s", error.buffer());
            }
#endif
            break;

        case 1: // read ok
            if (c == '\n')
            {
                // success
                return true;
            }
            else
            {
                carla_stderr("waitForClientFirstMessage() - read has wrong first char '%c'", c);
            }
            break;

        default: // ???
            carla_stderr("waitForClientFirstMessage() - read returned %i", int(ret));
            break;
        }

        break;
    }

    return false;
}

// -----------------------------------------------------------------------
// waitForChildToStop / waitForProcessToStop

#ifdef CARLA_OS_WIN
static inline
bool waitForProcessToStop(const PROCESS_INFORMATION& processInfo, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(processInfo.hProcess != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    // TODO - this code is completly wrong...

    const uint32_t timeoutEnd(getMillisecondCounter() + timeOutMilliseconds);

    for (;;)
    {
        if (WaitForSingleObject(processInfo.hProcess, 0) == WAIT_OBJECT_0)
            return true;

        if (getMillisecondCounter() >= timeoutEnd)
            break;

        carla_msleep(5);
    }

    return false;
}

static inline
void waitForProcessToStopOrKillIt(const PROCESS_INFORMATION& processInfo, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(processInfo.hProcess != INVALID_HANDLE_VALUE,);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0,);

    if (! waitForProcessToStop(processInfo, timeOutMilliseconds))
    {
        carla_stderr("waitForProcessToStopOrKillIt() - process didn't stop, force termination");

        if (TerminateProcess(processInfo.hProcess, 0) != FALSE)
        {
            // wait for process to stop
            waitForProcessToStop(processInfo, timeOutMilliseconds);
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
    const uint32_t timeoutEnd(getMillisecondCounter() + timeOutMilliseconds);

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
                CarlaString error(std::strerror(errno));
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
            if (getMillisecondCounter() < timeoutEnd)
            {
                carla_msleep(5);
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
            CarlaString error(std::strerror(errno));
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
    HANDLE cancelEvent;
    HANDLE pipeRecv;
    HANDLE pipeSend;
#else
    pid_t pid;
    int pipeRecv;
    int pipeSend;
#endif

    // read functions must only be called in context of idlePipe()
    bool isReading;

    // common write lock
    CarlaMutex writeLock;

    // temporary buffers for _readline()
    mutable char        tmpBuf[0xff+1];
    mutable CarlaString tmpStr;

    PrivateData() noexcept
#ifdef CARLA_OS_WIN
        : processInfo(),
          cancelEvent(INVALID_HANDLE_VALUE),
#else
        : pid(-1),
#endif
          pipeRecv(INVALID_PIPE_VALUE),
          pipeSend(INVALID_PIPE_VALUE),
          isReading(false),
          writeLock(),
          tmpBuf(),
          tmpStr()
    {
#ifdef CARLA_OS_WIN
        carla_zeroStruct(processInfo);
        processInfo.hProcess = INVALID_HANDLE_VALUE;
        processInfo.hThread  = INVALID_HANDLE_VALUE;

        try {
            cancelEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        } CARLA_SAFE_EXCEPTION("CreateEvent");
#endif

        carla_zeroChars(tmpBuf, 0xff+1);
    }

    ~PrivateData() noexcept
    {
#ifdef CARLA_OS_WIN
        if (cancelEvent != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(cancelEvent);
            cancelEvent = INVALID_HANDLE_VALUE;
        }
#endif
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PrivateData)
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
    return (pData->pipeRecv != INVALID_PIPE_VALUE && pData->pipeSend != INVALID_PIPE_VALUE);
}

void CarlaPipeCommon::idlePipe(const bool onlyOnce) noexcept
{
    const char* locale = nullptr;

    for (;;)
    {
        const char* const msg(_readline());

        if (msg == nullptr)
            break;

        if (locale == nullptr && ! onlyOnce)
        {
            locale = carla_strdup_safe(::setlocale(LC_NUMERIC, nullptr));
            ::setlocale(LC_NUMERIC, "C");
        }

        pData->isReading = true;

        try {
            msgReceived(msg);
        } CARLA_SAFE_EXCEPTION("msgReceived");

        pData->isReading = false;

        delete[] msg;

        if (onlyOnce)
            break;
    }

    if (locale != nullptr)
    {
        ::setlocale(LC_NUMERIC, locale);
        delete[] locale;
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

    if (const char* const msg = _readlineblock())
    {
        value = (std::strcmp(msg, "true") == 0);
        delete[] msg;
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsByte(uint8_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        int tmp = std::atoi(msg);
        delete[] msg;

        if (tmp >= 0 && tmp <= 0xFF)
        {
            value = static_cast<uint8_t>(tmp);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsInt(int32_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        value = std::atoi(msg);
        delete[] msg;
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsUInt(uint32_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        int32_t tmp = std::atoi(msg);
        delete[] msg;

        if (tmp >= 0)
        {
            value = static_cast<uint32_t>(tmp);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsLong(int64_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        value = std::atol(msg);
        delete[] msg;
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsULong(uint64_t& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        int64_t tmp = std::atol(msg);
        delete[] msg;

        if (tmp >= 0)
        {
            value = static_cast<uint64_t>(tmp);
            return true;
        }
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsFloat(float& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        value = static_cast<float>(std::atof(msg));
        delete[] msg;
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsDouble(double& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        value = std::atof(msg);
        delete[] msg;
        return true;
    }

    return false;
}

bool CarlaPipeCommon::readNextLineAsString(const char*& value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->isReading, false);

    if (const char* const msg = _readlineblock())
    {
        value = msg;
        return true;
    }

    return false;
}

// -------------------------------------------------------------------
// must be locked before calling

bool CarlaPipeCommon::writeMessage(const char* const msg) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msg != nullptr && msg[0] != '\0', false);

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

    return _writeMsgBuffer(msg, size);
}

bool CarlaPipeCommon::writeAndFixMessage(const char* const msg) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msg != nullptr, false);

    const std::size_t size(std::strlen(msg));

    char fixedMsg[size+2];

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

    return _writeMsgBuffer(fixedMsg, size+1);
}

bool CarlaPipeCommon::flushMessages() const noexcept
{
    // TESTING remove later (replace with trylock scope)
    if (pData->writeLock.tryLock())
    {
        carla_safe_assert("! pData->writeLock.tryLock()", __FILE__, __LINE__);
        pData->writeLock.unlock();
        return false;
    }

    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend != INVALID_PIPE_VALUE, false);

    try {
#ifdef CARLA_OS_WIN
        return (::FlushFileBuffers(pData->pipeSend) != FALSE);
#else
        return (::fsync(pData->pipeSend) == 0);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);
}

// -------------------------------------------------------------------

void CarlaPipeCommon::writeErrorMessage(const char* const error) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(error != nullptr && error[0] != '\0',);

    const CarlaMutexLocker cml(pData->writeLock);
    _writeMsgBuffer("error\n", 6);
    writeAndFixMessage(error);
    flushMessages();
}

void CarlaPipeCommon::writeControlMessage(const uint32_t index, const float value) const noexcept
{
    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);
    const ScopedLocale csl;

    _writeMsgBuffer("control\n", 8);

    {
        std::snprintf(tmpBuf, 0xff, "%i\n", index);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%f\n", value);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));
    }

    flushMessages();
}

void CarlaPipeCommon::writeConfigureMessage(const char* const key, const char* const value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("configure\n", 10);

    {
        writeAndFixMessage(key);
        writeAndFixMessage(value);
    }

    flushMessages();
}

void CarlaPipeCommon::writeProgramMessage(const uint32_t index) const noexcept
{
    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("program\n", 8);

    {
        std::snprintf(tmpBuf, 0xff, "%i\n", index);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));
    }

    flushMessages();
}

void CarlaPipeCommon::writeMidiProgramMessage(const uint32_t bank, const uint32_t program) const noexcept
{
    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("midiprogram\n", 8);

    {
        std::snprintf(tmpBuf, 0xff, "%i\n", bank);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%i\n", program);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));
    }

    flushMessages();
}

void CarlaPipeCommon::writeMidiNoteMessage(const bool onOff, const uint8_t channel, const uint8_t note, const uint8_t velocity) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velocity < MAX_MIDI_VALUE,);

    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("note\n", 5);

    {
        std::snprintf(tmpBuf, 0xff, "%s\n", bool2str(onOff));
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%i\n", channel);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%i\n", note);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%i\n", velocity);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));
    }

    flushMessages();
}

void CarlaPipeCommon::writeLv2AtomMessage(const uint32_t index, const LV2_Atom* const atom) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(atom != nullptr,);

    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const uint32_t atomTotalSize(lv2_atom_total_size(atom));
    CarlaString base64atom(CarlaString::asBase64(atom, atomTotalSize));

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("atom\n", 5);

    {
        std::snprintf(tmpBuf, 0xff, "%i\n", index);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        std::snprintf(tmpBuf, 0xff, "%i\n", atomTotalSize);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        writeAndFixMessage(base64atom.buffer());
    }

    flushMessages();
}

void CarlaPipeCommon::writeLv2UridMessage(const uint32_t urid, const char* const uri) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(urid != 0,);
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0',);

    char tmpBuf[0xff+1];
    tmpBuf[0xff] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    _writeMsgBuffer("urid\n", 5);

    {
        std::snprintf(tmpBuf, 0xff, "%i\n", urid);
        _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

        writeAndFixMessage(uri);
    }

    flushMessages();
}

// -------------------------------------------------------------------

// internal
const char* CarlaPipeCommon::_readline() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->pipeRecv != INVALID_PIPE_VALUE, nullptr);

    char    c;
    char*   ptr = pData->tmpBuf;
    ssize_t ret;

    pData->tmpStr.clear();

    for (int i=0; i<0xff; ++i)
    {
        try {
#ifdef CARLA_OS_WIN
            //ret = ::ReadFileBlock(pData->pipeRecv, &c, 1);
            ret = ::ReadFileNonBlock(pData->pipeRecv, pData->cancelEvent, &c, 1);
#else
            ret = ::read(pData->pipeRecv, &c, 1);
#endif
        } CARLA_SAFE_EXCEPTION_BREAK("CarlaPipeCommon::readline() - read");

        if (ret != 1 || c == '\n')
            break;

        if (c == '\r')
            c = '\n';

        *ptr++ = c;

        if (i+1 == 0xff)
        {
            i = 0;
            *ptr = '\0';
            pData->tmpStr += pData->tmpBuf;
            ptr = pData->tmpBuf;
        }
    }

    if (ptr != pData->tmpBuf)
    {
        *ptr = '\0';
        pData->tmpStr += pData->tmpBuf;
    }
    else if (pData->tmpStr.isEmpty() && ret != 1)
    {
        // some error
        return nullptr;
    }

    try {
        return pData->tmpStr.dup();
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::readline() - dup", nullptr);
}

const char* CarlaPipeCommon::_readlineblock(const uint32_t timeOutMilliseconds) const noexcept
{
    const uint32_t timeoutEnd(getMillisecondCounter() + timeOutMilliseconds);

    for (;;)
    {
        if (const char* const msg = _readline())
            return msg;

        if (getMillisecondCounter() >= timeoutEnd)
            break;

        carla_msleep(5);
    }

    carla_stderr("readlineblock timed out");
    return nullptr;
}

bool CarlaPipeCommon::_writeMsgBuffer(const char* const msg, const std::size_t size) const noexcept
{
    // TESTING remove later (replace with trylock scope)
    if (pData->writeLock.tryLock())
    {
        carla_safe_assert("! pData->writeLock.tryLock()", __FILE__, __LINE__);
        pData->writeLock.unlock();
        return false;
    }

    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend != INVALID_PIPE_VALUE, false);

    ssize_t ret;

    try {
#ifdef CARLA_OS_WIN
        //ret = ::WriteFileBlock(pData->pipeSend, msg, size);
        ret = ::WriteFileNonBlock(pData->pipeSend, pData->cancelEvent, msg, size);
#else
        ret = ::write(pData->pipeSend, msg, size);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);

     return (ret == static_cast<ssize_t>(size));
}

// -----------------------------------------------------------------------

CarlaPipeServer::CarlaPipeServer() noexcept
    : CarlaPipeCommon()
{
    carla_debug("CarlaPipeServer::CarlaPipeServer()");
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

// -----------------------------------------------------------------------

bool CarlaPipeServer::startPipeServer(const char* const filename, const char* const arg1, const char* const arg2) noexcept
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

    const CarlaMutexLocker cml(pData->writeLock);

    //----------------------------------------------------------------
    // create pipes

#ifdef CARLA_OS_WIN
    HANDLE pipe1[2]; // read by server, written by client
    HANDLE pipe2[2]; // read by client, written by server

    SECURITY_ATTRIBUTES sa;
    carla_zeroStruct(sa);
    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    std::srand(static_cast<uint>(std::time(nullptr)));

    char strBuf[0xff+1];
    strBuf[0xff] = '\0';

    static ulong sCounter = 0;
    ++sCounter;

    const int randint = std::rand();

    std::snprintf(strBuf, 0xff, "\\\\.\\pipe\\carla-pipe1-%i-%li", randint, sCounter);
    pipe1[0] = ::CreateNamedPipeA(strBuf, PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE|PIPE_WAIT, 1, 4096, 4096, 300, &sa);
    pipe1[1] = ::CreateFileA(strBuf, GENERIC_WRITE, 0x0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, nullptr);

    std::snprintf(strBuf, 0xff, "\\\\.\\pipe\\carla-pipe2-%i-%li", randint, sCounter);
    pipe2[0] = ::CreateNamedPipeA(strBuf, PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE|PIPE_WAIT, 1, 4096, 4096, 300, &sa);
    pipe2[1] = ::CreateFileA(strBuf, GENERIC_WRITE, 0x0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, nullptr);

#if 0
    std::snprintf(strBuf, 0xff, "\\\\.\\pipe\\carla-pipe1-%i-%li", randint, sCounter);
    pipe1[1] = ::CreateNamedPipeA(strBuf, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE|PIPE_NOWAIT, 1, 4096, 4096, 120*1000, &sa);
    pipe1[0] = ::CreateFileA(strBuf, GENERIC_READ, 0x0, &sa, OPEN_EXISTING, 0x0, nullptr);

    std::snprintf(strBuf, 0xff, "\\\\.\\pipe\\carla-pipe2-%i-%li", randint, sCounter);
    pipe2[0] = ::CreateNamedPipeA(strBuf, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE|PIPE_NOWAIT, 1, 4096, 4096, 120*1000, &sa); // NB
    pipe2[1] = ::CreateFileA(strBuf, GENERIC_WRITE, 0x0, &sa, OPEN_EXISTING, 0x0, nullptr);
#endif

    if (pipe1[0] == INVALID_HANDLE_VALUE || pipe1[1] == INVALID_HANDLE_VALUE || pipe2[0] == INVALID_HANDLE_VALUE || pipe2[1] == INVALID_HANDLE_VALUE)
    {
        if (pipe1[0] != INVALID_HANDLE_VALUE) {
            try { ::CloseHandle(pipe1[0]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1[0])");
        }
        if (pipe1[1] != INVALID_HANDLE_VALUE) {
            try { ::CloseHandle(pipe1[1]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1[1])");
        }
        if (pipe2[0] != INVALID_HANDLE_VALUE) {
            try { ::CloseHandle(pipe2[0]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2[0])");
        }
        if (pipe2[1] != INVALID_HANDLE_VALUE) {
            try { ::CloseHandle(pipe2[1]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2[1])");
        }
        fail("pipe creation failed");
        return false;
    }

    HANDLE pipeRecvServer = pipe1[0];
    HANDLE pipeRecvClient = pipe2[0];
    HANDLE pipeSendClient = pipe1[1];
    HANDLE pipeSendServer = pipe2[1];
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

    int pipeRecvServer = pipe1[0];
    int pipeRecvClient = pipe2[0];
    int pipeSendClient = pipe1[1];
    int pipeSendServer = pipe2[1];
#endif

    //----------------------------------------------------------------
    // set arguments

    const char* argv[8];

    //----------------------------------------------------------------
    // argv[0] => filename

    argv[0] = filename;

    //----------------------------------------------------------------
    // argv[1-2] => args

    argv[1] = arg1;
    argv[2] = arg2;

    //----------------------------------------------------------------
    // argv[3-6] => pipes

    char pipeRecvServerStr[100+1];
    char pipeRecvClientStr[100+1];
    char pipeSendServerStr[100+1];
    char pipeSendClientStr[100+1];

    std::snprintf(pipeRecvServerStr, 100, P_INTPTR, (intptr_t)pipeRecvServer); // pipe1[0]
    std::snprintf(pipeRecvClientStr, 100, P_INTPTR, (intptr_t)pipeRecvClient); // pipe2[0]
    std::snprintf(pipeSendServerStr, 100, P_INTPTR, (intptr_t)pipeSendServer); // pipe2[1]
    std::snprintf(pipeSendClientStr, 100, P_INTPTR, (intptr_t)pipeSendClient); // pipe1[1]

    pipeRecvServerStr[100] = '\0';
    pipeRecvClientStr[100] = '\0';
    pipeSendServerStr[100] = '\0';
    pipeSendClientStr[100] = '\0';

    argv[3] = pipeRecvServerStr; // pipe1[0] close READ
    argv[4] = pipeRecvClientStr; // pipe2[0] READ
    argv[5] = pipeSendServerStr; // pipe2[1] close SEND
    argv[6] = pipeSendClientStr; // pipe1[1] SEND

    //----------------------------------------------------------------
    // argv[7] => null

    argv[7] = nullptr;

    //----------------------------------------------------------------
    // start process

#ifdef CARLA_OS_WIN
    if (! startProcess(argv, &pData->processInfo))
    {
        carla_zeroStruct(pData->processInfo);
        pData->processInfo.hProcess = INVALID_HANDLE_VALUE;
        pData->processInfo.hThread  = INVALID_HANDLE_VALUE;
        try { ::CloseHandle(pipe1[0]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1[0])");
        try { ::CloseHandle(pipe1[1]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1[1])");
        try { ::CloseHandle(pipe2[0]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2[0])");
        try { ::CloseHandle(pipe2[1]); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2[1])");
        fail("startProcess() failed");
        return false;
    }

    // just to make sure
    CARLA_SAFE_ASSERT(pData->processInfo.hThread  != nullptr);
    CARLA_SAFE_ASSERT(pData->processInfo.hProcess != nullptr);
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
#endif

    //----------------------------------------------------------------
    // close duplicated handles used by the client

#ifdef CARLA_OS_WIN
    try { ::CloseHandle(pipeRecvServer); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeRecvServer)");
    try { ::CloseHandle(pipeSendServer); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeSendServer)");
#else
    try { ::close      (pipeRecvServer); } CARLA_SAFE_EXCEPTION("close(pipeRecvServer)");
    try { ::close      (pipeSendServer); } CARLA_SAFE_EXCEPTION("close(pipeSendServer)");
#endif
    pipeRecvServer = pipeSendServer = INVALID_PIPE_VALUE;

#ifndef CARLA_OS_WIN
    //----------------------------------------------------------------
    // set non-block reading

    int ret = 0;

    try {
        ret = ::fcntl(pipeRecvClient, F_SETFL, ::fcntl(pipeRecvClient, F_GETFL) | O_NONBLOCK);
    } catch (...) {
        ret = -1;
        fail("failed to set pipe as non-block");
    }
#endif

    //----------------------------------------------------------------
    // wait for client to say something

#ifdef CARLA_OS_WIN
    struct { HANDLE handle; HANDLE cancel; } pipe;
    pipe.handle = pipeRecvClient;
    pipe.cancel = pData->cancelEvent;
    if (             waitForClientFirstMessage(pipe, 10*1000 /* 10 secs */))
#else
    if (ret != -1 && waitForClientFirstMessage(pipeRecvClient, 10*1000 /* 10 secs */))
#endif
    {
        pData->pipeRecv = pipeRecvClient;
        pData->pipeSend = pipeSendClient;
        carla_stdout("ALL OK!");
        return true;
    }

    //----------------------------------------------------------------
    // failed to set non-block or get first child message, cannot continue

#ifdef CARLA_OS_WIN
    if (TerminateProcess(pData->processInfo.hProcess, 0) != FALSE)
    {
        // wait for process to stop
        waitForProcessToStop(pData->processInfo, 2*1000);
    }

    // clear pData->processInfo
    try { CloseHandle(pData->processInfo.hThread);  } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hThread)");
    try { CloseHandle(pData->processInfo.hProcess); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hProcess)");
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

    //----------------------------------------------------------------
    // close pipes

#ifdef CARLA_OS_WIN
    try { ::CloseHandle(pipeRecvServer); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeRecvServer)");
    try { ::CloseHandle(pipeSendServer); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeSendServer)");
#else
    try { ::close      (pipeRecvServer); } CARLA_SAFE_EXCEPTION("close(pipeRecvServer)");
    try { ::close      (pipeSendServer); } CARLA_SAFE_EXCEPTION("close(pipeSendServer)");
#endif

    return false;
}

void CarlaPipeServer::stopPipeServer(const uint32_t timeOutMilliseconds) noexcept
{
    carla_debug("CarlaPipeServer::stopPipeServer(%i)", timeOutMilliseconds);

#ifdef CARLA_OS_WIN
    if (pData->processInfo.hProcess != INVALID_HANDLE_VALUE)
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (pData->pipeSend != INVALID_PIPE_VALUE)
            _writeMsgBuffer("quit\n", 5);

        waitForProcessToStopOrKillIt(pData->processInfo, timeOutMilliseconds);
        try { CloseHandle(pData->processInfo.hThread);  } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hThread)");
        try { CloseHandle(pData->processInfo.hProcess); } CARLA_SAFE_EXCEPTION("CloseHandle(pData->processInfo.hProcess)");
        carla_zeroStruct(pData->processInfo);
        pData->processInfo.hProcess = INVALID_HANDLE_VALUE;
        pData->processInfo.hThread  = INVALID_HANDLE_VALUE;
    }
#else
    if (pData->pid != -1)
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (pData->pipeSend != INVALID_PIPE_VALUE)
            _writeMsgBuffer("quit\n", 5);

        waitForChildToStopOrKillIt(pData->pid, timeOutMilliseconds);
        pData->pid = -1;
    }
#endif

    closePipeServer();
}

void CarlaPipeServer::closePipeServer() noexcept
{
    carla_debug("CarlaPipeServer::closePipeServer()");

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

void CarlaPipeServer::writeShowMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);
    _writeMsgBuffer("show\n", 5);
    flushMessages();
}

void CarlaPipeServer::writeFocusMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);
    _writeMsgBuffer("focus\n", 6);
    flushMessages();
}

void CarlaPipeServer::writeHideMessage() const noexcept
{
    const CarlaMutexLocker cml(pData->writeLock);
    _writeMsgBuffer("show\n", 5);
    flushMessages();
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
    HANDLE pipeRecvServer = (HANDLE)std::atoll(argv[3]); // READ
    HANDLE pipeRecvClient = (HANDLE)std::atoll(argv[4]);
    HANDLE pipeSendServer = (HANDLE)std::atoll(argv[5]); // SEND
    HANDLE pipeSendClient = (HANDLE)std::atoll(argv[6]);

    CARLA_SAFE_ASSERT_RETURN(pipeRecvServer != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pipeRecvClient != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendServer != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendClient != INVALID_HANDLE_VALUE, false);
#else
    int pipeRecvServer = std::atoi(argv[3]); // READ
    int pipeRecvClient = std::atoi(argv[4]);
    int pipeSendServer = std::atoi(argv[5]); // SEND
    int pipeSendClient = std::atoi(argv[6]);

    CARLA_SAFE_ASSERT_RETURN(pipeRecvServer > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeRecvClient > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendServer > 0, false);
    CARLA_SAFE_ASSERT_RETURN(pipeSendClient > 0, false);
#endif

    //----------------------------------------------------------------
    // close duplicated handles used by the client

#ifdef CARLA_OS_WIN
    try { ::CloseHandle(pipeRecvClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeRecvClient)");
    try { ::CloseHandle(pipeSendClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeSendClient)");
#else
    try { ::close      (pipeRecvClient); } CARLA_SAFE_EXCEPTION("close(pipeRecvClient)");
    try { ::close      (pipeSendClient); } CARLA_SAFE_EXCEPTION("close(pipeSendClient)");
#endif
    pipeRecvClient = pipeSendClient = INVALID_PIPE_VALUE;

#ifndef CARLA_OS_WIN
    //----------------------------------------------------------------
    // set non-block reading

    int ret = 0;

    try {
        ret = ::fcntl(pipeRecvServer, F_SETFL, ::fcntl(pipeRecvServer, F_GETFL) | O_NONBLOCK);
    } catch (...) {
        ret = -1;
    }
    CARLA_SAFE_ASSERT_RETURN(ret != -1, false);
#endif

    //----------------------------------------------------------------
    // done

    pData->pipeRecv = pipeRecvServer;
    pData->pipeSend = pipeSendServer;

    writeMessage("\n", 1);
    flushMessages();

    return true;
}

void CarlaPipeClient::closePipeClient() noexcept
{
    carla_debug("CarlaPipeClient::closePipeClient()");

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

// -----------------------------------------------------------------------

ScopedEnvVar::ScopedEnvVar(const char* const key, const char* const value) noexcept
    : fKey(nullptr),
      fOrigValue(nullptr)
{
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);

    fKey = carla_strdup_safe(key);
    CARLA_SAFE_ASSERT_RETURN(fKey != nullptr,);

    if (const char* const origValue = std::getenv(key))
    {
        fOrigValue = carla_strdup_safe(origValue);
        CARLA_SAFE_ASSERT_RETURN(fOrigValue != nullptr,);
    }

    if (value != nullptr)
        carla_setenv(key, value);
    else if (fOrigValue != nullptr)
        carla_unsetenv(key);
}

ScopedEnvVar::~ScopedEnvVar() noexcept
{
    bool hasOrigValue = false;

    if (fOrigValue != nullptr)
    {
        hasOrigValue = true;

        carla_setenv(fKey, fOrigValue);

        delete[] fOrigValue;
        fOrigValue = nullptr;
    }

    if (fKey != nullptr)
    {
        if (! hasOrigValue)
            carla_unsetenv(fKey);

        delete[] fKey;
        fKey = nullptr;
    }
}

// -----------------------------------------------------------------------

ScopedLocale::ScopedLocale() noexcept
    : fLocale(carla_strdup_safe(::setlocale(LC_NUMERIC, nullptr)))
{
    ::setlocale(LC_NUMERIC, "C");
}

ScopedLocale::~ScopedLocale() noexcept
{
    if (fLocale != nullptr)
    {
        ::setlocale(LC_NUMERIC, fLocale);
        delete[] fLocale;
    }
}

// -----------------------------------------------------------------------

#undef INVALID_PIPE_VALUE
