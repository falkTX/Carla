/*
 * Carla Native Plugins
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BUNDLE_TYPE
# error CARLA_BUNDLE_TYPE undefined
#endif

// --------------------------------------------------------------------------------------------------------------------

#include "carla-lv2.cpp"

// --------------------------------------------------------------------------------------------------------------------

#define MAX_PLUGIN_DESCRIPTORS 8
static const NativePluginDescriptor* sDescs[MAX_PLUGIN_DESCRIPTORS];
static std::size_t sDescsUsed = 0;

std::size_t carla_getNativePluginCount() noexcept
{
    return sDescsUsed;
}

const NativePluginDescriptor* carla_getNativePluginDescriptor(const std::size_t index) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < sDescsUsed, nullptr);

    return sDescs[index];
}

void carla_register_native_plugin(const NativePluginDescriptor* desc)
{
    CARLA_SAFE_ASSERT_RETURN(sDescsUsed < MAX_PLUGIN_DESCRIPTORS,);

    sDescs[sDescsUsed++] = desc;
}

// --------------------------------------------------------------------------------------------------------------------

static void carla_register_bundled_native_plugins(void);

static const struct PluginRegisterer {
  PluginRegisterer() {
      carla_register_bundled_native_plugins();
  }
} sPluginRegisterer;

// --------------------------------------------------------------------------------------------------------------------

extern "C" {

// Simple plugins
void carla_register_native_plugin_audiogain(void);
void carla_register_native_plugin_bypass(void);
void carla_register_native_plugin_lfo(void);
void carla_register_native_plugin_midichanab(void);
void carla_register_native_plugin_midichanfilter(void);
void carla_register_native_plugin_midichannelize(void);
void carla_register_native_plugin_midigain(void);
void carla_register_native_plugin_midijoin(void);
void carla_register_native_plugin_midisplit(void);
void carla_register_native_plugin_midithrough(void);
void carla_register_native_plugin_miditranspose(void);

// Audio file
void carla_register_native_plugin_audiofile(void);

// MIDI file and sequencer
void carla_register_native_plugin_midifile(void);
void carla_register_native_plugin_midipattern(void);

// Carla
void carla_register_native_plugin_carla(void);

// External-UI plugins
void carla_register_native_plugin_bigmeter(void);
void carla_register_native_plugin_notes(void);

#ifdef HAVE_EXTERNAL_PLUGINS
void carla_register_all_native_external_plugins(void);
#endif

}

static void carla_register_bundled_native_plugins(void)
{
#if CARLA_BUNDLE_TYPE == 1
    carla_register_native_plugin_audiogain();
#elif CARLA_BUNDLE_TYPE == 2
    carla_register_native_plugin_audiofile();
    carla_register_native_plugin_midifile();
#elif CARLA_BUNDLE_TYPE == 3
    carla_register_native_plugin_midichanab();
    carla_register_native_plugin_midichannelize();
    carla_register_native_plugin_midichanfilter();
    carla_register_native_plugin_midigain();
    carla_register_native_plugin_midijoin();
    carla_register_native_plugin_midisplit();
    carla_register_native_plugin_miditranspose();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
