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

#ifdef NDEBUG
# error Build this file with debug ON please
#endif

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
        assert(port3.ScalePoints[0].Label == nullptr);
        port3.ScalePoints[0].Label = carla_strdup("");
        port3.ScalePoints[1].Label = carla_strdup("");

        LADSPA_RDF_Descriptor desc1, desc2, desc3;
        desc2.Title = carla_strdup("");
        desc3.Title = carla_strdup("");
        desc2.Creator = carla_strdup("");
        desc3.Creator = carla_strdup("");
        desc3.Ports = new LADSPA_RDF_Port[1];
        assert(desc3.Ports[0].Label == nullptr);
        desc3.Ports[0].Label = carla_strdup("");
        desc3.Ports[0].ScalePoints = new LADSPA_RDF_ScalePoint[1];
        desc3.Ports[0].ScalePoints[0].Label = carla_strdup("");
    }

    // lv2
    {
        LV2_RDF_PortMidiMap portMidiMap;
        LV2_RDF_PortPoints portPoints;

        LV2_RDF_PortUnit portUnit1, portUnit2;
        assert(portUnit1.Name == nullptr);
        portUnit2.Name   = carla_strdup("");
        portUnit2.Render = carla_strdup("");
        portUnit2.Symbol = carla_strdup("");

        LV2_RDF_PortScalePoint portScalePoint1, portScalePoint2;
        assert(portScalePoint1.Label == nullptr);
        portScalePoint2.Label = carla_strdup("");

        LV2_RDF_Port port1, port2, port3;
        assert(port1.Name == nullptr);
        port2.Name   = carla_strdup("");
        port3.Name   = carla_strdup("");
        port2.Symbol = carla_strdup("");
        port3.Symbol = carla_strdup("");
        port3.Unit.Name   = carla_strdup("");
        port3.Unit.Render = carla_strdup("");
        port3.Unit.Symbol = carla_strdup("");
        port3.ScalePoints = new LV2_RDF_PortScalePoint[2];
        assert(port3.ScalePoints[0].Label == nullptr);
        port3.ScalePoints[0].Label = carla_strdup("");
        port3.ScalePoints[1].Label = carla_strdup("");

        LV2_RDF_Preset preset1, preset2;
        assert(preset1.Label == nullptr);
        preset2.URI   = carla_strdup("");
        preset2.Label = carla_strdup("");

        LV2_RDF_Feature feat1, feat2;
        assert(feat1.URI == nullptr);
        feat2.URI = carla_strdup("");

        LV2_RDF_UI ui1, ui2, ui3;
        assert(ui1.Binary == nullptr);
        ui2.URI    = carla_strdup("");
        ui3.URI    = carla_strdup("");
        ui2.Binary = carla_strdup("");
        ui3.Binary = carla_strdup("");
        ui2.Bundle = carla_strdup("");
        ui3.Bundle = carla_strdup("");
        ui3.Features = new LV2_RDF_Feature[1];
        assert(ui3.Features[0].URI == nullptr);
        ui3.Features[0].URI = carla_strdup("");
        ui3.Extensions = new LV2_URI[3];
        ui3.Extensions[0] = carla_strdup("");
        ui3.Extensions[1] = nullptr;
        ui3.Extensions[2] = carla_strdup("");
        ui3.ExtensionCount = 3;

        LV2_RDF_Descriptor desc1, desc2, desc3;
        assert(desc1.URI == nullptr);
        desc2.URI     = carla_strdup("");
        desc3.URI     = carla_strdup("");
        desc2.Name    = carla_strdup("");
        desc3.Name    = carla_strdup("");
        desc2.Author  = carla_strdup("");
        desc3.Author  = carla_strdup("");
        desc2.License = carla_strdup("");
        desc3.License = carla_strdup("");
        desc2.Binary  = carla_strdup("");
        desc3.Binary  = carla_strdup("");
        desc2.Bundle  = carla_strdup("");
        desc3.Bundle  = carla_strdup("");
        desc3.Ports = new LV2_RDF_Port[1];
        assert(desc3.Ports[0].Name == nullptr);
        desc3.Ports[0].Name   = carla_strdup("");
        desc3.Ports[0].Symbol = carla_strdup("");
        desc3.Ports[0].Unit.Name   = carla_strdup("");
        desc3.Ports[0].Unit.Render = carla_strdup("");
        desc3.Ports[0].Unit.Symbol = carla_strdup("");
        desc3.Ports[0].ScalePoints = new LV2_RDF_PortScalePoint[1];
        assert(desc3.Ports[0].ScalePoints[0].Label == nullptr);
        desc3.Ports[0].ScalePoints[0].Label = carla_strdup("");
        desc3.Presets = new LV2_RDF_Preset[1];
        assert(desc3.Presets[0].URI == nullptr);
        desc3.Presets[0].URI   = carla_strdup("");
        desc3.Presets[0].Label = carla_strdup("");
        desc3.Features = new LV2_RDF_Feature[1];
        assert(desc3.Features[0].URI == nullptr);
        desc3.Features[0].URI = carla_strdup("");
        desc3.Extensions = new LV2_URI[3];
        desc3.Extensions[0] = carla_strdup("");
        desc3.Extensions[1] = nullptr;
        desc3.Extensions[2] = carla_strdup("");
        desc3.ExtensionCount = 3;
        desc3.UIs = new LV2_RDF_UI[1];
        assert(desc3.UIs[0].URI == nullptr);
        desc3.UIs[0].URI    = carla_strdup("");
        desc3.UIs[0].Binary = carla_strdup("");
        desc3.UIs[0].Bundle = carla_strdup("");
        desc3.UIs[0].Features = new LV2_RDF_Feature[1];
        assert(desc3.UIs[0].Features[0].URI == nullptr);
        desc3.UIs[0].Features[0].URI = carla_strdup("");
        desc3.UIs[0].Extensions = new LV2_URI[3];
        desc3.UIs[0].Extensions[0] = carla_strdup("");
        desc3.UIs[0].Extensions[1] = nullptr;
        desc3.UIs[0].Extensions[2] = carla_strdup("");
        desc3.UIs[0].ExtensionCount = 3;
    }

    return 0;
}

// -----------------------------------------------------------------------
