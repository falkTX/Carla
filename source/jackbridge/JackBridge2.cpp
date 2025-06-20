/*
 * JackBridge (Part 2, Semaphore + Shared memory and other misc functions)
 * Copyright (C) 2013-2025 Filipe Coelho <falktx@falktx.com>
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
# ifdef __WINE__
#  define NOMINMAX
#  include <winsock2.h>
# endif
# include "CarlaProcessUtils.hpp"
# include "CarlaSemUtils.hpp"
# include "CarlaShmUtils.hpp"
# ifdef __WINE__
#  include "utils/PipeClient.cpp"
# endif
#endif // ! JACKBRIDGE_DUMMY

// --------------------------------------------------------------------------------------------------------------------

bool jackbridge_sem_init(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifndef JACKBRIDGE_DUMMY
    return carla_sem_create2(*(carla_sem_t*)sem, true);
#endif
}

void jackbridge_sem_destroy(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_sem_destroy2(*(carla_sem_t*)sem);
#endif
}

bool jackbridge_sem_connect(void* sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return true;
#endif
}

void jackbridge_sem_post(void* sem, bool) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_sem_post(*(carla_sem_t*)sem);
#endif
}

#ifndef CARLA_OS_WASM
bool jackbridge_sem_timedwait(void* sem, uint msecs, bool) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_sem_timedwait(*(carla_sem_t*)sem, msecs);
#endif
}
#endif

// --------------------------------------------------------------------------------------------------------------------

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
    return carla_shm_map(*(carla_shm_t*)shm, static_cast<std::size_t>(size));
#endif
}

void jackbridge_shm_unmap(void* shm, void* ptr) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    return carla_shm_unmap(*(carla_shm_t*)shm, ptr);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

#if !defined(JACKBRIDGE_DUMMY) && defined(__WINE__)
static void discovery_pipe_callback(void*, const char* const msg) noexcept
{
    carla_stdout("discovery msgReceived %s", msg);
}
#endif

void* jackbridge_discovery_pipe_create(const char* argv[])
{
#if defined(JACKBRIDGE_DUMMY) || !defined(__WINE__)
    return nullptr;
    // unused
    (void)argv;
#else
    return carla_pipe_client_new(argv, discovery_pipe_callback, nullptr);
#endif
}

void jackbridge_discovery_pipe_message(void* pipe, const char* key, const char* value)
{
#if defined(JACKBRIDGE_DUMMY) || !defined(__WINE__)
    // unused
    (void)pipe;
    (void)key;
    (void)value;
#else
    carla_pipe_client_lock(pipe);
    carla_pipe_client_write_and_fix_msg(pipe, key);
    carla_pipe_client_write_and_fix_msg(pipe, value);
    carla_pipe_client_flush_and_unlock(pipe);
#endif
}

void jackbridge_discovery_pipe_destroy(void* pipe)
{
#if defined(JACKBRIDGE_DUMMY) || !defined(__WINE__)
    // unused
    (void)pipe;
#else
    carla_pipe_client_lock(pipe);
    carla_pipe_client_write_msg(pipe, "exiting\n");
    carla_pipe_client_flush_and_unlock(pipe);

    // NOTE: no more messages are handled after this point
    // pData->clientClosingDown = true;

    for (int i=0; i < 100 && carla_pipe_client_is_running(pipe); ++i)
    {
        d_msleep(50);
        carla_pipe_client_idle(pipe);
    }

    if (carla_pipe_client_is_running(pipe))
        carla_stderr2("jackbridge_discovery_pipe_destroy: pipe is still running!");

    carla_pipe_client_destroy(pipe);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

void jackbridge_parent_deathsig(bool kill) noexcept
{
#ifndef JACKBRIDGE_DUMMY
    carla_terminateProcessOnParentExit(kill);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
