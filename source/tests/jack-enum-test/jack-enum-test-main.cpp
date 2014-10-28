/*
 * Carla Tests
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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
 */

#include <jack/jack.h>
#include <jack/transport.h>

extern "C" {

jack_transport_state_t jackbridge_transport_query(const jack_client_t*, jack_position_t*);

}

jack_transport_state_t jackbridge_transport_query(const jack_client_t*, jack_position_t*)
{
    return JackTransportStopped;
}

extern void testcall();

int main()
{
    testcall();
    return 0;
}
