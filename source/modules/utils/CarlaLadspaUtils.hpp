/*
 * Carla LADSPA utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef CARLA_LADSPA_UTILS_HPP_INCLUDED
#define CARLA_LADSPA_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include "ladspa/ladspa.h"
#include "ladspa_rdf.hpp"

#include <cmath>

// -----------------------------------------------------------------------
// Copy RDF object

static inline
const LADSPA_RDF_Descriptor* ladspa_rdf_dup(const LADSPA_RDF_Descriptor* const oldDescriptor)
{
    CARLA_ASSERT(oldDescriptor != nullptr);

    if (oldDescriptor == nullptr)
        return nullptr;

    LADSPA_RDF_Descriptor* const newDescriptor(new LADSPA_RDF_Descriptor());

    newDescriptor->Type      = oldDescriptor->Type;
    newDescriptor->UniqueID  = oldDescriptor->UniqueID;
    newDescriptor->PortCount = oldDescriptor->PortCount;

    if (oldDescriptor->Title != nullptr)
        newDescriptor->Title = carla_strdup(oldDescriptor->Title);

    if (oldDescriptor->Creator != nullptr)
        newDescriptor->Creator = carla_strdup(oldDescriptor->Creator);

    if (newDescriptor->PortCount > 0)
    {
        newDescriptor->Ports = new LADSPA_RDF_Port[newDescriptor->PortCount];

        for (unsigned long i=0; i < newDescriptor->PortCount; ++i)
        {
            LADSPA_RDF_Port* const oldPort(&oldDescriptor->Ports[i]);
            LADSPA_RDF_Port* const newPort(&newDescriptor->Ports[i]);

            newPort->Type    = oldPort->Type;
            newPort->Hints   = oldPort->Hints;
            newPort->Default = oldPort->Default;
            newPort->Unit    = oldPort->Unit;
            newPort->ScalePointCount = oldPort->ScalePointCount;

            if (oldPort->Label != nullptr)
                newPort->Label = carla_strdup(oldPort->Label);

            if (oldPort->ScalePointCount > 0)
            {
                newPort->ScalePoints = new LADSPA_RDF_ScalePoint[oldPort->ScalePointCount];

                for (unsigned long j=0; j < oldPort->ScalePointCount; ++j)
                {
                    LADSPA_RDF_ScalePoint* const oldScalePoint(&oldPort->ScalePoints[j]);
                    LADSPA_RDF_ScalePoint* const newScalePoint(&newPort->ScalePoints[j]);

                    newScalePoint->Value = oldScalePoint->Value;

                    if (oldScalePoint->Label != nullptr)
                        newScalePoint->Label = carla_strdup(oldScalePoint->Label);
                }
            }
        }
    }

    return newDescriptor;
}

// -----------------------------------------------------------------------
// Check if 2 ports match types

static inline
bool is_ladspa_port_good(const LADSPA_PortDescriptor port1, const LADSPA_PortDescriptor port2)
{
    if (LADSPA_IS_PORT_INPUT(port1) && ! LADSPA_IS_PORT_INPUT(port2))
        return false;
    if (LADSPA_IS_PORT_OUTPUT(port1) && ! LADSPA_IS_PORT_OUTPUT(port2))
        return false;
    if (LADSPA_IS_PORT_CONTROL(port1) && ! LADSPA_IS_PORT_CONTROL(port2))
        return false;
    if (LADSPA_IS_PORT_AUDIO(port1) && ! LADSPA_IS_PORT_AUDIO(port2))
        return false;
    return true;
}

// -----------------------------------------------------------------------
// Check if rdf data matches descriptor

static inline
bool is_ladspa_rdf_descriptor_valid(const LADSPA_RDF_Descriptor* const rdfDescriptor, const LADSPA_Descriptor* const descriptor)
{
    if (rdfDescriptor == nullptr || descriptor == nullptr)
        return false;

    if (rdfDescriptor->UniqueID != descriptor->UniqueID)
    {
        carla_stderr("WARNING - Plugin has wrong UniqueID: %li != %li", rdfDescriptor->UniqueID, descriptor->UniqueID);
        return false;
    }

    if (rdfDescriptor->PortCount > descriptor->PortCount)
    {
        carla_stderr("WARNING - Plugin has RDF data, but invalid PortCount: %li > %li", rdfDescriptor->PortCount, descriptor->PortCount);
        return false;
    }

    for (unsigned long i=0; i < rdfDescriptor->PortCount; ++i)
    {
        if (! is_ladspa_port_good(rdfDescriptor->Ports[i].Type, descriptor->PortDescriptors[i]))
        {
            carla_stderr("WARNING - Plugin has RDF data, but invalid PortTypes: %i != %i", rdfDescriptor->Ports[i].Type, descriptor->PortDescriptors[i]);
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------
// Get default control port value

static inline
LADSPA_Data get_default_ladspa_port_value(const LADSPA_PortRangeHintDescriptor hintDescriptor, const LADSPA_Data min, const LADSPA_Data max)
{
    LADSPA_Data def;

    if (LADSPA_IS_HINT_HAS_DEFAULT(hintDescriptor))
    {
        switch (hintDescriptor & LADSPA_HINT_DEFAULT_MASK)
        {
        case LADSPA_HINT_DEFAULT_MINIMUM:
            def = min;
            break;
        case LADSPA_HINT_DEFAULT_MAXIMUM:
            def = max;
            break;
        case LADSPA_HINT_DEFAULT_0:
            def = 0.0f;
            break;
        case LADSPA_HINT_DEFAULT_1:
            def = 1.0f;
            break;
        case LADSPA_HINT_DEFAULT_100:
            def = 100.0f;
            break;
        case LADSPA_HINT_DEFAULT_440:
            def = 440.0f;
            break;
        case LADSPA_HINT_DEFAULT_LOW:
            if (LADSPA_IS_HINT_LOGARITHMIC(hintDescriptor))
                def = std::exp((std::log(min)*0.75f) + (std::log(max)*0.25f));
            else
                def = (min*0.75f) + (max*0.25f);
            break;
        case LADSPA_HINT_DEFAULT_MIDDLE:
            if (LADSPA_IS_HINT_LOGARITHMIC(hintDescriptor))
                def = std::sqrt(min*max);
            else
                def = (min+max)/2;
            break;
        case LADSPA_HINT_DEFAULT_HIGH:
            if (LADSPA_IS_HINT_LOGARITHMIC(hintDescriptor))
                def = std::exp((std::log(min)*0.25f) + (std::log(max)*0.75f));
            else
                def = (min*0.25f) + (max*0.75f);
            break;
        default:
            if (min < 0.0f && max > 0.0f)
                def = 0.0f;
            else
                def = min;
            break;
        }
    }
    else
    {
        // no default value
        if (min < 0.0f && max > 0.0f)
            def = 0.0f;
        else
            def = min;
    }

    return def;
}

#endif // CARLA_LADSPA_UTILS_HPP_INCLUDED
