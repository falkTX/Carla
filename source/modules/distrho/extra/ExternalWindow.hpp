/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
#define DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED

#include "String.hpp"

#ifdef DISTRHO_OS_UNIX
# include <cerrno>
# include <signal.h>
# include <sys/wait.h>
# include <unistd.h>
#else
# error Unsupported platform!
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// ExternalWindow class

class ExternalWindow
{
public:
    ExternalWindow(const uint w = 1, const uint h = 1, const char* const t = "")
        : width(w),
          height(h),
          title(t),
          pid(0) {}

    virtual ~ExternalWindow()
    {
        terminateAndWaitForProcess();
    }

    uint getWidth() const noexcept
    {
        return width;
    }

    uint getHeight() const noexcept
    {
        return height;
    }

    const char* getTitle() const noexcept
    {
        return title;
    }

    void setTitle(const char* const t) noexcept
    {
        title = t;
    }

    bool isRunning() noexcept
    {
        if (pid <= 0)
            return false;

        const pid_t p = ::waitpid(pid, nullptr, WNOHANG);

        if (p == pid || (p == -1 && errno == ECHILD))
        {
            printf("NOTICE: Child process exited while idle\n");
            pid = 0;
            return false;
        }

        return true;
    }

protected:
    bool startExternalProcess(const char* args[])
    {
        terminateAndWaitForProcess();

        pid = vfork();

        switch (pid)
        {
        case 0:
            execvp(args[0], (char**)args);
            _exit(1);
            return false;

        case -1:
            printf("Could not start external ui\n");
            return false;

        default:
            return true;
        }
    }

private:
    uint width;
    uint height;
    String title;
    pid_t pid;

    friend class UIExporter;

    void terminateAndWaitForProcess()
    {
        if (pid <= 0)
            return;

        printf("Waiting for previous process to stop,,,\n");

        bool sendTerm = true;

        for (pid_t p;;)
        {
            p = ::waitpid(pid, nullptr, WNOHANG);

            switch (p)
            {
            case 0:
                if (sendTerm)
                {
                    sendTerm = false;
                    ::kill(pid, SIGTERM);
                }
                break;

            case -1:
                if (errno == ECHILD)
                {
                    printf("Done! (no such process)\n");
                    pid = 0;
                    return;
                }
                break;

            default:
                if (p == pid)
                {
                    printf("Done! (clean wait)\n");
                    pid = 0;
                    return;
                }
                break;
            }

            // 5 msec
            usleep(5*1000);
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ExternalWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
