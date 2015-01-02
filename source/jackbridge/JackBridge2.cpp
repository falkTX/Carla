/*
 * JackBridge (Part 2, Semaphore functions)
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
# include <cerrno>
# include "CarlaSemUtils.hpp"
# include "CarlaShmUtils.hpp"
#endif // ! JACKBRIDGE_DUMMY

// -----------------------------------------------------------------------------

bool jackbridge_sem_init(void* sem) noexcept
{
#if defined(JACKBRIDGE_DUMMY)
    return false;
#else
    sem_t* const sema(carla_sem_create());
    CARLA_SAFE_ASSERT_RETURN(sema != nullptr, false);

    std::memcpy(sem, sema, sizeof(sem_t));
    return (sema != nullptr);
#endif
}

void jackbridge_sem_destroy(void* sem) noexcept
{
#ifndef JACKBRIDGE_DUMMY
    carla_sem_destroy((sem_t*)sem);
#endif
}

bool jackbridge_sem_post(void* sem) noexcept
{
#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_sem_post((sem_t*)sem);
#endif
}

bool jackbridge_sem_timedwait(void* sem, uint secs, bool* timedOut) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(timedOut != nullptr, false);
#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    if (carla_sem_timedwait((sem_t*)sem, secs))
    {
        *timedOut = false;
        return true;
    }
    *timedOut = (errno == ETIMEDOUT);
    return false;
#endif
}

// -----------------------------------------------------------------------------

bool jackbridge_shm_is_valid(const void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, false);
#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_is_shm_valid(*(const shm_t*)shm);
#endif
}

void jackbridge_shm_init(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_init(*(shm_t*)shm);
#endif
}

void jackbridge_shm_attach(void* shm, const char* name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    *(shm_t*)shm = carla_shm_attach(name);
#endif
}

void jackbridge_shm_close(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_close(*(shm_t*)shm);
#endif
}

void* jackbridge_shm_map(void* shm, size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, nullptr);

#ifdef JACKBRIDGE_DUMMY
    return nullptr;
#else
    return carla_shm_map(*(shm_t*)shm, size);
#endif
}

// -----------------------------------------------------------------------------
