/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_EXTERNAL_HPP_INCLUDED
#define DISTRHO_UI_EXTERNAL_HPP_INCLUDED

#include "DistrhoUI.hpp"

//#include <lo/lo.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// External UI

class ExternalUI : public UI
{
public:
    ExternalUI();
    virtual ~ExternalUI() override;

protected:
    // -------------------------------------------------------------------
    // Information (External)

    virtual const char* d_getExternalFilename() const = 0;

private:
    // -------------------------------------------------------------------
    // Implement stuff not needed for external UIs

    unsigned int d_getWidth()  const noexcept override { return 0; }
    unsigned int d_getHeight() const noexcept override { return 0; }

    void d_parameterChanged(uint32_t, float) override {}
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void d_programChanged(uint32_t) override {}
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    void d_stateChanged(const char*, const char*) override {}
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    void d_noteReceived(bool, uint8_t, uint8_t, uint8_t) override {}
#endif

    void d_uiIdle() override {}

    friend class UIInternal;
};

// -----------------------------------------------------------------------


END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_EXTERNAL_HPP_INCLUDED
