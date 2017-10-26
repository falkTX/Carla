/*
 * Carla binary utils
 * Copyright (C) 2014-2017 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BINARY_UTILS_HPP_INCLUDED
#define CARLA_BINARY_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaUtils.hpp"

#if defined(CARLA_OS_WIN)
# include "AppConfig.h"
# include "juce_core/juce_core.h"
#elif defined(HAVE_LIBMAGIC)
# include <magic.h>
#endif

CARLA_BACKEND_START_NAMESPACE

#ifdef HAVE_LIBMAGIC
// -----------------------------------------------------------------------

class CarlaMagic
{
public:
    CarlaMagic()
        : fMagic(magic_open(MAGIC_SYMLINK))
    {
        CARLA_SAFE_ASSERT_RETURN(fMagic != nullptr,);
        magic_load(fMagic, nullptr);
    }

    ~CarlaMagic()
    {
        if (fMagic != nullptr)
            magic_close(fMagic);
    }

    const char* getFileDescription(const char* const filename) const
    {
        if (fMagic == nullptr)
            return nullptr;

        return magic_file(fMagic, filename);
    }

private:
    const magic_t fMagic;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaMagic)
};
#endif

// -----------------------------------------------------------------------

static inline
BinaryType getBinaryTypeFromFile(const char* const filename)
{
    carla_debug("getBinaryTypeFromFile(\"%s\")", filename);

    if (filename == nullptr || filename[0] == '\0')
        return BINARY_NATIVE;

#if defined(CARLA_OS_WIN)
    using juce::File;
    using juce::FileInputStream;
    using juce::ScopedPointer;

    ScopedPointer<FileInputStream> stream(File(filename).createInputStream());
    CARLA_SAFE_ASSERT_RETURN(stream != nullptr || stream->failedToOpen(), BINARY_NATIVE);

    // -------------------------------------------------------------------
    // binary type code based on Ardour's dll_info function
    // See https://github.com/Ardour/ardour/blob/master/libs/ardour/plugin_manager.cc#L867,L925
    // Copyright (C) 2000-2006 Paul Davis

    uint8_t buf[68];

    if (stream->read(buf, 68) != 68)
        return BINARY_NATIVE;

    if (buf[0] != 'M' && buf[1] != 'Z')
        return BINARY_NATIVE;

    const int32_t* const pe_hdr_off_ptr = (int32_t*)&buf[60];
    const int32_t pe_hdr_off = *pe_hdr_off_ptr;

    if (! stream->setPosition(pe_hdr_off))
        return BINARY_NATIVE;

    if (stream->read(buf, 6) != 6)
        return BINARY_NATIVE;

    if (buf[0] != 'P' && buf[1] != 'E')
        return BINARY_NATIVE;

    const uint16_t* const type_ptr = (uint16_t*)&buf[4];
    const uint16_t type = *type_ptr;

    switch (type)
    {
    case 0x014c:
        return BINARY_WIN32;
    case 0x8664:
        return BINARY_WIN64;
    default:
        return BINARY_NATIVE;
    }
#elif defined(HAVE_LIBMAGIC)
    static const CarlaMagic magic;

    const char* const output(magic.getFileDescription(filename));

    if (output == nullptr || output[0] == '\0')
        return BINARY_NATIVE;

    if (std::strstr(output, "MS Windows") != nullptr)
        if (std::strstr(output, "PE32 executable") != nullptr || std::strstr(output, "PE32+ executable") != nullptr)
            return (std::strstr(output, "x86-64") != nullptr)
                   ? BINARY_WIN64
                   : BINARY_WIN32;

    if (std::strstr(output, "ELF") != nullptr)
        return (std::strstr(output, "x86-64") != nullptr || std::strstr(output, "aarch64") != nullptr)
               ? BINARY_POSIX64
               : BINARY_POSIX32;
#endif

    return BINARY_NATIVE;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_BINARY_UTILS_HPP_INCLUDED
