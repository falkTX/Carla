/*
 * JackBridge (Part 2, Semaphore functions)
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.hpp"

#ifndef JACKBRIDGE_HPP_INCLUDED
// don't include the whole JACK API in this file
CARLA_EXPORT bool jackbridge_sem_init(void* sem);
CARLA_EXPORT bool jackbridge_sem_destroy(void* sem);
CARLA_EXPORT bool jackbridge_sem_post(void* sem);
CARLA_EXPORT bool jackbridge_sem_timedwait(void* sem, int secs);
#endif

// -----------------------------------------------------------------------------

#if JACKBRIDGE_DUMMY
bool jackbridge_sem_init(void*)
{
    return false;
}

bool jackbridge_sem_destroy(void*)
{
    return false;
}

bool jackbridge_sem_post(void*)
{
    return false;
}

bool jackbridge_sem_timedwait(void*, int)
{
    return false;
}
#else

#include <ctime>
#include <semaphore.h>
#include <sys/time.h>

bool jackbridge_sem_init(void* sem)
{
    return (sem_init((sem_t*)sem, 1, 0) == 0);
}

bool jackbridge_sem_destroy(void* sem)
{
    return (sem_destroy((sem_t*)sem) == 0);
}

bool jackbridge_sem_post(void* sem)
{
    return (sem_post((sem_t*)sem) == 0);
}

bool jackbridge_sem_timedwait(void* sem, int secs)
{
# ifdef CARLA_OS_MAC
        alarm(secs);
        return (sem_wait((sem_t*)sem) == 0);
# else
        timespec timeout;

#  ifdef CARLA_OS_WIN
        timeval now;
        gettimeofday(&now, nullptr);
        timeout.tv_sec  = now.tv_sec;
        timeout.tv_nsec = now.tv_usec * 1000;
#  else
        clock_gettime(CLOCK_REALTIME, &timeout);
#  endif

        timeout.tv_sec += secs;

        return (sem_timedwait((sem_t*)sem, &timeout) == 0);
# endif
}
#endif

// -----------------------------------------------------------------------------
