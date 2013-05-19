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

#include "DistrhoUIInternal.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// External UI

ExternalUI::ExternalUI()
    : UI(),
      loServer(nullptr)
{
    //loServer = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler);

    //if (loServer != nullptr)
        //lo_server_add_method(loServer, nullptr, nullptr, osc_message_handler, this);
}

ExternalUI::~ExternalUI()
{
    if (loServer == nullptr)
        return;

    lo_server_del_method(loServer, nullptr, nullptr);
    lo_server_free(loServer);
}

// -------------------------------------------------
// DSP Callbacks

void ExternalUI::d_parameterChanged(uint32_t index, float value) override
{
    // TODO
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void ExternalUI::d_programChanged(uint32_t index) override
{
    // TODO
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void ExternalUI::d_stateChanged(const char* key, const char* value) override
{
    // TODO
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
void ExternalUI::d_noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) override
{
    // TODO
}
#endif

// -------------------------------------------------
// UI Callbacks

void ExternalUI::d_uiIdle() override
{
    if (loServer == nullptr)
        return;

    while (lo_server_recv_noblock(loServer, 0) != 0) {}
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
