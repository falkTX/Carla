/*
 * Carla binary utils
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

#ifndef CARLA_BINARY_UTILS_HPP_INCLUDED
#define CARLA_BINARY_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaUtils.hpp"

#if defined(CARLA_OS_LINUX) && ! defined(BUILD_BRIDGE)
# include "magic.h"
#endif

CARLA_BACKEND_START_NAMESPACE

#if defined(CARLA_OS_LINUX) && ! defined(BUILD_BRIDGE)
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
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaMagic)
};

static const CarlaMagic& getCarlaMagicInstance()
{
    static CarlaMagic magic;
    return magic;
}
#endif

// -----------------------------------------------------------------------

static inline
BinaryType getBinaryTypeFromFile(const char* const filename)
{
    carla_stdout("getBinaryTypeFromFile(\"%s\")", filename);

#if defined(CARLA_OS_LINUX) && ! defined(BUILD_BRIDGE)
    if (filename == nullptr || filename[0] == '\0')
        return BINARY_NATIVE;

    const CarlaMagic& magic(getCarlaMagicInstance());

    const char* const output(magic.getFileDescription(filename));

    if (output == nullptr || output[0] == '\0')
        return BINARY_NATIVE;

    if (std::strstr(output, "PE32 executable") != nullptr && std::strstr(output, "MS Windows") != nullptr)
        return std::strstr(output, "x86-64") != nullptr ? BINARY_WIN64 : BINARY_WIN32;

    if (std::strstr(output, "ELF") != nullptr)
        return (std::strstr(output, "x86-64") != nullptr || std::strstr(output, "aarch64") != nullptr) ? BINARY_POSIX64 : BINARY_POSIX32;
#endif

    return BINARY_NATIVE;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_BINARY_UTILS_HPP_INCLUDED
