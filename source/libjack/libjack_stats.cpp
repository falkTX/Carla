/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2017 Filipe Coelho <falktx@falktx.com>
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

#include "libjack.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
float jack_get_max_delayed_usecs(jack_client_t*)
{
    return 0.0f;
}

CARLA_EXPORT
float jack_get_xrun_delayed_usecs(jack_client_t*)
{
    return 0.0f;
}

CARLA_EXPORT
void jack_reset_max_delayed_usecs(jack_client_t*)
{
}

// --------------------------------------------------------------------------------------------------------------------
