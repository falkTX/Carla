/*
 * PatchBay Canvas Themes
 * Copyright (C) 2010-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PATCHBAY_THEME_HPP_INCLUDED
#define CARLA_PATCHBAY_THEME_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaJuceUtils.hpp"

//---------------------------------------------------------------------------------------------------------------------

class Theme
{
public:
    enum PortType {
        THEME_PORT_SQUARE,
        THEME_PORT_POLYGON
    };

    enum List {
        THEME_MODERN_DARK,
        THEME_MODERN_DARK_TINY,
        THEME_MODERN_LIGHT,
        THEME_CLASSIC_DARK,
        THEME_OOSTUDIO,
        THEME_MAX
    };

    enum BackgroundType {
        THEME_BG_SOLID,
        THEME_BG_GRADIENT
    };

    Theme(Theme::List idx);
    ~Theme();

private:
    struct PrivateData;
    PrivateData* const self;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Theme)
};

//---------------------------------------------------------------------------------------------------------------------

Theme::List getDefaultTheme();
const char* getThemeName(Theme::List idx);
const char* getDefaultThemeName();

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_PATCHBAY_THEME_HPP_INCLUDED
