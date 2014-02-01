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
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaUtils.hpp"

#include "rewire/ReWirePanelAPI.h"

int main(/*int argc, char* argv[]*/)
{
    ReWire::ReWireError result;

    result = ReWire::RWPOpen();

    if (result == ReWire::kReWireError_NoError)
    {
        carla_stdout("opened ok");
    }
    else
    {
        carla_stdout("opened failed");
        return 1;
    }

    result = ReWire::RWPClose();

    if (result == ReWire::kReWireError_NoError)
    {
        carla_stdout("closed ok");
    }
    else
    {
        carla_stdout("closed failed");
        return 1;
    }

    return 0;
}
