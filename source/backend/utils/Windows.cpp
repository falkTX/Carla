/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_MAC
# import <Cocoa/Cocoa.h>
#endif

namespace CB = CarlaBackend;

// -------------------------------------------------------------------------------------------------------------------

int carla_cocoa_get_window(void* nsViewPtr)
{
    CARLA_SAFE_ASSERT_RETURN(nsViewPtr != nullptr, 0);

#ifdef CARLA_OS_MAC
    NSView* const nsView = (NSView*)nsViewPtr;
    return [[nsView window] windowNumber];
#else
    return 0;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
