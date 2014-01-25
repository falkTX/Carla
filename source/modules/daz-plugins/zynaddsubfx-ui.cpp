/*
 * Carla Native Plugins
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

#include "CarlaString.hpp"

#define PLUGINVERSION
#define SOURCE_DIR "/usr/share/zynaddsubfx/examples"
#undef override

CarlaString gUiPixmapPath;

// zynaddsubfx ui includes
#include "zynaddsubfx/UI/NioUI.cpp"
#include "zynaddsubfx/UI/WidgetPDial.cpp"
#include "zynaddsubfx/UI/ADnoteUI.cpp"
#include "zynaddsubfx/UI/BankUI.cpp"
#include "zynaddsubfx/UI/ConfigUI.cpp"
#include "zynaddsubfx/UI/EffUI.cpp"
#include "zynaddsubfx/UI/EnvelopeUI.cpp"
#include "zynaddsubfx/UI/FilterUI.cpp"
#include "zynaddsubfx/UI/LFOUI.cpp"
#include "zynaddsubfx/UI/MasterUI.cpp"
#include "zynaddsubfx/UI/MicrotonalUI.cpp"
#include "zynaddsubfx/UI/OscilGenUI.cpp"
#include "zynaddsubfx/UI/PADnoteUI.cpp"
#include "zynaddsubfx/UI/PartUI.cpp"
#include "zynaddsubfx/UI/PresetsUI.cpp"
#include "zynaddsubfx/UI/ResonanceUI.cpp"
#include "zynaddsubfx/UI/SUBnoteUI.cpp"
#include "zynaddsubfx/UI/VirKeyboard.cpp"
