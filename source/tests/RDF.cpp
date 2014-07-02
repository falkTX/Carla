/*
 * Carla LADSPA/LV2_RDF Tests
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

#include "ladspa_rdf.hpp"
#include "lv2_rdf.hpp"

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------
// main

int main()
{
    // ladspa
    {
        LADSPA_RDF_ScalePoint scalePoint1, scalePoint2;
        scalePoint2.Label = carla_strdup("");

        LADSPA_RDF_Port port1, port2, port3;
        port2.Label = carla_strdup("");
        port3.Label = carla_strdup("");
        port3.ScalePoints = new LADSPA_RDF_ScalePoint[2];
        port3.ScalePoints[0].Label = carla_strdup("");
        port3.ScalePoints[1].Label = carla_strdup("");

        LADSPA_RDF_Descriptor desc1, desc2, desc3;
        desc2.Title = carla_strdup("");
        desc3.Title = carla_strdup("");
        desc2.Creator = carla_strdup("");
        desc3.Creator = carla_strdup("");
        desc3.Ports = new LADSPA_RDF_Port[1];
        desc3.Ports[0].Label = carla_strdup("");
        desc3.Ports[0].ScalePoints = new LADSPA_RDF_ScalePoint[1];
        desc3.Ports[0].ScalePoints[0].Label = carla_strdup("");
    }

    return 0;
}

// -----------------------------------------------------------------------
