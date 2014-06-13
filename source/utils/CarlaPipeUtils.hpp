/*
 * Carla Pipe utils based on lv2fil UI code
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#ifndef CARLA_PIPE_UTILS_HPP_INCLUDED
#define CARLA_PIPE_UTILS_HPP_INCLUDED

#include "CarlaMutex.hpp"
#include "CarlaString.hpp"

#include <cerrno>
#include <clocale>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define WAIT_START_TIMEOUT  3000 /* ms */
#define WAIT_ZOMBIE_TIMEOUT 3000 /* ms */
#define WAIT_STEP 100            /* ms */

// -----------------------------------------------------------------------

class CarlaPipeServer
{
protected:
    CarlaPipeServer()
        : fPipeRecv(-1),
          fPipeSend(-1),
          fIsReading(false),
          fPid(-1)
    {
        carla_debug("CarlaPipeServer::CarlaPipeServer()");

        carla_zeroChar(fTmpBuf, 0xff+1);
    }

    // -------------------------------------------------------------------

public:
    virtual ~CarlaPipeServer()
    {
        carla_debug("CarlaPipeServer::~CarlaPipeServer()");

        stop();
    }

    bool isOk() const noexcept
    {
        return (fPipeRecv != -1 && fPipeSend != -1 && fPid != -1);
    }

    bool start(const char* const filename, const char* const arg1, const char* const arg2) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
        CARLA_SAFE_ASSERT_RETURN(arg1 != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(arg2 != nullptr, false);
        carla_debug("CarlaPipeServer::start(\"%s\", \"%s\", \"%s\")", filename, arg1, arg2);

        const CarlaMutexLocker cml(fWriteLock);

        //----------------------------------------------------------------

        const char* argv[7];

        //----------------------------------------------------------------
        // argv[0] => python

        argv[0] = "/usr/bin/python3";

        //----------------------------------------------------------------
        // argv[1] => filename

        argv[1] = filename;

        //----------------------------------------------------------------
        // argv[2-3] => args

        argv[2] = arg1;
        argv[3] = arg2;

        //----------------------------------------------------------------
        // argv[4-5] => pipes

        int pipe1[2]; // written by host process, read by plugin UI process
        int pipe2[2]; // written by plugin UI process, read by host process

        if (::pipe(pipe1) != 0)
        {
            fail("pipe1 creation failed");
            return false;
        }

        if (::pipe(pipe2) != 0)
        {
            try {
                ::close(pipe1[0]);
                ::close(pipe1[1]);
            } catch (...) {}
            fail("pipe2 creation failed");
            return false;
        }

        char pipeRecv[100+1];
        char pipeSend[100+1];

        std::snprintf(pipeRecv, 100, "%d", pipe1[0]); // [0] means reading end
        std::snprintf(pipeSend, 100, "%d", pipe2[1]); // [1] means writting end

        pipeRecv[100] = '\0';
        pipeSend[100] = '\0';

        argv[4] = pipeRecv; // reading end
        argv[5] = pipeSend; // writting end

        //----------------------------------------------------------------
        // argv[6] => null

        argv[6] = nullptr;

        //----------------------------------------------------------------
        // fork

        int ret = -1;

        if ((! fork_exec(argv, &ret)) || ret == -1)
        {
            try {
                ::close(pipe1[0]);
                ::close(pipe1[1]);
            } catch (...) {}
            try {
                ::close(pipe2[0]);
                ::close(pipe2[1]);
            } catch (...) {}
            fail("fork_exec() failed");
            return false;
        }

        fPid = ret;

        // fork duplicated the handles, close pipe ends that are used by the child process
        try {
            ::close(pipe1[0]);
        } catch(...) {}
        try {
            ::close(pipe2[1]);
        } catch(...) {}

        fPipeSend = pipe1[1]; // [1] means writting end
        fPipeRecv = pipe2[0]; // [0] means reading end

        // set non-block
        try {
            ret = ::fcntl(fPipeRecv, F_SETFL, ::fcntl(fPipeRecv, F_GETFL) | O_NONBLOCK);
        } catch (...) {
            ret = -1;
            fail("failed to set pipe as non-block");
        }

        //----------------------------------------------------------------
        // wait a while for child process to confirm it is alive

        if (ret != -1)
        {
            char ch;
            ssize_t ret2;

            for (int i=0; ;)
            {
                try {
                    ret2 = ::read(fPipeRecv, &ch, 1);
                }
                catch (...) {
                    fail("failed to read from pipe");
                    break;
                }

                switch (ret2)
                {
                case -1:
                    if (errno == EAGAIN)
                    {
                        if (i < WAIT_START_TIMEOUT / WAIT_STEP)
                        {
                            carla_msleep(WAIT_STEP);
                            ++i;
                            continue;
                        }
                        carla_stderr("we have waited for child with pid %d to appear for %.1f seconds and we are giving up", (int)fPid, (float)WAIT_START_TIMEOUT / 1000.0f);
                    }
                    else
                        carla_stderr("read() failed: %s", std::strerror(errno));
                    break;

                case 1:
                    if (ch == '\n') {
                        // success
                        return true;
                    }
                    carla_stderr("read() has wrong first char '%c'", ch);
                    break;

                default:
                    carla_stderr("read() returned %i", int(ret2));
                    break;
                }

                break;
            }
        }

        carla_stderr("force killing misbehaved child %i (start)", int(fPid));

        if (kill(fPid, SIGKILL) == -1)
        {
            carla_stderr("kill() failed: %s (start)\n", std::strerror(errno));
        }

        // wait a while child to exit, we dont like zombie processes
        wait_child(fPid);

        // close pipes
        try {
            ::close(fPipeRecv);
        } catch (...) {}

        try {
            ::close(fPipeSend);
        } catch (...) {}

        fPipeRecv = -1;
        fPipeSend = -1;
        fPid = -1;

        return false;
    }

    void stop() noexcept
    {
        carla_debug("CarlaPipeServer::stop()");

        if (fPipeSend == -1 || fPipeRecv == -1 || fPid == -1)
            return;

        const CarlaMutexLocker cml(fWriteLock);

        try {
            ssize_t ignore = ::write(fPipeSend, "quit\n", 5);
            (void)ignore;
        } CARLA_SAFE_EXCEPTION("CarlaPipeServer::stop");

        waitChildClose();

        try {
            ::close(fPipeRecv);
        } catch (...) {}

        try {
            ::close(fPipeSend);
        } catch (...) {}

        fPipeRecv = -1;
        fPipeSend = -1;
        fPid = -1;
    }

    void idle()
    {
        const char* locale = nullptr;

        for (;;)
        {
            const char* const msg(readline());

            if (msg == nullptr)
                break;

            if (locale == nullptr)
            {
                locale = carla_strdup(setlocale(LC_NUMERIC, nullptr));
                setlocale(LC_NUMERIC, "POSIX");
            }

            fIsReading = true;
            msgReceived(msg);
            fIsReading = false;

            delete[] msg;
        }

        if (locale != nullptr)
        {
            setlocale(LC_NUMERIC, locale);
            delete[] locale;
        }
    }

    // -------------------------------------------------------------------

    bool readNextLineAsBool(bool& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
        {
            value = (std::strcmp(msg, "true") == 0);
            delete[] msg;
            return true;
        }

        return false;
    }

    bool readNextLineAsInt(int32_t& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
        {
            value = std::atoi(msg);
            delete[] msg;
            return true;
        }

        return false;
    }

    bool readNextLineAsUInt(uint32_t& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
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

    bool readNextLineAsLong(int64_t& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
        {
            value = std::atol(msg);
            delete[] msg;
            return true;
        }

        return false;
    }

    bool readNextLineAsULong(uint64_t& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
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

    bool readNextLineAsFloat(float& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
        {
            const bool ret(std::sscanf(msg, "%f", &value) == 1);
            delete[] msg;
            return ret;
        }

        return false;
    }

    bool readNextLineAsString(const char*& value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsReading, false);

        if (const char* const msg = readline())
        {
            value = msg;
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------
    // must be locked before calling

    void writeMsg(const char* const msg) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);
        CARLA_SAFE_ASSERT(! fWriteLock.tryLock());

        try {
            ssize_t ignore = ::write(fPipeSend, msg, std::strlen(msg));
            (void)ignore;
        } CARLA_SAFE_EXCEPTION("CarlaPipeServer::writeMsg");
    }

    void writeMsg(const char* const msg, size_t size) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);
        CARLA_SAFE_ASSERT(! fWriteLock.tryLock());

        try {
            ssize_t ignore = ::write(fPipeSend, msg, size);
            (void)ignore;
        } CARLA_SAFE_EXCEPTION("CarlaPipeServer::writeMsg");
    }

    void writeAndFixMsg(const char* const msg) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);
        CARLA_SAFE_ASSERT(! fWriteLock.tryLock());

        const size_t size(msg != nullptr ? std::strlen(msg) : 0);

        char fixedMsg[size+2];

        if (size > 0)
        {
            std::strcpy(fixedMsg, msg);

            for (size_t i=0; i < size; ++i)
            {
                if (fixedMsg[i] == '\n')
                    fixedMsg[i] = '\r';
            }

            if (fixedMsg[size-1] == '\r')
            {
                fixedMsg[size-1] = '\n';
                fixedMsg[size]   = '\0';
                fixedMsg[size+1] = '\0';
            }
            else
            {
                fixedMsg[size]   = '\n';
                fixedMsg[size+1] = '\0';
            }
        }
        else
        {
            fixedMsg[0] = '\n';
            fixedMsg[1] = '\0';
        }

        try {
            ssize_t ignore = ::write(fPipeSend, fixedMsg, size+1);
            (void)ignore;
        } CARLA_SAFE_EXCEPTION("CarlaPipeServer::writeAndFixMsg");
    }

    // -------------------------------------------------------------------

    void waitChildClose() noexcept
    {
        if (! wait_child(fPid))
        {
            carla_stderr2("force killing misbehaved child %i (exit)", int(fPid));

            if (kill(fPid, SIGKILL) == -1)
                carla_stderr2("kill() failed: %s (exit)", std::strerror(errno));
            else
                wait_child(fPid);
        }
    }

    // -------------------------------------------------------------------

protected:
    // common write lock
    CarlaMutex fWriteLock;

    // to possibly send errors somewhere
    virtual void fail(const char* const error)
    {
        carla_stderr2(error);
    }

    // returns true if msg handled
    virtual bool msgReceived(const char* const msg) noexcept = 0;

    // -------------------------------------------------------------------

private:
    int   fPipeRecv; // the pipe end that is used for receiving messages from UI
    int   fPipeSend; // the pipe end that is used for sending messages to UI
    bool  fIsReading;
    pid_t fPid;

    char        fTmpBuf[0xff+1];
    CarlaString fTmpStr;

    // -------------------------------------------------------------------

    const char* readline() noexcept
    {
        char    ch;
        char*   ptr = fTmpBuf;
        ssize_t ret;

        fTmpStr.clear();

        for (int i=0; i < 0xff; ++i)
        {
            try {
                ret = ::read(fPipeRecv, &ch, 1);
            }
            catch (...) {
                break;
            }

            if (ret == 1 && ch != '\n')
            {
                if (ch == '\r')
                    ch = '\n';

                *ptr++ = ch;

                if (i+1 == 0xff)
                {
                    i = 0;
                    ptr = fTmpBuf;
                    fTmpStr += fTmpBuf;
                }

                continue;
            }

            if (fTmpStr.isNotEmpty() || ptr != fTmpBuf)
            {
                if (ptr != fTmpBuf)
                {
                    *ptr = '\0';
                    fTmpStr += fTmpBuf;
                }

                try {
                    return fTmpStr.dup();
                }
                catch(...) {
                    return nullptr;
                }
            }

            break;
        }

        return nullptr;
    }

    // -------------------------------------------------------------------

    static bool fork_exec(const char* const argv[7], int* const retp) noexcept
    {
        const pid_t ret = *retp = vfork();

        switch (ret)
        {
        case 0: // child process
            execvp(argv[0], const_cast<char* const*>(argv));

            carla_stderr2("exec failed: %s", std::strerror(errno));
            _exit(0); // this is not noexcept safe but doesn't matter anyway
            return false;

        case -1: // error
            carla_stderr2("fork() failed: %s", std::strerror(errno));
            _exit(0); // this is not noexcept safe but doesn't matter anyway
            return false;
        }

        return true;
    }

    static bool wait_child(const pid_t pid) noexcept
    {
        if (pid == -1)
        {
            carla_stderr2("Can't wait for pid -1");
            return false;
        }

        pid_t ret;

        for (int i=0, maxTime=WAIT_ZOMBIE_TIMEOUT/WAIT_STEP; i < maxTime; ++i)
        {
            try {
                ret = ::waitpid(pid, nullptr, WNOHANG);
            }
            catch(...) {
                break;
            }

            if (ret != 0)
            {
                if (ret == pid)
                    return true;

                if (ret == -1)
                {
                    if (errno == ECHILD)
                        return true;

                    carla_stderr2("waitpid(%i) failed: %s", int(pid), std::strerror(errno));
                    return false;
                }

                carla_stderr2("we waited for child pid %i to exit but we got pid %i instead", int(pid), int(ret));
                return false;
            }

            carla_msleep(WAIT_STEP); /* wait 100 ms */
        }

        carla_stderr2("we waited for child with pid %i to exit for %.1f seconds and we are giving up", int(pid), float(WAIT_START_TIMEOUT)/1000.0f);
        return false;
    }
};

// -----------------------------------------------------------------------

#endif // CARLA_PIPE_UTILS_HPP_INCLUDED
