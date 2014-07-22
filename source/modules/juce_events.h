/*
 * Carla Juce setup
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_JUCE_EVENTS_H_INCLUDED
#define CARLA_JUCE_EVENTS_H_INCLUDED

#if ! (JUCE_MAC || JUCE_WINDOWS)
# error Don't want juce_events with the current setup
#endif

#include "juce_core.h"

#include "juce_events/AppConfig.h"
#include "juce_events/juce_events.h"

#endif // CARLA_JUCE_EVENTS_H_INCLUDED
