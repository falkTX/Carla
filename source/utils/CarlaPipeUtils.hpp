/*
 * Carla Pipe utils based on lv2fil UI code
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#define WAIT_START_TIMEOUT  3000 /* ms */
#define WAIT_ZOMBIE_TIMEOUT 3000 /* ms */
#define WAIT_STEP 100            /* ms */

#include "CarlaUtils.hpp"
#include "CarlaString.hpp"

#include <fcntl.h>
#include <sys/wait.h>

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
    }

    // -------------------------------------------------------------------

public:
    virtual ~CarlaPipeServer()
    {
        carla_debug("CarlaPipeServer::~CarlaPipeServer()");

        stop();
    }

    void start(const char* const filename, const char* const arg1, const char* const arg2)
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(arg1 != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(arg2 != nullptr,)
        carla_debug("CarlaPipeServer::start(\"%s\", \"%s\", \"%s\"", filename, arg1, arg2);

        //----------------------------------------------------------------

        const char* argv[6];

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

        if (pipe(pipe1) != 0)
        {
            fail("pipe1 creation failed");
            return;
        }

        if (pipe(pipe2) != 0)
        {
            fail("pipe2 creation failed");
            return;
        }

        char uiPipeRecv[100+1];
        char uiPipeSend[100+1];

        std::snprintf(uiPipeRecv, 100, "%d", pipe1[0]); /* [0] means reading end */
        std::snprintf(uiPipeSend, 100, "%d", pipe2[1]); /* [1] means writting end */

        uiPipeRecv[100] = '\0';
        uiPipeSend[100] = '\0';

        argv[3] = uiPipeRecv; // reading end
        argv[4] = uiPipeSend; // writting end

        //----------------------------------------------------------------
        // argv[5] => NULL

        argv[5] = nullptr;

        //----------------------------------------------------------------
        // fork

        int ret = -1;

        if ((! fork_exec(argv, &ret)) || ret == -1)
        {
            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);
            fail("fork_exec() failed");
            return;
        }

        fPid = ret;

        /* fork duplicated the handles, close pipe ends that are used by the child process */
        close(pipe1[0]);
        close(pipe2[1]);

        fPipeSend = pipe1[1]; /* [1] means writting end */
        fPipeRecv = pipe2[0]; /* [0] means reading end */

        fcntl(fPipeRecv, F_SETFL, fcntl(fPipeRecv, F_GETFL) | O_NONBLOCK);

        //----------------------------------------------------------------
        // wait a while for child process to confirm it is alive

        char ch;

        for (int i=0; ;)
        {
            ret = read(fPipeRecv, &ch, 1);

            switch (ret)
            {
            case -1:
                if (errno == EAGAIN)
                {
                    if (i < WAIT_START_TIMEOUT / WAIT_STEP)
                    {
                        carla_msleep(WAIT_STEP);
                        i++;
                        continue;
                    }

                    carla_stderr("we have waited for child with pid %d to appear for %.1f seconds and we are giving up", (int)fPid, (float)WAIT_START_TIMEOUT / 1000.0f);
                }
                else
                    carla_stderr("read() failed: %s", strerror(errno));
                break;

            case 1:
                if (ch == '\n')
                    // success
                    return;

                carla_stderr("read() wrong first char '%c'", ch);
                break;

            default:
                carla_stderr("read() returned %d", ret);
                break;
            }

            break;
        }

        carla_stderr("force killing misbehaved child %d (start)", (int)fPid);

        if (kill(fPid, SIGKILL) == -1)
        {
            carla_stderr("kill() failed: %s (start)\n", strerror(errno));
        }

        /* wait a while child to exit, we dont like zombie processes */
        wait_child(fPid);
    }

    void stop()
    {
        carla_debug("CarlaPipeServer::stop()");

        if (fPipeSend == -1 || fPipeRecv == -1 || fPid == -1)
            return;

        write(fPipeSend, "quit\n", 5);

        waitChildClose();

        close(fPipeRecv);
        close(fPipeSend);
        fPipeRecv = -1;
        fPipeSend = -1;
        fPid = -1;
    }

    void idle()
    {
        char* locale = nullptr;

        for (;;)
        {
            char* const msg = readline();

            if (msg == nullptr)
                break;

            if (locale == nullptr)
            {
                locale = strdup(setlocale(LC_NUMERIC, nullptr));
                setlocale(LC_NUMERIC, "POSIX");
            }

            fReading = true;
            msgReceived(msg);
            fReading = false;

            std::free(msg);
        }

        if (locale != nullptr)
        {
            setlocale(LC_NUMERIC, locale);
            std::free(locale);
        }
    }

    // -------------------------------------------------------------------

    bool readNextLineAsBool(bool& value)
    {
        if (! fReading)
            return false;

        if (char* const msg = readline())
        {
            value = (std::strcmp(msg, "true") == 0);
            std::free(msg);
            return true;
        }

        return false;
    }

    bool readNextLineAsInt(int& value)
    {
        if (! fReading)
            return false;

        if (char* const msg = readline())
        {
            value = atoi(msg);
            std::free(msg);
            return true;
        }

        return false;
    }

    bool readNextLineAsFloat(float& value)
    {
        if (! fReading)
            return false;

        if (char* const msg = readline())
        {
            bool ret = (sscanf(msg, "%f", &value) == 1);
            std::free(msg);
            return ret;
        }

        return false;
    }

    bool readNextLineAsString(char*& value)
    {
        if (! fReading)
            return false;

        if (char* const msg = readline())
        {
            value = msg;
            return true;
        }

        return false;
    }

    void writeMsg(const char* const msg)
    {
        ::write(fPipeSend, msg, std::strlen(msg));
    }

    void writeMsg(const char* const msg, size_t size)
    {
        ::write(fPipeSend, msg, size);
    }

    void writeAndFixMsg(const char* const msg)
    {
        const size_t size = std::strlen(msg);

        char smsg[size+1];
        std::strcpy(smsg, msg);

        smsg[size-1] = '\n';
        smsg[size]   = '\0';

        for (size_t i=0; i<size; ++i)
        {
            if (smsg[i] == '\n')
                smsg[i] = '\r';
        }

        ::write(fPipeSend, smsg, size+1);
    }

    void waitChildClose()
    {
        if (! wait_child(fPid))
        {
            carla_stderr2("force killing misbehaved child %d (exit)", (int)fPid);

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
    CarlaString fTempBuf;

    // -------------------------------------------------------------------

    char* readline()
    {
        char ch;
        ssize_t ret;

        char buf[0xff+1];
        char* ptr = buf;

        fTempBuf.clear();
        buf[0xff] = '\0';

        for (int i=0;; ++i)
        {
            ret = read(fPipeRecv, &ch, 1);

            if (ret == 1 && ch != '\n')
            {
                if (ch == '\r')
                    ch = '\n';

                *ptr++ = ch;

                if (i+1 == 0xff)
                {
                    i = 0;
                    ptr = buf;
                    fTempBuf += buf;
                }

                continue;
            }

            if (fTempBuf.isNotEmpty() || ptr != buf)
            {
                if (ptr != buf)
                {
                    *ptr = '\0';
                    fTempBuf += buf;
                }

                return strdup((const char*)fTempBuf);
            }

            break;
        }

        return nullptr;
    }

    static bool fork_exec(const char* const argv[6], int* const retp)
    {
        pid_t ret = *retp = vfork();

        switch (ret)
        {
        case 0: /* child process */
            execvp(argv[0], (char* const*)argv);
            carla_stderr2("exec of UI failed: %s", strerror(errno));
            return false;
        case -1:
            carla_stderr2("fork() failed to create new process for plugin UI");
            return false;
        }

        return true;
    }

    static bool wait_child(const pid_t pid)
    {
        pid_t ret;
        int i;

        if (pid == -1)
        {
            carla_stderr2("Can't wait for pid -1");
            return false;
        }

        for (i = 0; i < WAIT_ZOMBIE_TIMEOUT / WAIT_STEP; ++i)
        {
            ret = waitpid(pid, NULL, WNOHANG);

            if (ret != 0)
            {
                if (ret == pid)
                  return true;

                if (ret == -1)
                {
                  carla_stderr2("waitpid(%d) failed: %s", (int)pid, strerror(errno));
                  return false;
                }

                carla_stderr2("we have waited for child pid %d to exit but we got pid %d instead", (int)pid, (int)ret);

                return false;
            }

            carla_msleep(WAIT_STEP); /* wait 100 ms */
        }

        carla_stderr2("we have waited for child with pid %d to exit for %.1f seconds and we are giving up", (int)pid, (float)WAIT_START_TIMEOUT / 1000.0f);
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
