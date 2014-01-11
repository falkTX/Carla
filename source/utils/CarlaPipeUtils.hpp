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

#include "CarlaUtils.hpp"
#include "CarlaString.hpp"

#include <cerrno>
#include <clocale>

#include <fcntl.h>
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
          fPid(-1),
          fReading(false)
    {
        carla_debug("CarlaPipeServer::CarlaPipeServer()");

        fTmpBuf[0xff] = '\0';
    }

    // -------------------------------------------------------------------

public:
    virtual ~CarlaPipeServer()
    {
        carla_debug("CarlaPipeServer::~CarlaPipeServer()");

        stop();
    }

    bool start(const char* const filename, const char* const arg1, const char* const arg2)
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
        CARLA_SAFE_ASSERT_RETURN(arg1 != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(arg2 != nullptr, false);
        carla_debug("CarlaPipeServer::start(\"%s\", \"%s\", \"%s\")", filename, arg1, arg2);

        //----------------------------------------------------------------

        const char* argv[5];

        //----------------------------------------------------------------
        // argv[0] => filename

        argv[0] = filename;

        //----------------------------------------------------------------
        // argv[1-2] => args

        argv[1] = arg1;
        argv[2] = arg2;

        //----------------------------------------------------------------
        // argv[3-4] => pipes

        int pipe1[2]; // written by host process, read by plugin UI process
        int pipe2[2]; // written by plugin UI process, read by host process

        if (::pipe(pipe1) != 0)
        {
            fail("pipe1 creation failed");
            return false;
        }

        if (::pipe(pipe2) != 0)
        {
            ::close(pipe1[0]);
            ::close(pipe1[1]);
            fail("pipe2 creation failed");
            return false;
        }

        char uiPipeRecv[100+1];
        char uiPipeSend[100+1];

        std::snprintf(uiPipeRecv, 100, "%d", pipe1[0]); // [0] means reading end
        std::snprintf(uiPipeSend, 100, "%d", pipe2[1]); // [1] means writting end

        uiPipeRecv[100] = '\0';
        uiPipeSend[100] = '\0';

        argv[3] = uiPipeRecv; // reading end
        argv[4] = uiPipeSend; // writting end

        //----------------------------------------------------------------
        // fork

        int ret = -1;

        if ((! fork_exec(argv, &ret)) || ret == -1)
        {
            ::close(pipe1[0]);
            ::close(pipe1[1]);
            ::close(pipe2[0]);
            ::close(pipe2[1]);
            fail("fork_exec() failed");
            return false;
        }

        fPid = ret;

        // fork duplicated the handles, close pipe ends that are used by the child process
        ::close(pipe1[0]);
        ::close(pipe2[1]);

        fPipeSend = pipe1[1]; // [1] means writting end
        fPipeRecv = pipe2[0]; // [0] means reading end

        // set non-block
        try {
            ret = fcntl(fPipeRecv, F_SETFL, fcntl(fPipeRecv, F_GETFL) | O_NONBLOCK);
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

            carla_stderr("force killing misbehaved child %i (start)", int(fPid));
        }

        if (kill(fPid, SIGKILL) == -1)
        {
            carla_stderr("kill() failed: %s (start)\n", strerror(errno));
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

        try {
            ::write(fPipeSend, "quit\n", 5);
        } catch (...) {}

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

            fReading = true;
            msgReceived(msg);
            fReading = false;

            delete[] msg;
        }

        if (locale != nullptr)
        {
            setlocale(LC_NUMERIC, locale);
            delete[] locale;
        }
    }

    // -------------------------------------------------------------------

    bool readNextLineAsBool(bool& value)
    {
        CARLA_SAFE_ASSERT_RETURN(fReading, false);

        if (const char* const msg = readline())
        {
            value = (std::strcmp(msg, "true") == 0);
            delete[] msg;
            return true;
        }

        return false;
    }

    bool readNextLineAsInt(int& value)
    {
        CARLA_SAFE_ASSERT_RETURN(fReading, false);

        if (const char* const msg = readline())
        {
            value = std::atoi(msg);
            delete[] msg;
            return true;
        }

        return false;
    }

    bool readNextLineAsFloat(float& value)
    {
        CARLA_SAFE_ASSERT_RETURN(fReading, false);

        if (const char* const msg = readline())
        {
            const bool ret(std::sscanf(msg, "%f", &value) == 1);
            delete[] msg;
            return ret;
        }

        return false;
    }

    bool readNextLineAsString(const char*& value)
    {
        CARLA_SAFE_ASSERT_RETURN(fReading, false);

        if (const char* const msg = readline())
        {
            value = msg;
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

    void writeMsg(const char* const msg) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);

        try {
            ::write(fPipeSend, msg, std::strlen(msg));
        } catch (...) {}
    }

    void writeMsg(const char* const msg, size_t size) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);

        try {
            ::write(fPipeSend, msg, size);
        } catch (...) {}
    }

    void writeAndFixMsg(const char* const msg)
    {
        CARLA_SAFE_ASSERT_RETURN(fPipeSend != -1,);

        const size_t size(std::strlen(msg));

        char fixedMsg[size+1];
        std::strcpy(fixedMsg, msg);

        for (size_t i=0; i < size; ++i)
        {
            if (fixedMsg[i] == '\n')
                fixedMsg[i] = '\r';
        }

        fixedMsg[size-1] = '\n';
        fixedMsg[size]   = '\0';

        try {
            ::write(fPipeSend, fixedMsg, size);
        } catch (...) {}
    }

    void waitChildClose()
    {
        if (! wait_child(fPid))
        {
            carla_stderr2("force killing misbehaved child %i (exit)", int(fPid));

            if (kill(fPid, SIGKILL) == -1)
                carla_stderr2("kill() failed: %s (exit)", strerror(errno));
            else
                wait_child(fPid);
        }
    }

    // -------------------------------------------------------------------

protected:
    virtual void fail(const char* const error)
    {
        carla_stderr2(error);
    }

    virtual void msgReceived(const char* const msg) = 0;

    // -------------------------------------------------------------------

private:
    int fPipeRecv; // the pipe end that is used for receiving messages from UI
    int fPipeSend; // the pipe end that is used for sending messages to UI
    pid_t fPid;
    bool  fReading;

    char        fTmpBuf[0xff+1];
    CarlaString fTmpStr;

    // -------------------------------------------------------------------

    const char* readline()
    {
        char    ch;
        char*   ptr = fTmpBuf;
        ssize_t ret;

        fTmpStr.clear();

        for (int i=0; i < 0xff; ++i)
        {
            try {
                ret = read(fPipeRecv, &ch, 1);
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

                return fTmpStr.dup();
            }

            break;
        }

        return nullptr;
    }

    static bool fork_exec(const char* const argv[5], int* const retp)
    {
        const pid_t ret = *retp = vfork();

        switch (ret)
        {
        case 0: /* child process */
            execlp(argv[0], argv[0], argv[1], argv[2], argv[3], argv[4], nullptr);
            carla_stderr2("exec failed: %s", std::strerror(errno));
            _exit(0);
            return false;
        case -1: /* error */
            carla_stderr2("fork() failed: %s", std::strerror(errno));
            _exit(0);
            return false;
        }

        return true;
    }

    static bool wait_child(const pid_t pid)
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
            catch (...) {
                break;
            }

            if (ret != 0)
            {
                if (ret == pid)
                    return true;

                if (ret == -1)
                {
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

class CarlaPipeClient
{
protected:
    CarlaPipeClient()
    {
        carla_debug("CarlaPipeClient::CarlaPipeClient()");
    }

    // -------------------------------------------------------------------

public:
    virtual ~CarlaPipeClient()
    {
        carla_debug("CarlaPipeClient::~CarlaPipeClient()");

        stop();
    }

    void stop()
    {
    }
};

// -----------------------------------------------------------------------

#endif // CARLA_PIPE_UTILS_HPP_INCLUDED
