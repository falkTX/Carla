/*
 * Custom types to store LADSPA-RDF information
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef LADSPA_RDF_HPP_INCLUDED
#define LADSPA_RDF_HPP_INCLUDED

#include "CarlaDefines.h"

// Base Types
typedef float LADSPA_Data;
typedef int LADSPA_Property;
typedef unsigned long long LADSPA_PluginType;

// Unit Types
#define LADSPA_UNIT_DB                    0x01
#define LADSPA_UNIT_COEF                  0x02
#define LADSPA_UNIT_HZ                    0x04
#define LADSPA_UNIT_S                     0x08
#define LADSPA_UNIT_MS                    0x10
#define LADSPA_UNIT_MIN                   0x20

#define LADSPA_UNIT_CLASS_AMPLITUDE       (LADSPA_UNIT_DB|LADSPA_UNIT_COEF)
#define LADSPA_UNIT_CLASS_FREQUENCY       (LADSPA_UNIT_HZ)
#define LADSPA_UNIT_CLASS_TIME            (LADSPA_UNIT_S|LADSPA_UNIT_MS|LADSPA_UNIT_MIN)

#define LADSPA_IS_UNIT_DB(x)              ((x) == LADSPA_UNIT_DB)
#define LADSPA_IS_UNIT_COEF(x)            ((x) == LADSPA_UNIT_COEF)
#define LADSPA_IS_UNIT_HZ(x)              ((x) == LADSPA_UNIT_HZ)
#define LADSPA_IS_UNIT_S(x)               ((x) == LADSPA_UNIT_S)
#define LADSPA_IS_UNIT_MS(x)              ((x) == LADSPA_UNIT_MS)
#define LADSPA_IS_UNIT_MIN(x)             ((x) == LADSPA_UNIT_MIN)

#define LADSPA_IS_UNIT_CLASS_AMPLITUDE(x) ((x) & LADSPA_UNIT_CLASS_AMPLITUDE)
#define LADSPA_IS_UNIT_CLASS_FREQUENCY(x) ((x) & LADSPA_UNIT_CLASS_FREQUENCY)
#define LADSPA_IS_UNIT_CLASS_TIME(x)      ((x) & LADSPA_UNIT_CLASS_TIME)

// Port Types (Official API)
#define LADSPA_PORT_INPUT                 0x1
#define LADSPA_PORT_OUTPUT                0x2
#define LADSPA_PORT_CONTROL               0x4
#define LADSPA_PORT_AUDIO                 0x8

// Port Hints
#define LADSPA_PORT_UNIT                  0x1
#define LADSPA_PORT_DEFAULT               0x2
#define LADSPA_PORT_LABEL                 0x4

#define LADSPA_PORT_HAS_UNIT(x)           ((x) & LADSPA_PORT_UNIT)
#define LADSPA_PORT_HAS_DEFAULT(x)        ((x) & LADSPA_PORT_DEFAULT)
#define LADSPA_PORT_HAS_LABEL(x)          ((x) & LADSPA_PORT_LABEL)

// Plugin Types
#define LADSPA_PLUGIN_UTILITY             0x000000001LL
#define LADSPA_PLUGIN_GENERATOR           0x000000002LL
#define LADSPA_PLUGIN_SIMULATOR           0x000000004LL
#define LADSPA_PLUGIN_OSCILLATOR          0x000000008LL
#define LADSPA_PLUGIN_TIME                0x000000010LL
#define LADSPA_PLUGIN_DELAY               0x000000020LL
#define LADSPA_PLUGIN_PHASER              0x000000040LL
#define LADSPA_PLUGIN_FLANGER             0x000000080LL
#define LADSPA_PLUGIN_CHORUS              0x000000100LL
#define LADSPA_PLUGIN_REVERB              0x000000200LL
#define LADSPA_PLUGIN_FREQUENCY           0x000000400LL
#define LADSPA_PLUGIN_FREQUENCY_METER     0x000000800LL
#define LADSPA_PLUGIN_FILTER              0x000001000LL
#define LADSPA_PLUGIN_LOWPASS             0x000002000LL
#define LADSPA_PLUGIN_HIGHPASS            0x000004000LL
#define LADSPA_PLUGIN_BANDPASS            0x000008000LL
#define LADSPA_PLUGIN_COMB                0x000010000LL
#define LADSPA_PLUGIN_ALLPASS             0x000020000LL
#define LADSPA_PLUGIN_EQ                  0x000040000LL
#define LADSPA_PLUGIN_PARAEQ              0x000080000LL
#define LADSPA_PLUGIN_MULTIEQ             0x000100000LL
#define LADSPA_PLUGIN_AMPLITUDE           0x000200000LL
#define LADSPA_PLUGIN_PITCH               0x000400000LL
#define LADSPA_PLUGIN_AMPLIFIER           0x000800000LL
#define LADSPA_PLUGIN_WAVESHAPER          0x001000000LL
#define LADSPA_PLUGIN_MODULATOR           0x002000000LL
#define LADSPA_PLUGIN_DISTORTION          0x004000000LL
#define LADSPA_PLUGIN_DYNAMICS            0x008000000LL
#define LADSPA_PLUGIN_COMPRESSOR          0x010000000LL
#define LADSPA_PLUGIN_EXPANDER            0x020000000LL
#define LADSPA_PLUGIN_LIMITER             0x040000000LL
#define LADSPA_PLUGIN_GATE                0x080000000LL
#define LADSPA_PLUGIN_SPECTRAL            0x100000000LL
#define LADSPA_PLUGIN_NOTCH               0x200000000LL

#define LADSPA_GROUP_DYNAMICS             (LADSPA_PLUGIN_DYNAMICS|LADSPA_PLUGIN_COMPRESSOR|LADSPA_PLUGIN_EXPANDER|LADSPA_PLUGIN_LIMITER|LADSPA_PLUGIN_GATE)
#define LADSPA_GROUP_AMPLITUDE            (LADSPA_PLUGIN_AMPLITUDE|LADSPA_PLUGIN_AMPLIFIER|LADSPA_PLUGIN_WAVESHAPER|LADSPA_PLUGIN_MODULATOR|LADSPA_PLUGIN_DISTORTION|LADSPA_GROUP_DYNAMICS)
#define LADSPA_GROUP_EQ                   (LADSPA_PLUGIN_EQ|LADSPA_PLUGIN_PARAEQ|LADSPA_PLUGIN_MULTIEQ)
#define LADSPA_GROUP_FILTER               (LADSPA_PLUGIN_FILTER|LADSPA_PLUGIN_LOWPASS|LADSPA_PLUGIN_HIGHPASS|LADSPA_PLUGIN_BANDPASS|LADSPA_PLUGIN_COMB|LADSPA_PLUGIN_ALLPASS|LADSPA_PLUGIN_NOTCH)
#define LADSPA_GROUP_FREQUENCY            (LADSPA_PLUGIN_FREQUENCY|LADSPA_PLUGIN_FREQUENCY_METER|LADSPA_GROUP_FILTER|LADSPA_GROUP_EQ|LADSPA_PLUGIN_PITCH)
#define LADSPA_GROUP_SIMULATOR            (LADSPA_PLUGIN_SIMULATOR|LADSPA_PLUGIN_REVERB)
#define LADSPA_GROUP_TIME                 (LADSPA_PLUGIN_TIME|LADSPA_PLUGIN_DELAY|LADSPA_PLUGIN_PHASER|LADSPA_PLUGIN_FLANGER|LADSPA_PLUGIN_CHORUS|LADSPA_PLUGIN_REVERB)
#define LADSPA_GROUP_GENERATOR            (LADSPA_PLUGIN_GENERATOR|LADSPA_PLUGIN_OSCILLATOR)

#define LADSPA_IS_PLUGIN_DYNAMICS(x)      ((x) & LADSPA_GROUP_DYNAMICS)
#define LADSPA_IS_PLUGIN_AMPLITUDE(x)     ((x) & LADSPA_GROUP_AMPLITUDE)
#define LADSPA_IS_PLUGIN_EQ(x)            ((x) & LADSPA_GROUP_EQ)
#define LADSPA_IS_PLUGIN_FILTER(x)        ((x) & LADSPA_GROUP_FILTER)
#define LADSPA_IS_PLUGIN_FREQUENCY(x)     ((x) & LADSPA_GROUP_FREQUENCY)
#define LADSPA_IS_PLUGIN_SIMULATOR(x)     ((x) & LADSPA_GROUP_SIMULATOR)
#define LADSPA_IS_PLUGIN_TIME(x)          ((x) & LADSPA_GROUP_TIME)
#define LADSPA_IS_PLUGIN_GENERATOR(x)     ((x) & LADSPA_GROUP_GENERATOR)

// Scale Point
struct LADSPA_RDF_ScalePoint {
    LADSPA_Data Value;
    const char* Label;

    LADSPA_RDF_ScalePoint() noexcept
        : Value(0.0f),
          Label(nullptr) {}

    ~LADSPA_RDF_ScalePoint()
    {
        if (Label != nullptr)
        {
            delete[] Label;
            Label = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(LADSPA_RDF_ScalePoint)
};

// Port
struct LADSPA_RDF_Port {
    LADSPA_Property Type;
    LADSPA_Property Hints;
    const char* Label;
    LADSPA_Data Default;
    LADSPA_Property Unit;

    unsigned long ScalePointCount;
    LADSPA_RDF_ScalePoint* ScalePoints;

    LADSPA_RDF_Port() noexcept
        : Type(0x0),
          Hints(0x0),
          Label(nullptr),
          Default(0.0f),
          Unit(0),
          ScalePointCount(0),
          ScalePoints(nullptr) {}

    ~LADSPA_RDF_Port()
    {
        if (Label != nullptr)
        {
            delete[] Label;
            Label = nullptr;
        }
        if (ScalePoints != nullptr)
        {
            delete[] ScalePoints;
            ScalePoints = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(LADSPA_RDF_Port)
};

// Plugin Descriptor
struct LADSPA_RDF_Descriptor {
    LADSPA_PluginType Type;
    unsigned long UniqueID;
    const char* Title;
    const char* Creator;

    unsigned long PortCount;
    LADSPA_RDF_Port* Ports;

    LADSPA_RDF_Descriptor() noexcept
        : Type(0x0),
          UniqueID(0),
          Title(nullptr),
          Creator(nullptr),
          PortCount(0),
          Ports(nullptr) {}

    ~LADSPA_RDF_Descriptor()
    {
        if (Title != nullptr)
        {
            delete[] Title;
            Title = nullptr;
        }
        if (Creator != nullptr)
        {
            delete[] Creator;
            Creator = nullptr;
        }
        if (Ports != nullptr)
        {
            delete[] Ports;
            Ports = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(LADSPA_RDF_Descriptor)
};

#endif // LADSPA_RDF_HPP_INCLUDED
