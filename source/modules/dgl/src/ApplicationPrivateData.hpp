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

#ifndef DGL_APP_PRIVATE_DATA_HPP_INCLUDED
#define DGL_APP_PRIVATE_DATA_HPP_INCLUDED

#include "../Application.hpp"
#include "../../distrho/extra/Sleep.hpp"

#include <list>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

struct Application::PrivateData {
    bool doLoop;
    uint visibleWindows;
    std::list<Window*> windows;
    std::list<IdleCallback*> idleCallbacks;

    PrivateData()
        : doLoop(true),
          visibleWindows(0),
          windows(),
          idleCallbacks() {}

    ~PrivateData()
    {
        DISTRHO_SAFE_ASSERT(! doLoop);
        DISTRHO_SAFE_ASSERT(visibleWindows == 0);

        windows.clear();
        idleCallbacks.clear();
    }

    void oneShown() noexcept
    {
        if (++visibleWindows == 1)
            doLoop = true;
    }

    void oneHidden() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(visibleWindows > 0,);

        if (--visibleWindows == 0)
            doLoop = false;
    }

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
