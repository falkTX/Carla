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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __DISTRHO_PLUGIN_NOTES_HPP__
#define __DISTRHO_PLUGIN_NOTES_HPP__

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class DistrhoPluginNotes : public Plugin
{
public:
    DistrhoPluginNotes();
    ~DistrhoPluginNotes();

protected:
    // ---------------------------------------------
    // Information

    const char* d_label() const
    {
        return "Notes";
    }

    const char* d_maker() const
    {
        return "DISTRHO";
    }

    const char* d_license() const
    {
        return "GPL v2+";
    }

    uint32_t d_version() const
    {
        return 0x1000;
    }

    long d_uniqueId() const
    {
        return d_cconst('D', 'N', 'o', 't');
    }

    // ---------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter);
    void d_initStateKey(uint32_t index, d_string& stateKeyName);

    // ---------------------------------------------
    // Internal data

    float d_parameterValue(uint32_t index);
    void  d_setParameterValue(uint32_t index, float value);
    void  d_setState(const char* key, const char* value);

    // ---------------------------------------------
    // Process

    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents);

    // ---------------------------------------------

private:
    int fCurPage;
};

END_NAMESPACE_DISTRHO

#endif  // __DISTRHO_PLUGIN_NOTES_HPP__
