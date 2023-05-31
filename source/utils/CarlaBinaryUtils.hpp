/*
 * Carla binary utils
 * Copyright (C) 2014-2023 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaScopeUtils.hpp"

#if defined(BUILDING_CARLA)
# include "water/files/FileInputStream.h"
#elif defined(CARLA_UTILS_USE_QT)
# include <QtCore/QFile>
# include <QtCore/QString>
#endif

#if defined(HAVE_LIBMAGIC) && ! defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
# include <magic.h>
# ifdef CARLA_OS_MAC
#  include "CarlaMacUtils.hpp"
# endif
#endif

CARLA_BACKEND_START_NAMESPACE

#if defined(HAVE_LIBMAGIC) && ! defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
// --------------------------------------------------------------------------------------------------------------------

class CarlaMagic
{
public:
    CarlaMagic()
        : fMagic(magic_open(MAGIC_SYMLINK)),
          fLoadedOk(false)
    {
        CARLA_SAFE_ASSERT_RETURN(fMagic != nullptr,);

        fLoadedOk = magic_load(fMagic, std::getenv("CARLA_MAGIC_FILE")) == 0;
    }

    ~CarlaMagic()
    {
        if (fMagic != nullptr)
            magic_close(fMagic);
    }

    const char* getFileDescription(const char* const filename) const
    {
        if (fMagic == nullptr || ! fLoadedOk)
            return nullptr;

        return magic_file(fMagic, filename);
    }

private:
    const magic_t fMagic;
    bool fLoadedOk;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaMagic)
};
#endif

// --------------------------------------------------------------------------------------------------------------------

static inline
BinaryType getBinaryTypeFromFile(const char* const filename)
{
    carla_debug("getBinaryTypeFromFile(\"%s\")", filename);

    if (filename == nullptr || filename[0] == '\0')
        return BINARY_NATIVE;

   #if defined(HAVE_LIBMAGIC) && ! defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
    static const CarlaMagic magic;

    const char* const output(magic.getFileDescription(filename));

    if (output != nullptr && output[0] != '\0')
    {
        if (std::strstr(output, "MS Windows") != nullptr)
            if (std::strstr(output, "PE32 executable") != nullptr || std::strstr(output, "PE32+ executable") != nullptr)
                return (std::strstr(output, "x86-64") != nullptr)
                       ? BINARY_WIN64
                       : BINARY_WIN32;

        if (std::strstr(output, "MS-DOS executable, MZ for MS-DOS") != nullptr)
            return BINARY_WIN32;

        if (std::strstr(output, "ELF") != nullptr)
            return (std::strstr(output, "x86-64") != nullptr || std::strstr(output, "aarch64") != nullptr)
                   ? BINARY_POSIX64
                   : BINARY_POSIX32;

       #ifdef CARLA_OS_MAC
        if (std::strcmp(output, "directory") == 0)
            if (const char* const binary = findBinaryInBundle(filename))
                return getBinaryTypeFromFile(binary);

        if (std::strstr(output, "Mach-O universal binary") != nullptr)
        {
            // This is tricky, binary actually contains multiple architectures
            // We just assume what architectures are more important, and check for them first
            if (std::strstr(output, "x86_64") != nullptr)
                return BINARY_POSIX64;
            if (std::strstr(output, "arm64") != nullptr)
                return BINARY_POSIX64;
            if (std::strstr(output, "i386") != nullptr)
                return BINARY_POSIX32;
            if (std::strstr(output, "ppc") != nullptr)
                return BINARY_OTHER;
        }

        carla_debug("getBinaryTypeFromFile(\"%s\") - have output:\n%s", filename, output);
       #endif

        return BINARY_NATIVE;
    }
   #endif

  #if defined(BUILDING_CARLA) || defined(CARLA_UTILS_USE_QT)
   #if defined(CARLA_UTILS_USE_QT)
    QFile file(QString::fromUtf8(filename));
    CARLA_SAFE_ASSERT_RETURN(file.open(QIODevice::ReadOnly), BINARY_NATIVE);
   #else
    using water::File;
    using water::FileInputStream;

    CarlaScopedPointer<FileInputStream> stream(File(filename).createInputStream());
    CARLA_SAFE_ASSERT_RETURN(stream != nullptr && ! stream->failedToOpen(), BINARY_NATIVE);
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // binary type code based on Ardour's dll_info function
    // See https://github.com/Ardour/ardour/blob/master/libs/ardour/plugin_manager.cc#L867,L925
    // Copyright (C) 2000-2006 Paul Davis

   #if defined(CARLA_UTILS_USE_QT)
    char buf[68];
    if (file.read(buf, 68) != 68)
   #else
    uint8_t buf[68];
    if (stream->read(buf, 68) != 68)
   #endif
        return BINARY_NATIVE;

    if (buf[0] != 'M' && buf[1] != 'Z')
        return BINARY_NATIVE;

    const int32_t* const pe_hdr_off_ptr = (int32_t*)&buf[60];
    const int32_t pe_hdr_off = *pe_hdr_off_ptr;

   #if defined(CARLA_UTILS_USE_QT)
    if (! file.seek(pe_hdr_off))
   #else
    if (! stream->setPosition(pe_hdr_off))
   #endif
        return BINARY_NATIVE;

   #if defined(CARLA_UTILS_USE_QT)
    if (file.read(buf, 6) != 6)
   #else
    if (stream->read(buf, 6) != 6)
   #endif
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
  #else
    return BINARY_NATIVE;
  #endif
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_BINARY_UTILS_HPP_INCLUDED
