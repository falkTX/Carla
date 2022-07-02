/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_INIT_HPP_INCLUDED
#define CARLA_ENGINE_INIT_HPP_INCLUDED

#include "CarlaEngine.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------

/*!
  * Native audio APIs.
  */
enum AudioApi {
    AUDIO_API_NULL,
    // common
    AUDIO_API_JACK,
    AUDIO_API_OSS,
    // linux
    AUDIO_API_ALSA,
    AUDIO_API_PULSEAUDIO,
    // macos
    AUDIO_API_COREAUDIO,
    // windows
    AUDIO_API_ASIO,
    AUDIO_API_DIRECTSOUND,
    AUDIO_API_WASAPI
};

// -------------------------------------------------------------------
// Engine initializers

namespace EngineInit {

// JACK
CarlaEngine* newJack();

// Dummy
CarlaEngine* newDummy();

// Bridge
CarlaEngine* newBridge(const char* audioPoolBaseName,
                       const char* rtClientBaseName,
                       const char* nonRtClientBaseName,
                       const char* nonRtServerBaseName);

// Juce
CarlaEngine*       newJuce(AudioApi api);
uint               getJuceApiCount();
const char*        getJuceApiName(uint index);
const char* const* getJuceApiDeviceNames(uint index);
const EngineDriverDeviceInfo* getJuceDeviceInfo(uint index, const char* deviceName);
bool               showJuceDeviceControlPanel(uint index, const char* deviceName);

// RtAudio
CarlaEngine*       newRtAudio(AudioApi api);
uint               getRtAudioApiCount();
const char*        getRtAudioApiName(uint index);
const char* const* getRtAudioApiDeviceNames(uint index);
const EngineDriverDeviceInfo* getRtAudioDeviceInfo(uint index, const char* deviceName);

// SDL
CarlaEngine*       newSDL();
const char* const* getSDLDeviceNames();

}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INIT_HPP_INCLUDED
