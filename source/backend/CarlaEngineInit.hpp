// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
