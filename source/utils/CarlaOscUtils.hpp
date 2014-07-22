/*
 * Carla OSC utils
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_OSC_UTILS_HPP_INCLUDED
#define CARLA_OSC_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <lo/lo.h>

#define try_lo_send(...)      \
    try {                     \
        lo_send(__VA_ARGS__); \
    } CARLA_SAFE_EXCEPTION("lo_send");

// -----------------------------------------------------------------------

struct CarlaOscData {
    const char* path;
    lo_address source;
    lo_address target;

    CarlaOscData() noexcept
        : path(nullptr),
          source(nullptr),
          target(nullptr) {}

    ~CarlaOscData() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        if (path != nullptr)
        {
            delete[] path;
            path = nullptr;
        }

        if (source != nullptr)
        {
            try {
                lo_address_free(source);
            } CARLA_SAFE_EXCEPTION("lo_address_free source");
            source = nullptr;
        }

        if (target != nullptr)
        {
            try {
                lo_address_free(target);
            } CARLA_SAFE_EXCEPTION("lo_address_free target");
            target = nullptr;
        }
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaOscData)
};

// -----------------------------------------------------------------------

static inline
void osc_send_configure(const CarlaOscData& oscData, const char* const key, const char* const value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("osc_send_configure(path:\"%s\", \"%s\", \"%s\")", oscData.path, key, value);

    char targetPath[std::strlen(oscData.path)+11];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/configure");
    try_lo_send(oscData.target, targetPath, "ss", key, value);
}

static inline
void osc_send_control(const CarlaOscData& oscData, const int32_t index, const float value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(index != -1,); // -1 == PARAMETER_NULL
    carla_debug("osc_send_control(path:\"%s\", %i, %f)", oscData.path, index, value);

    char targetPath[std::strlen(oscData.path)+9];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/control");
    try_lo_send(oscData.target, targetPath, "if", index, value);
}

static inline
void osc_send_program(const CarlaOscData& oscData, const uint32_t index) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_program(path:\"%s\", %u)", oscData.path, index);

    char targetPath[std::strlen(oscData.path)+9];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/program");
    try_lo_send(oscData.target, targetPath, "i", static_cast<int32_t>(index));
}

static inline
void osc_send_program(const CarlaOscData& oscData, const uint32_t bank, const uint32_t program) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_program(path:\"%s\", %u, %u)", oscData.path, bank, program);

    char targetPath[std::strlen(oscData.path)+9];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/program");
    try_lo_send(oscData.target, targetPath, "ii", static_cast<int32_t>(bank), static_cast<int32_t>(program));
}

static inline
void osc_send_midi_program(const CarlaOscData& oscData, const uint32_t index) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_midi_program(path:\"%s\", %u)", oscData.path, index);

    char targetPath[std::strlen(oscData.path)+14];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/midi-program");
    try_lo_send(oscData.target, targetPath, "i", static_cast<int32_t>(index));
}

static inline
void osc_send_midi_program(const CarlaOscData& oscData, const uint32_t bank, const uint32_t program) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_midi_program(path:\"%s\", %u, %u)", oscData.path, bank, program);

    char targetPath[std::strlen(oscData.path)+14];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/midi-program");
    try_lo_send(oscData.target, targetPath, "ii", static_cast<int32_t>(bank), static_cast<int32_t>(program));
}

static inline
void osc_send_midi(const CarlaOscData& oscData, const uint8_t buf[4]) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(buf[0] == 0,);
    CARLA_SAFE_ASSERT_RETURN(buf[1] != 0,);
    carla_debug("osc_send_midi(path:\"%s\", port:%u, {0x%X, %03u, %03u})", oscData.path, buf[0], buf[1], buf[2], buf[3]);

    char targetPath[std::strlen(oscData.path)+6];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/midi");
    try_lo_send(oscData.target, targetPath, "m", buf);
}

static inline
void osc_send_sample_rate(const CarlaOscData& oscData, const float sampleRate) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(sampleRate > 0.0f,);
    carla_debug("osc_send_sample_rate(path:\"%s\", %f)", oscData.path, sampleRate);

    char targetPath[std::strlen(oscData.path)+13];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/sample-rate");
    try_lo_send(oscData.target, targetPath, "f", sampleRate);
}

#ifdef BUILD_BRIDGE
static inline
void osc_send_update(const CarlaOscData& oscData, const char* const url) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(url != nullptr && url[0] != '\0',);
    carla_debug("osc_send_update(path:\"%s\", \"%s\")", oscData.path, url);

    char targetPath[std::strlen(oscData.path)+8];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/update");
    try_lo_send(oscData.target, targetPath, "s", url);
}

static inline
void osc_send_exiting(const CarlaOscData& oscData) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_exiting(path:\"%s\")", oscData.path);

    char targetPath[std::strlen(oscData.path)+9];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/exiting");
    try_lo_send(oscData.target, targetPath, "");
}
#endif

static inline
void osc_send_show(const CarlaOscData& oscData) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_show(path:\"%s\")", oscData.path);

    char targetPath[std::strlen(oscData.path)+6];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/show");
    try_lo_send(oscData.target, targetPath, "");
}

static inline
void osc_send_hide(const CarlaOscData& oscData) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_hide(path:\"%s\")", oscData.path);

    char targetPath[std::strlen(oscData.path)+6];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/hide");
    try_lo_send(oscData.target, targetPath, "");
}

static inline
void osc_send_quit(const CarlaOscData& oscData) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    carla_debug("osc_send_quit(path:\"%s\")", oscData.path);

    char targetPath[std::strlen(oscData.path)+6];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/quit");
    try_lo_send(oscData.target, targetPath, "");
}

// -----------------------------------------------------------------------

#ifdef BUILD_BRIDGE_PLUGIN
static inline
void osc_send_bridge_update(const CarlaOscData& oscData, const char* const url) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(url != nullptr && url[0] != '\0',);
    carla_debug("osc_send_bridge_update(path:\"%s\", \"%s\")", oscData.path, url);

    char targetPath[std::strlen(oscData.path)+19];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/bridge_update_now");
    try_lo_send(oscData.target, targetPath, "s", url);
}

static inline
void osc_send_bridge_error(const CarlaOscData& oscData, const char* const error) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(error != nullptr && error[0] != '\0',);
    carla_debug("osc_send_bridge_error(path:\"%s\", \"%s\")", oscData.path, error);

    char targetPath[std::strlen(oscData.path)+14];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/bridge_error");
    try_lo_send(oscData.target, targetPath, "s", error);
}
#endif

#if defined(BRIDGE_LV2) || defined(WANT_LV2)
static inline
void osc_send_lv2_atom_transfer(const CarlaOscData& oscData, const uint32_t portIndex, const char* const atomBuf) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(atomBuf != nullptr && atomBuf[0] != '\0',);
    carla_debug("osc_send_lv2_atom_transfer(path:\"%s\", %u, <atomBuf:%p>)", oscData.path, portIndex, atomBuf);

    char targetPath[std::strlen(oscData.path)+19];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/lv2_atom_transfer");
    try_lo_send(oscData.target, targetPath, "is", static_cast<int32_t>(portIndex), atomBuf);
}

static inline
void osc_send_lv2_urid_map(const CarlaOscData& oscData, const uint32_t urid, const char* const uri) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(oscData.path != nullptr && oscData.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(oscData.target != nullptr,);
    //CARLA_SAFE_ASSERT_RETURN(urid != 0,);
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0',);
    carla_debug("osc_send_lv2_urid_map(path:\"%s\", %u, \"%s\")", oscData.path, urid, uri);

    char targetPath[std::strlen(oscData.path)+14];
    std::strcpy(targetPath, oscData.path);
    std::strcat(targetPath, "/lv2_urid_map");
    try_lo_send(oscData.target, targetPath, "is", static_cast<int32_t>(urid), uri);
}
#endif

// -----------------------------------------------------------------------

#endif // CARLA_OSC_UTILS_HPP_INCLUDED
