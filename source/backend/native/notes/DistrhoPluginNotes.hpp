/*
 * DISTRHO Notes Plugin
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_PLUGIN_NOTES_HPP_INCLUDED
#define DISTRHO_PLUGIN_NOTES_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoPluginNotes : public Plugin
{
public:
    DistrhoPluginNotes();
    ~DistrhoPluginNotes() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "Notes";
    }

    const char* d_getMaker() const noexcept override
    {
        return "DISTRHO";
    }

    const char* d_getLicense() const noexcept override
    {
        return "GPL v2+";
    }

    uint32_t d_getVersion() const noexcept override
    {
        return 0x1000;
    }

    long d_getUniqueId() const noexcept override
    {
        return d_cconst('D', 'N', 'o', 't');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) override;
    void d_initStateKey(uint32_t index, d_string& stateKeyName) override;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;
    void  d_setState(const char* key, const char* value) override;

    // -------------------------------------------------------------------
    // Process

    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents) override;

    // -------------------------------------------------------------------

private:
    int fCurPage;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_NOTES_HPP_INCLUDED
