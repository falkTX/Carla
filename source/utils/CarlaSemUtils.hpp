/*
 * Carla semaphore utils
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SEM_UTILS_HPP_INCLUDED
#define CARLA_SEM_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <ctime>

#if defined(CARLA_OS_LINUX) && ! defined(STOAT_TEST_BUILD)
# define CARLA_USE_FUTEXES 1
#endif

#if defined(CARLA_OS_WIN)
# ifdef __WINE__
#  error Wine is not supposed to use this!
# endif
struct carla_sem_t { HANDLE handle; };
#elif defined(CARLA_OS_MAC)
# include <mach/mach.h>
# include <mach/semaphore.h>
# include <servers/bootstrap.h>
struct carla_sem_t { char bootname[32]; semaphore_t sem; semaphore_t sem2; };
#elif defined(CARLA_USE_FUTEXES)
# include <cerrno>
# include <syscall.h>
# include <sys/time.h>
# include <linux/futex.h>
struct carla_sem_t { int count; bool external; };
#else
# include <cerrno>
# include <semaphore.h>
# include <sys/time.h>
# include <sys/types.h>
struct carla_sem_t { sem_t sem; };
#endif

/*
 * Create a new semaphore, pre-allocated version.
 */
static inline
bool carla_sem_create2(carla_sem_t& sem, const bool externalIPC) noexcept
{
    carla_zeroStruct(sem);
#if defined(CARLA_OS_WIN)
    SECURITY_ATTRIBUTES sa;
    carla_zeroStruct(sa);
    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    sem.handle = ::CreateSemaphoreA(externalIPC ? &sa : nullptr, 0, 1, nullptr);

    return (sem.handle != INVALID_HANDLE_VALUE);
#elif defined(CARLA_OS_MAC)
    mach_port_t bootport;
    const mach_port_t task = ::mach_task_self();

    if (externalIPC) {
        CARLA_SAFE_ASSERT_RETURN(task_get_bootstrap_port(task, &bootport) == KERN_SUCCESS, false);
    }
    CARLA_SAFE_ASSERT_RETURN(::semaphore_create(task, &sem.sem, SYNC_POLICY_FIFO, 0) == KERN_SUCCESS, false);

    if (! externalIPC)
        return true;

    static int bootcounter = 0;
    std::snprintf(sem.bootname, 31, "crlsm_%i_%i_%p", ++bootcounter, ::getpid(), &sem);
    sem.bootname[31] = '\0';

   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (::bootstrap_register(bootport, sem.bootname, sem.sem) == KERN_SUCCESS)
   #pragma clang diagnostic pop
        return true;

    sem.bootname[0] = '\0';
    ::semaphore_destroy(task, sem.sem);
    return false;
#elif defined(CARLA_USE_FUTEXES)
    sem.external = externalIPC;
    return true;
#else
    return (::sem_init(&sem.sem, externalIPC, 0) == 0);
#endif
}

/*
 * Create a new semaphore.
 */
static inline
carla_sem_t* carla_sem_create(const bool externalIPC) noexcept
{
    if (carla_sem_t* const sem = (carla_sem_t*)std::malloc(sizeof(carla_sem_t)))
    {
        if (carla_sem_create2(*sem, externalIPC))
            return sem;

        std::free(sem);
    }

    return nullptr;
}

/*
 * Destroy a semaphore, pre-allocated version.
 */
static inline
void carla_sem_destroy2(carla_sem_t& sem) noexcept
{
#if defined(CARLA_OS_WIN)
    ::CloseHandle(sem.handle);
#elif defined(CARLA_OS_MAC)
    ::semaphore_destroy(mach_task_self(), sem.sem);
#elif defined(CARLA_USE_FUTEXES)
    // nothing to do
#else
    ::sem_destroy(&sem.sem);
#endif
    carla_zeroStruct(sem);
}

/*
 * Destroy a semaphore.
 */
static inline
void carla_sem_destroy(carla_sem_t* const sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

    carla_sem_destroy2(*sem);
    std::free(sem);
}

/*
 * Connect to semaphore.
 * Used only on macOS for a client to connect to a server.
 */
static inline
bool carla_sem_connect(carla_sem_t& sem) noexcept
{
#ifdef CARLA_OS_MAC
    mach_port_t bootport;
    CARLA_SAFE_ASSERT_RETURN(task_get_bootstrap_port(mach_task_self(), &bootport) == KERN_SUCCESS, false);

    try {
        return (::bootstrap_look_up(bootport, sem.bootname, &sem.sem2) == KERN_SUCCESS);
    } CARLA_SAFE_EXCEPTION_RETURN("carla_sem_connect", false);
#else
    // nothing to do
    return true;
    // unused
    (void)sem;
#endif
}

/*
 * Post semaphore (unlock).
 */
static inline
void carla_sem_post(carla_sem_t& sem, const bool server = true) noexcept
{
#ifdef CARLA_OS_WIN
    ::ReleaseSemaphore(sem.handle, 1, nullptr);
#elif defined(CARLA_OS_MAC)
    try {
        ::semaphore_signal(server ? sem.sem : sem.sem2);
    } CARLA_SAFE_EXCEPTION_RETURN("carla_sem_post",);
#elif defined(CARLA_USE_FUTEXES)
    const bool unlocked = __sync_bool_compare_and_swap(&sem.count, 0, 1);
    CARLA_SAFE_ASSERT_RETURN(unlocked,);
    ::syscall(__NR_futex, &sem.count, sem.external ? FUTEX_WAKE : FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#else
    ::sem_post(&sem.sem);
#endif
    // may be unused
    return; (void)server;
}

#ifndef CARLA_OS_WASM
/*
 * Wait for a semaphore (lock).
 */
static inline
bool carla_sem_timedwait(carla_sem_t& sem, const uint msecs, const bool server = true) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0, false);

#if defined(CARLA_OS_WIN)
    return (::WaitForSingleObject(sem.handle, msecs) == WAIT_OBJECT_0);
#else
    const uint secs  =  msecs / 1000;
    const uint nsecs = (msecs % 1000) * 1000000;

# if defined(CARLA_OS_MAC)
    const mach_timespec timeout = { secs, static_cast<int>(nsecs) };

    try {
        return (::semaphore_timedwait(server ? sem.sem : sem.sem2, timeout) == KERN_SUCCESS);
    } CARLA_SAFE_EXCEPTION_RETURN("carla_sem_timedwait", false);
# elif defined(CARLA_USE_FUTEXES)
    const timespec timeout = { static_cast<time_t>(secs), static_cast<long>(nsecs) };

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&sem.count, 1, 0))
            return true;

        if (::syscall(__NR_futex, &sem.count, sem.external ? FUTEX_WAIT : FUTEX_WAIT_PRIVATE, 0, &timeout, nullptr, 0) != 0)
            if (errno != EAGAIN && errno != EINTR)
                return false;
    }
# else
    if (::sem_trywait(&sem.sem) == 0)
        return true;

    timespec now;
    ::clock_gettime(CLOCK_REALTIME, &now);

    const timespec delta = { static_cast<time_t>(secs), static_cast<long>(nsecs) };
    /* */ timespec end   = { now.tv_sec + delta.tv_sec, now.tv_nsec + delta.tv_nsec };
    if (end.tv_nsec >= 1000000000L) {
        ++end.tv_sec;
        end.tv_nsec -= 1000000000L;
    }

    for (int ret;;)
    {
        try {
            ret = sem_timedwait(&sem.sem, &end);
        } CARLA_SAFE_EXCEPTION_RETURN("carla_sem_timedwait", false);

        if (ret == 0)
            return true;
        if (errno != EINTR)
            return false;
    }
# endif
#endif
    // may be unused
    (void)server;
}
#endif

// -----------------------------------------------------------------------

#endif // CARLA_SEM_UTILS_HPP_INCLUDED
