/*
 * Carla Pipe Utilities
 * Copyright (C) 2013-2017 Filipe Coelho <falktx@falktx.com>
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
#include <fcntl.h>

#include "water/misc/Time.h"
#include "water/text/String.h"

#ifndef CARLA_OS_WIN
# include <cerrno>
# include <signal.h>
# include <sys/wait.h>
# ifdef CARLA_OS_LINUX
#  include <sys/prctl.h>
# endif
#endif

#ifdef CARLA_OS_WIN
# define INVALID_PIPE_VALUE INVALID_HANDLE_VALUE
#else
# define INVALID_PIPE_VALUE -1
#endif

using water::Time;

#ifdef CARLA_OS_WIN
// -----------------------------------------------------------------------
// win32 stuff

static inline
ssize_t ReadFileWin32(const HANDLE pipeh, void* const buf, const std::size_t numBytes)
{
    DWORD dsize = numBytes;
    DWORD available = 0;

    if (::PeekNamedPipe(pipeh, nullptr, 0, nullptr, &available, nullptr) == FALSE || available == 0)
        return -1;

    if (::ReadFile(pipeh, buf, dsize, &dsize, nullptr) != FALSE)
        return static_cast<ssize_t>(dsize);

    return -1;
}

static inline
ssize_t WriteFileWin32(const HANDLE pipeh, const void* const buf, const std::size_t numBytes)
{
    DWORD dsize = numBytes;

    if (::WriteFile(pipeh, buf, dsize, &dsize, nullptr) != FALSE)
        return static_cast<ssize_t>(dsize);

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

#if 0 // FIXME
        // If there are spaces, surround it with quotes. If there are quotes,
        // replace them with \" so that CommandLineToArgv will correctly parse them.
        if (arg.containsAnyOf("\" "))
            arg = arg.replace("\"", "\\\"").quoted();
#endif

        command << arg << ' ';
    }

    command = command.trim();

    STARTUPINFO startupInfo;
    carla_zeroStruct(startupInfo);
    startupInfo.cb = sizeof(startupInfo);

    return CreateProcess(nullptr, const_cast<LPSTR>(command.toRawUTF8()),
                         nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                         nullptr, nullptr, &startupInfo, processInfo) != FALSE;
}

static inline
bool waitForClientConnect(const HANDLE pipe, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pipe != INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    bool connecting = true;
    const uint32_t timeoutEnd(Time::getMillisecondCounter() + timeOutMilliseconds);

    for (; connecting && ::ConnectNamedPipe(pipe, nullptr) == FALSE;)
    {
        const DWORD err = ::GetLastError();

        switch (err)
        {
        case ERROR_PIPE_CONNECTED:
            connecting = false;
            break;

        case ERROR_IO_PENDING:
        case ERROR_PIPE_LISTENING:
            if (Time::getMillisecondCounter() < timeoutEnd)
            {
                carla_msleep(5);
                continue;
            }
            carla_stderr("waitForClientFirstMessage() - connect timed out");
            return false;

        default:
            carla_stderr("waitForClientFirstMessage() - connect returned %i", int(err));
            return false;
        }
    }

    return true;
}
#else
static inline
bool startProcess(const char* const argv[], pid_t& pidinst) noexcept
{
    const ScopedEnvVar sev1("LD_LIBRARY_PATH", nullptr);
    const ScopedEnvVar sev2("LD_PRELOAD", nullptr);

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
    CARLA_SAFE_ASSERT_RETURN(pipe != INVALID_PIPE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    char c;
    ssize_t ret;
    const uint32_t timeoutEnd(Time::getMillisecondCounter() + timeOutMilliseconds);

#ifdef CARLA_OS_WIN
    if (! waitForClientConnect(pipe, timeOutMilliseconds))
        return false;
#endif

    for (;;)
    {
        try {
#ifdef CARLA_OS_WIN
            ret = ::ReadFileWin32(pipe, &c, 1);
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
                if (Time::getMillisecondCounter() < timeoutEnd)
                {
                    carla_msleep(5);
                    continue;
                }
                carla_stderr("waitForClientFirstMessage() - read timed out");
            }
            else
            {
#ifdef CARLA_OS_WIN
                carla_stderr("waitForClientFirstMessage() - read failed");
#else
                CarlaString error(std::strerror(errno));
                carla_stderr("waitForClientFirstMessage() - read failed: %s", error.buffer());
#endif
            }
            return false;

        default: // ???
            carla_stderr("waitForClientFirstMessage() - read returned %i", int(ret));
            return false;
        }
    }
}

// -----------------------------------------------------------------------
// waitForChildToStop / waitForProcessToStop

#ifdef CARLA_OS_WIN
static inline
bool waitForProcessToStop(const PROCESS_INFORMATION& processInfo, const uint32_t timeOutMilliseconds) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(processInfo.hProcess != INVALID_HANDLE_VALUE, false);
    CARLA_SAFE_ASSERT_RETURN(timeOutMilliseconds > 0, false);

    const uint32_t timeoutEnd(Time::getMillisecondCounter() + timeOutMilliseconds);

    for (;;)
    {
        switch (WaitForSingleObject(processInfo.hProcess, 0))
        {
        case WAIT_OBJECT_0:
        case -1:
            return true;
        }

        if (Time::getMillisecondCounter() >= timeoutEnd)
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

        if (TerminateProcess(processInfo.hProcess, 9) != FALSE)
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
    const uint32_t timeoutEnd(Time::getMillisecondCounter() + timeOutMilliseconds);

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
            if (Time::getMillisecondCounter() < timeoutEnd)
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
    HANDLE pipeRecv;
    HANDLE pipeSend;
#else
    pid_t pid;
    int pipeRecv;
    int pipeSend;
#endif

    // read functions must only be called in context of idlePipe()
    bool isReading;

    // print error only once
    bool lastMessageFailed;

    // common write lock
    CarlaMutex writeLock;

    // temporary buffers for _readline()
    mutable char        tmpBuf[0xff+1];
    mutable CarlaString tmpStr;

    PrivateData() noexcept
#ifdef CARLA_OS_WIN
        : processInfo(),
#else
        : pid(-1),
#endif
          pipeRecv(INVALID_PIPE_VALUE),
          pipeSend(INVALID_PIPE_VALUE),
          isReading(false),
          lastMessageFailed(false),
          writeLock(),
          tmpBuf(),
          tmpStr()
    {
#ifdef CARLA_OS_WIN
        carla_zeroStruct(processInfo);
        processInfo.hProcess = INVALID_HANDLE_VALUE;
        processInfo.hThread  = INVALID_HANDLE_VALUE;
#endif

        carla_zeroChars(tmpBuf, 0xff+1);
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
#ifdef CARLA_OS_WIN
    // TESTING remove later
    const CarlaMutexTryLocker cmtl(pData->writeLock);
    CARLA_SAFE_ASSERT_RETURN(cmtl.wasNotLocked(), false);
    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend != INVALID_PIPE_VALUE, false);

    try {
        return (::FlushFileBuffers(pData->pipeSend) != FALSE);
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);
#else
    // nothing to do
    return true;
#endif
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

    _writeMsgBuffer("midiprogram\n", 12);

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
    ssize_t ret = -1;

    pData->tmpStr.clear();

    for (int i=0; i<0xff; ++i)
    {
        try {
#ifdef CARLA_OS_WIN
            ret = ::ReadFileWin32(pData->pipeRecv, &c, 1);
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
    const uint32_t timeoutEnd(Time::getMillisecondCounter() + timeOutMilliseconds);

    for (;;)
    {
        if (const char* const msg = _readline())
            return msg;

        if (Time::getMillisecondCounter() >= timeoutEnd)
            break;

        carla_msleep(5);
    }

    carla_stderr("readlineblock timed out");
    return nullptr;
}

bool CarlaPipeCommon::_writeMsgBuffer(const char* const msg, const std::size_t size) const noexcept
{
    // TESTING remove later
    const CarlaMutexTryLocker cmtl(pData->writeLock);
    CARLA_SAFE_ASSERT_RETURN(cmtl.wasNotLocked(), false);
    CARLA_SAFE_ASSERT_RETURN(pData->pipeSend != INVALID_PIPE_VALUE, false);

    ssize_t ret;

    try {
#ifdef CARLA_OS_WIN
        ret = ::WriteFileWin32(pData->pipeSend, msg, size);
#else
        ret = ::write(pData->pipeSend, msg, size);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaPipeCommon::writeMsgBuffer", false);

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
                "CarlaPipeCommon::_writeMsgBuffer(..., " P_SIZE ") - failed with " P_SSIZE ", message was:\n%s",
                size, ret, msg);
    }

    return false;
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

bool CarlaPipeServer::startPipeServer(const char* const filename,
                                      const char* const arg1,
                                      const char* const arg2,
                                      const int size) noexcept
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

    char pipeRecvServerStr[100+1];
    char pipeSendServerStr[100+1];
    char pipeRecvClientStr[100+1];
    char pipeSendClientStr[100+1];

    pipeRecvServerStr[100] = '\0';
    pipeSendServerStr[100] = '\0';
    pipeRecvClientStr[100] = '\0';
    pipeSendClientStr[100] = '\0';

    const CarlaMutexLocker cml(pData->writeLock);

    //----------------------------------------------------------------
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

    pipe1 = ::CreateNamedPipeA(pipeRecvServerStr, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT, 2, size, size, 0, nullptr);

    if (pipe1 == INVALID_HANDLE_VALUE)
    {
        fail("pipe creation failed");
        return false;
    }

    pipe2 = ::CreateNamedPipeA(pipeSendServerStr, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT, 2, size, size, 0, nullptr);

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

    //----------------------------------------------------------------
    // set size, non-fatal

#ifdef CARLA_OS_LINUX
    try {
        ::fcntl(pipeRecvClient, F_SETPIPE_SZ, size);
    } CARLA_SAFE_EXCEPTION("Set pipe size");

    try {
        ::fcntl(pipeRecvServer, F_SETPIPE_SZ, size);
    } CARLA_SAFE_EXCEPTION("Set pipe size");
#endif

    //----------------------------------------------------------------
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

    argv[3] = pipeRecvServerStr;
    argv[4] = pipeSendServerStr;
    argv[5] = pipeRecvClientStr;
    argv[6] = pipeSendClientStr;

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
        try { ::CloseHandle(pipe1); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe1)");
        try { ::CloseHandle(pipe2); } CARLA_SAFE_EXCEPTION("CloseHandle(pipe2)");
        fail("startProcess() failed");
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

    //----------------------------------------------------------------
    // close duplicated handles used by the client

    try { ::close(pipeRecvServer); } CARLA_SAFE_EXCEPTION("close(pipeRecvServer)");
    try { ::close(pipeSendServer); } CARLA_SAFE_EXCEPTION("close(pipeSendServer)");
    pipeRecvServer = pipeSendServer = INVALID_PIPE_VALUE;
#endif

    //----------------------------------------------------------------
    // wait for client to say something

    if (waitForClientFirstMessage(pipeRecvClient, 10*1000 /* 10 secs */))
    {
#ifdef CARLA_OS_WIN
        CARLA_SAFE_ASSERT(waitForClientConnect(pipeSendClient, 1000 /* 1 sec */));
#endif
        pData->pipeRecv = pipeRecvClient;
        pData->pipeSend = pipeSendClient;
        carla_stdout("ALL OK!");
        return true;
    }

    //----------------------------------------------------------------
    // failed to set non-block or get first child message, cannot continue

#ifdef CARLA_OS_WIN
    if (TerminateProcess(pData->processInfo.hProcess, 9) != FALSE)
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
    try { ::CloseHandle(pipeRecvClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeRecvClient)");
    try { ::CloseHandle(pipeSendClient); } CARLA_SAFE_EXCEPTION("CloseHandle(pipeSendClient)");
#else
    try { ::close      (pipeRecvClient); } CARLA_SAFE_EXCEPTION("close(pipeRecvClient)");
    try { ::close      (pipeSendClient); } CARLA_SAFE_EXCEPTION("close(pipeSendClient)");
#endif

    return false;
}

void CarlaPipeServer::stopPipeServer(const uint32_t timeOutMilliseconds) noexcept
{
    carla_debug("CarlaPipeServer::stopPipeServer(%i)", timeOutMilliseconds);

#ifdef CARLA_OS_WIN
    if (pData->processInfo.hThread != INVALID_HANDLE_VALUE || pData->processInfo.hProcess != INVALID_HANDLE_VALUE)
    {
        const CarlaMutexLocker cml(pData->writeLock);

        if (pData->pipeSend != INVALID_PIPE_VALUE)
        {
            _writeMsgBuffer("quit\n", 5);
            flushMessages();
        }

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
        {
            _writeMsgBuffer("quit\n", 5);
            flushMessages();
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
    pipeRecvClient = pipeSendClient = INVALID_PIPE_VALUE;

    //----------------------------------------------------------------
    // kill ourselves if parent dies

# ifdef CARLA_OS_LINUX
    ::prctl(PR_SET_PDEATHSIG, SIGKILL);
    // TODO, osx version too, see https://stackoverflow.com/questions/284325/how-to-make-child-process-die-after-parent-exits
# endif
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
