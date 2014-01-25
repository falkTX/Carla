/*
 * DISTRHO BigMeter Plugin
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_BIGMETER_HPP_INCLUDED
#define DISTRHO_UI_BIGMETER_HPP_INCLUDED

#include "DistrhoUIExternal.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoUIBigMeter : public ExternalUI
{
public:
    DistrhoUIBigMeter();

protected:
    // -------------------------------------------------------------------
    // Information (External)

    const char* d_getExternalFilename() const override
    {
        return "bigmeterM-ui";
    }
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_BIGMETER_HPP_INCLUDED
