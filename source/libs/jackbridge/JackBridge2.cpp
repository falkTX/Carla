/*
 * JackBridge (Part 2, Semaphore functions)
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "CarlaDefines.hpp"

// don't include the whole JACK deal in this file
CARLA_EXPORT bool jackbridge_sem_post(void* sem);
CARLA_EXPORT bool jackbridge_sem_timedwait(void* sem, int secs);

// -----------------------------------------------------------------------------

#if JACKBRIDGE_DUMMY
bool jackbridge_sem_post(void*)
{
    return false;
}

bool jackbridge_sem_timedwait(void*, int)
{
    return false;
}
#else

#include <semaphore.h>

#ifdef __WINE__
# define _STRUCT_TIMEVAL 1
# define _SYS_SELECT_H   1
# include <bits/types.h>
struct timespec {
    __time_t tv_sec;  /* Seconds.  */
    long int tv_nsec; /* Nanoseconds.  */
};
#endif

#ifdef CARLA_OS_WIN
# include <sys/time.h>
#else
# include <time.h>
#endif

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
