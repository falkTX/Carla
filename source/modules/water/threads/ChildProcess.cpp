/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

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
#include "../streams/MemoryOutputStream.h"

#ifndef CARLA_OS_WIN
# include <signal.h>
# include <sys/wait.h>
#endif

#define JUCE_USE_VFORK 1

namespace water {

#ifdef CARLA_OS_WIN
//=====================================================================================================================
class ChildProcess::ActiveProcess
{
public:
    ActiveProcess (const String& command, int streamFlags)
        : ok (false), readPipe (0), writePipe (0)
    {
        SECURITY_ATTRIBUTES securityAtts;
        carla_zeroStruct(securityAtts);
        securityAtts.nLength = sizeof (securityAtts);
        securityAtts.bInheritHandle = TRUE;

        if (CreatePipe (&readPipe, &writePipe, &securityAtts, 0)
             && SetHandleInformation (readPipe, HANDLE_FLAG_INHERIT, 0))
        {
            STARTUPINFO startupInfo;
            carla_zeroStruct(startupInfo);
            startupInfo.cb = sizeof (startupInfo);

            startupInfo.hStdOutput = (streamFlags & wantStdOut) != 0 ? writePipe : 0;
            startupInfo.hStdError  = (streamFlags & wantStdErr) != 0 ? writePipe : 0;
            startupInfo.dwFlags = STARTF_USESTDHANDLES;

            ok = CreateProcess (nullptr, const_cast<LPSTR>(command.toRawUTF8()),
                                nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                                nullptr, nullptr, &startupInfo, &processInfo) != FALSE;
        }
    }

    ~ActiveProcess()
    {
        if (ok)
        {
            CloseHandle (processInfo.hThread);
            CloseHandle (processInfo.hProcess);
        }

        if (readPipe != 0)
            CloseHandle (readPipe);

        if (writePipe != 0)
            CloseHandle (writePipe);
    }

    bool isRunning() const noexcept
    {
        return WaitForSingleObject (processInfo.hProcess, 0) != WAIT_OBJECT_0;
    }

    int read (void* dest, int numNeeded) const noexcept
    {
        int total = 0;

        while (ok && numNeeded > 0)
        {
            DWORD available = 0;

            if (! PeekNamedPipe ((HANDLE) readPipe, nullptr, 0, nullptr, &available, nullptr))
                break;

            const int numToDo = jmin ((int) available, numNeeded);

            if (available == 0)
            {
                if (! isRunning())
                    break;

                Sleep(0);
            }
            else
            {
                DWORD numRead = 0;
                if (! ReadFile ((HANDLE) readPipe, dest, numToDo, &numRead, nullptr))
                    break;

                total += numRead;
                dest = addBytesToPointer (dest, numRead);
                numNeeded -= numRead;
            }
        }

        return total;
    }

    bool killProcess() const noexcept
    {
        return TerminateProcess (processInfo.hProcess, 0) != FALSE;
    }

    uint32 getExitCode() const noexcept
    {
        DWORD exitCode = 0;
        GetExitCodeProcess (processInfo.hProcess, &exitCode);
        return (uint32) exitCode;
    }

    int getPID() const noexcept
    {
        return 0;
    }

    bool ok;

private:
    HANDLE readPipe, writePipe;
    PROCESS_INFORMATION processInfo;

    CARLA_DECLARE_NON_COPY_CLASS (ActiveProcess)
};
#else
class ChildProcess::ActiveProcess
{
public:
    ActiveProcess (const StringArray& arguments, int streamFlags)
        : childPID (0), pipeHandle (0), readHandle (0)
    {
        String exe (arguments[0].unquoted());

        // Looks like you're trying to launch a non-existent exe or a folder (perhaps on OSX
        // you're trying to launch the .app folder rather than the actual binary inside it?)
        jassert (File::getCurrentWorkingDirectory().getChildFile (exe).existsAsFile()
                  || ! exe.containsChar (File::separator));

        int pipeHandles[2] = { 0 };

        if (pipe (pipeHandles) == 0)
        {
              Array<char*> argv;
              for (int i = 0; i < arguments.size(); ++i)
                  if (arguments[i].isNotEmpty())
                      argv.add (const_cast<char*> (arguments[i].toRawUTF8()));

              argv.add (nullptr);

#if JUCE_USE_VFORK
            const pid_t result = vfork();
#else
            const pid_t result = fork();
#endif

            if (result < 0)
            {
                close (pipeHandles[0]);
                close (pipeHandles[1]);
            }
            else if (result == 0)
            {
#if ! JUCE_USE_VFORK
                // we're the child process..
                close (pipeHandles[0]);   // close the read handle

                if ((streamFlags & wantStdOut) != 0)
                    dup2 (pipeHandles[1], STDOUT_FILENO); // turns the pipe into stdout
                else
                    dup2 (open ("/dev/null", O_WRONLY), STDOUT_FILENO);

                if ((streamFlags & wantStdErr) != 0)
                    dup2 (pipeHandles[1], STDERR_FILENO);
                else
                    dup2 (open ("/dev/null", O_WRONLY), STDERR_FILENO);

                close (pipeHandles[1]);
#endif

                if (execvp (exe.toRawUTF8(), argv.getRawDataPointer()))
                    _exit (-1);
            }
            else
            {
                // we're the parent process..
                childPID = result;
                pipeHandle = pipeHandles[0];
                close (pipeHandles[1]); // close the write handle
            }

            // FIXME
            (void)streamFlags;
        }
    }

    ~ActiveProcess()
    {
        if (readHandle != 0)
            fclose (readHandle);

        if (pipeHandle != 0)
            close (pipeHandle);
    }

    bool isRunning() const noexcept
    {
        if (childPID != 0)
        {
            int childState;
            const int pid = waitpid (childPID, &childState, WNOHANG);
            return pid == 0 || ! (WIFEXITED (childState) || WIFSIGNALED (childState));
        }

        return false;
    }

    int read (void* const dest, const int numBytes) noexcept
    {
        jassert (dest != nullptr);

        #ifdef fdopen
         #error // the zlib headers define this function as NULL!
        #endif

        if (readHandle == 0 && childPID != 0)
            readHandle = fdopen (pipeHandle, "r");

        if (readHandle != 0)
            return (int) fread (dest, 1, (size_t) numBytes, readHandle);

        return 0;
    }

    bool killProcess() const noexcept
    {
        return ::kill (childPID, SIGKILL) == 0;
    }

    uint32 getExitCode() const noexcept
    {
        if (childPID != 0)
        {
            int childState = 0;
            const int pid = waitpid (childPID, &childState, WNOHANG);

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
    int pipeHandle;
    FILE* readHandle;

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

int ChildProcess::readProcessOutput (void* dest, int numBytes)
{
    return activeProcess != nullptr ? activeProcess->read (dest, numBytes) : 0;
}

bool ChildProcess::kill()
{
    return activeProcess == nullptr || activeProcess->killProcess();
}

uint32 ChildProcess::getExitCode() const
{
    return activeProcess != nullptr ? activeProcess->getExitCode() : 0;
}

bool ChildProcess::waitForProcessToFinish (const int timeoutMs) const
{
    const uint32 timeoutTime = Time::getMillisecondCounter() + (uint32) timeoutMs;

    do
    {
        if (! isRunning())
            return true;
    }
    while (timeoutMs < 0 || Time::getMillisecondCounter() < timeoutTime);

    return false;
}

String ChildProcess::readAllProcessOutput()
{
    MemoryOutputStream result;

    for (;;)
    {
        char buffer [512];
        const int num = readProcessOutput (buffer, sizeof (buffer));

        if (num <= 0)
            break;

        result.write (buffer, (size_t) num);
    }

    return result.toString();
}

uint32 ChildProcess::getPID() const noexcept
{
    return activeProcess != nullptr ? activeProcess->getPID() : 0;
}

//=====================================================================================================================

#ifdef CARLA_OS_WIN
bool ChildProcess::start (const String& command, int streamFlags)
{
    activeProcess = new ActiveProcess (command, streamFlags);

    if (! activeProcess->ok)
        activeProcess = nullptr;

    return activeProcess != nullptr;
}

bool ChildProcess::start (const StringArray& args, int streamFlags)
{
    String escaped;

    for (int i = 0; i < args.size(); ++i)
    {
        String arg (args[i]);

#if 0 // FIXME
        // If there are spaces, surround it with quotes. If there are quotes,
        // replace them with \" so that CommandLineToArgv will correctly parse them.
        if (arg.containsAnyOf ("\" "))
            arg = arg.replace ("\"", "\\\"").quoted();
#endif

        escaped << arg << ' ';
    }

    return start (escaped.trim(), streamFlags);
}
#else
bool ChildProcess::start (const String& command, int streamFlags)
{
    return start (StringArray::fromTokens (command, true), streamFlags);
}

bool ChildProcess::start (const StringArray& args, int streamFlags)
{
    if (args.size() == 0)
        return false;

    activeProcess = new ActiveProcess (args, streamFlags);

    if (activeProcess->childPID == 0)
        activeProcess = nullptr;

    return activeProcess != nullptr;
}
#endif

}
