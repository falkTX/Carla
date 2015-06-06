/*
 * Carla library utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LIB_UTILS_HPP_INCLUDED
#define CARLA_LIB_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_WIN
typedef HMODULE lib_t;
#else
# include <dlfcn.h>
typedef void* lib_t;
#endif

// -----------------------------------------------------------------------
// library related calls

/*
 * Open 'filename' library (must not be null).
 * May return null, in which case "lib_error" has the error.
 */
static inline
lib_t lib_open(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

    try {
#ifdef CARLA_OS_WIN
        return ::LoadLibraryA(filename);
#else
        return ::dlopen(filename, RTLD_NOW|RTLD_LOCAL);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("lib_open", nullptr);
}

/*
 * Close a previously opened library (must not be null).
 * If false is returned, "lib_error" has the error.
 */
static inline
bool lib_close(const lib_t lib) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(lib != nullptr, false);

    try {
#ifdef CARLA_OS_WIN
        return ::FreeLibrary(lib);
#else
        return (::dlclose(lib) == 0);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("lib_close", false);
}

/*
 * Get a library symbol (must not be null).
 * Returns null if the symbol is not found.
 */
template<typename Func>
static inline
Func lib_symbol(const lib_t lib, const char* const symbol) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(lib != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(symbol != nullptr && symbol[0] != '\0', nullptr);

    try {
#ifdef CARLA_OS_WIN
        return (Func)::GetProcAddress(lib, symbol);
#else
        return (Func)(uintptr_t)::dlsym(lib, symbol);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("lib_symbol", nullptr);
}

/*
 * Return the last operation error ('filename' must not be null).
 * May return null.
 */
static inline
const char* lib_error(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

#ifdef CARLA_OS_WIN
    static char libError[2048+1];
    carla_zeroChars(libError, 2048+1);

    try {
        const DWORD winErrorCode  = ::GetLastError();
        const int   winErrorFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS;
        LPVOID      winErrorString;

        ::FormatMessage(winErrorFlags, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

        std::snprintf(libError, 2048, "%s: error code %li: %s", filename, winErrorCode, (const char*)winErrorString);
        ::LocalFree(winErrorString);
    } CARLA_SAFE_EXCEPTION("lib_error");

    return (libError[0] != '\0') ? libError : nullptr;
#else
    return ::dlerror();
#endif
}

// -----------------------------------------------------------------------

#endif // CARLA_LIB_UTILS_HPP_INCLUDED
