/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2020 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#include "ChildProcess.h"
#include "../files/File.h"
#include "../misc/Time.h"

#ifndef CARLA_OS_WIN
# include <signal.h>
# include <sys/wait.h>
#endif

#include "CarlaProcessUtils.hpp"

namespace water {

#ifdef CARLA_OS_WIN
//=====================================================================================================================
class ChildProcess::ActiveProcess
{
public:
    ActiveProcess (const String& command)
        : ok (false)
    {
        STARTUPINFO startupInfo;
        carla_zeroStruct(startupInfo);
        startupInfo.cb = sizeof (startupInfo);

        ok = CreateProcess (nullptr, const_cast<LPSTR>(command.toRawUTF8()),
                            nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                            nullptr, nullptr, &startupInfo, &processInfo) != FALSE;
    }

    ~ActiveProcess()
    {
        closeProcessInfo();
    }

    void closeProcessInfo() noexcept
    {
        if (ok)
        {
            ok = false;
            CloseHandle (processInfo.hThread);
            CloseHandle (processInfo.hProcess);
        }
    }

    bool isRunning() const noexcept
    {
        return WaitForSingleObject (processInfo.hProcess, 0) != WAIT_OBJECT_0;
    }

    bool checkRunningAndUnsetPID() noexcept
    {
        if (isRunning())
            return true;

        ok = false;
        CloseHandle (processInfo.hThread);
        CloseHandle (processInfo.hProcess);
        return false;
    }

    bool killProcess() const noexcept
    {
        return TerminateProcess (processInfo.hProcess, 0) != FALSE;
    }

    bool terminateProcess() const noexcept
    {
        return TerminateProcess (processInfo.hProcess, 0) != FALSE;
    }

    uint32 getExitCodeAndClearPID() noexcept
    {
        DWORD exitCode = 0;
        GetExitCodeProcess (processInfo.hProcess, &exitCode);
        closeProcessInfo();
        return (uint32) exitCode;
    }

    int getPID() const noexcept
    {
        return 0;
    }

    bool ok;

private:
    PROCESS_INFORMATION processInfo;

    CARLA_DECLARE_NON_COPY_CLASS (ActiveProcess)
};
#else
class ChildProcess::ActiveProcess
{
public:
    ActiveProcess (const StringArray& arguments)
        : childPID (0)
    {
        String exe (arguments[0].unquoted());

        // Looks like you're trying to launch a non-existent exe or a folder (perhaps on OSX
        // you're trying to launch the .app folder rather than the actual binary inside it?)
        wassert (File::getCurrentWorkingDirectory().getChildFile (exe).existsAsFile()
                  || ! exe.containsChar (File::separator));

        Array<char*> argv;
        for (int i = 0; i < arguments.size(); ++i)
            if (arguments[i].isNotEmpty())
                argv.add (const_cast<char*> (arguments[i].toRawUTF8()));

        argv.add (nullptr);

        const pid_t result = vfork();

        if (result < 0)
        {
            // error
        }
        else if (result == 0)
        {
            // child process
            carla_terminateProcessOnParentExit(true);

            if (execvp (exe.toRawUTF8(), argv.getRawDataPointer()))
                _exit (-1);
        }
        else
        {
            // we're the parent process..
            childPID = result;
        }
    }

    ~ActiveProcess()
    {
        CARLA_SAFE_ASSERT_INT(childPID == 0, childPID);
    }

    bool isRunning() const noexcept
    {
        if (childPID != 0)
        {
            int childState = 0;
            const int pid = waitpid (childPID, &childState, WNOHANG|WUNTRACED);
            return pid == 0 || ! (WIFEXITED (childState) || WIFSIGNALED (childState) || WIFSTOPPED (childState));
        }

        return false;
    }

    bool checkRunningAndUnsetPID() noexcept
    {
        if (childPID != 0)
        {
            int childState = 0;
            const int pid = waitpid (childPID, &childState, WNOHANG|WUNTRACED);
            if (pid == 0)
                return true;
            if ( ! (WIFEXITED (childState) || WIFSIGNALED (childState) || WIFSTOPPED (childState)))
                return true;

            childPID = 0;
            return false;
        }

        return false;
    }

    bool killProcess() noexcept
    {
        if (::kill (childPID, SIGKILL) == 0)
        {
            childPID = 0;
            return true;
        }

        return false;
    }

    bool terminateProcess() const noexcept
    {
        return ::kill (childPID, SIGTERM) == 0;
    }

    uint32 getExitCodeAndClearPID() noexcept
    {
        if (childPID != 0)
        {
            int childState = 0;
            const int pid = waitpid (childPID, &childState, WNOHANG);
            childPID = 0;

            if (pid >= 0 && WIFEXITED (childState))
                return WEXITSTATUS (childState);
        }

        return 0;
    }

    int getPID() const noexcept
    {
        return childPID;
    }

    int childPID;

private:
    CARLA_DECLARE_NON_COPY_CLASS (ActiveProcess)
};
#endif

//=====================================================================================================================

ChildProcess::ChildProcess() {}
ChildProcess::~ChildProcess() {}

bool ChildProcess::isRunning() const
{
    return activeProcess != nullptr && activeProcess->isRunning();
}

bool ChildProcess::kill()
{
    return activeProcess == nullptr || activeProcess->killProcess();
}

bool ChildProcess::terminate()
{
    return activeProcess == nullptr || activeProcess->terminateProcess();
}

uint32 ChildProcess::getExitCodeAndClearPID()
{
    return activeProcess != nullptr ? activeProcess->getExitCodeAndClearPID() : 0;
}

bool ChildProcess::waitForProcessToFinish (const int timeoutMs)
{
    const uint32 timeoutTime = Time::getMillisecondCounter() + (uint32) timeoutMs;

    do
    {
        if (activeProcess == nullptr)
            return true;
        if (! activeProcess->checkRunningAndUnsetPID())
            return true;

        carla_msleep(5);
    }
    while (timeoutMs < 0 || Time::getMillisecondCounter() < timeoutTime);

    return false;
}

uint32 ChildProcess::getPID() const noexcept
{
    return activeProcess != nullptr ? activeProcess->getPID() : 0;
}

//=====================================================================================================================

#ifdef CARLA_OS_WIN
bool ChildProcess::start (const String& command)
{
    activeProcess = new ActiveProcess (command);

    if (! activeProcess->ok)
        activeProcess = nullptr;

    return activeProcess != nullptr;
}

bool ChildProcess::start (const StringArray& args)
{
    String escaped;

    for (int i = 0, size = args.size(); i < size; ++i)
    {
        String arg (args[i]);

        // If there are spaces, surround it with quotes. If there are quotes,
        // replace them with \" so that CommandLineToArgv will correctly parse them.
        if (arg.containsAnyOf ("\" "))
            arg = arg.replace ("\"", "\\\"").quoted();

        escaped << arg;

        if (i+1 < size)
            escaped << ' ';
    }

    return start (escaped.trim());
}
#else
bool ChildProcess::start (const String& command)
{
    return start (StringArray::fromTokens (command, true));
}

bool ChildProcess::start (const StringArray& args)
{
    if (args.size() == 0)
        return false;

    activeProcess = new ActiveProcess (args);

    if (activeProcess->childPID == 0)
        activeProcess = nullptr;

    return activeProcess != nullptr;
}
#endif

}
