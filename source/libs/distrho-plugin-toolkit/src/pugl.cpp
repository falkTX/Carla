/*
 * DISTRHO Plugin Toolkit (DPT)
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
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#if DISTRHO_OS_WINDOWS
# include "pugl/pugl_win.cpp"
#elif DISTRHO_OS_MAC
# include "pugl/pugl_osx.m"
#elif DISTRHO_OS_LINUX
extern "C" {
# include "pugl/pugl_x11.c"
}
#else
# error Unsupported platform!
#endif
