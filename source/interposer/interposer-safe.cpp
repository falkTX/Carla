/*
 * Carla Interposer
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

#include "CarlaUtils.hpp"

#include <cerrno>
#include <dlfcn.h>
#include <sched.h>
#include <spawn.h>

CARLA_EXPORT
pid_t fork()
{
    carla_stdout("------------------------------- fork called");
    errno = ENOSYS;
    return -1;
}

CARLA_EXPORT
int clone(int (*)(void*), void*, int, void*, ...)
{
    carla_stdout("------------------------------- clone called");
    errno = ENOSYS;
    return -1;
}

CARLA_EXPORT
int posix_spawn(pid_t*, const char*, const posix_spawn_file_actions_t*, const posix_spawnattr_t*, char* const[], char* const[])
{
    carla_stdout("------------------------------- posix_spawn called");
    return ENOSYS;
}

CARLA_EXPORT
int posix_spawnp(pid_t*, const char*, const posix_spawn_file_actions_t*, const posix_spawnattr_t*, char* const[], char* const[])
{
    carla_stdout("------------------------------- posix_spawnp called");
    return ENOSYS;
}
