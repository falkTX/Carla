/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_UI_EXTERNAL_HPP__
#define __DISTRHO_UI_EXTERNAL_HPP__

#include "DistrhoUI.hpp"

#include <lo/lo.h>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// External UI

class ExternalUI : public UI
{
public:
    ExternalUI();
    virtual ~ExternalUI() override;

protected:
    virtual const char* d_externalFilename() const = 0;

private:
    friend class UIInternal;

    // ---------------------------------------------
    // not needed for external UIs

    unsigned int d_width() const override { return 0; }
    unsigned int d_height() const override { return 0; }
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
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_EXTERNAL_HPP__
