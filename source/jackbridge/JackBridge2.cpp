/*
 * JackBridge (Part 2, Semaphore + Shared memory functions)
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "JackBridge.hpp"

#ifdef JACKBRIDGE_DUMMY
# include "CarlaUtils.hpp"
#else
# include "CarlaSemUtils.hpp"
# include "CarlaShmUtils.hpp"
#endif // ! JACKBRIDGE_DUMMY

// -----------------------------------------------------------------------------

bool jackbridge_sem_init(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifndef JACKBRIDGE_DUMMY
    return carla_sem_create2(*(carla_sem_t*)sem);
#endif
}

void jackbridge_sem_destroy(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_sem_destroy2(*(carla_sem_t*)sem);
#endif
}

void jackbridge_sem_post(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_sem_post(*(carla_sem_t*)sem);
#endif
}

bool jackbridge_sem_timedwait(void* sem, uint secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_sem_timedwait(*(carla_sem_t*)sem, secs);
#endif
}

// -----------------------------------------------------------------------------

bool jackbridge_shm_is_valid(const void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, false);

#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_is_shm_valid(*(const carla_shm_t*)shm);
#endif
}

void jackbridge_shm_init(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_init(*(carla_shm_t*)shm);
#endif
}

void jackbridge_shm_attach(void* shm, const char* name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    *(carla_shm_t*)shm = carla_shm_attach(name);
#endif
}

void jackbridge_shm_close(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_close(*(carla_shm_t*)shm);
#endif
}

void* jackbridge_shm_map(void* shm, uint64_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, nullptr);

#ifdef JACKBRIDGE_DUMMY
    return nullptr;
#else
    return carla_shm_map(*(carla_shm_t*)shm, size);
#endif
}

// -----------------------------------------------------------------------------
