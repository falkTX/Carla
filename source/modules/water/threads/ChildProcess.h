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

#ifndef WATER_CHILDPROCESS_H_INCLUDED
#define WATER_CHILDPROCESS_H_INCLUDED

#include "../text/StringArray.h"

#include "CarlaJuceUtils.hpp"

namespace water {

//==============================================================================
/**
    Launches and monitors a child process.

    This class lets you launch an executable, and read its output. You can also
    use it to check whether the child process has finished.
*/
class ChildProcess
{
public:
    //==============================================================================
    /** Creates a process object.
        To actually launch the process, use start().
    */
    ChildProcess();

    /** Destructor.
        Note that deleting this object won't terminate the child process.
    */
    ~ChildProcess();

    /** These flags are used by the start() methods. */
    enum StreamFlags
    {
        wantStdOut = 1,
        wantStdErr = 2
    };

    /** Attempts to launch a child process command.

        The command should be the name of the executable file, followed by any arguments
        that are required.
        If the process has already been launched, this will launch it again. If a problem
        occurs, the method will return false.
        The streamFlags is a combinations of values to indicate which of the child's output
        streams should be read and returned by readProcessOutput().
    */
    bool start (const String& command, int streamFlags = wantStdOut | wantStdErr);

    /** Attempts to launch a child process command.

        The first argument should be the name of the executable file, followed by any other
        arguments that are needed.
        If the process has already been launched, this will launch it again. If a problem
        occurs, the method will return false.
        The streamFlags is a combinations of values to indicate which of the child's output
        streams should be read and returned by readProcessOutput().
    */
    bool start (const StringArray& arguments, int streamFlags = wantStdOut | wantStdErr);

    /** Returns true if the child process is alive. */
    bool isRunning() const;

    /** Attempts to read some output from the child process.
        This will attempt to read up to the given number of bytes of data from the
        process. It returns the number of bytes that were actually read.
    */
    int readProcessOutput (void* destBuffer, int numBytesToRead);

    /** Blocks until the process has finished, and then returns its complete output
        as a string.
    */
    String readAllProcessOutput();

    /** Blocks until the process is no longer running. */
    bool waitForProcessToFinish (int timeoutMs) const;

    /** If the process has finished, this returns its exit code. */
    uint32 getExitCode() const;

    /** Attempts to kill the child process.
        Returns true if it succeeded. Trying to read from the process after calling this may
        result in undefined behaviour.
    */
    bool kill();
    bool terminate();

    uint32 getPID() const noexcept;

private:
    //==============================================================================
    class ActiveProcess;
    ScopedPointer<ActiveProcess> activeProcess;

    CARLA_DECLARE_NON_COPY_CLASS (ChildProcess)
};

}

#endif // WATER_CHILDPROCESS_H_INCLUDED
