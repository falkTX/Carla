/*
 * Carla process utils
 * Copyright (C) 2019-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaProcessUtils.hpp"

// --------------------------------------------------------------------------------------------------------------------
// process utility classes

#if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
ScopedAbortCatcher::ScopedAbortCatcher()
{
    s_triggered = false;
    s_oldsig = ::setjmp(s_env) == 0
             ? std::signal(SIGABRT, sig_handler)
             : nullptr;
}

ScopedAbortCatcher::~ScopedAbortCatcher()
{
    if (s_oldsig != nullptr && ! s_triggered)
        std::signal(SIGABRT, s_oldsig);
}

bool ScopedAbortCatcher::s_triggered = false;

jmp_buf ScopedAbortCatcher::s_env;
sig_t ScopedAbortCatcher::s_oldsig;

void ScopedAbortCatcher::sig_handler(const int signum)
{
    CARLA_SAFE_ASSERT_INT2_RETURN(signum == SIGABRT, signum, SIGABRT,);

    s_triggered = true;
    std::signal(signum, s_oldsig);
    std::longjmp(s_env, 1);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

CarlaSignalRestorer::CarlaSignalRestorer()
{
#if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
    carla_zeroStructs(sigs, 16);

    for (int i=0; i < 16; ++i)
        ::sigaction(i+1, nullptr, &sigs[i]);
#endif
}

CarlaSignalRestorer::~CarlaSignalRestorer()
{
#if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
    for (int i=0; i < 16; ++i)
        ::sigaction(i+1, &sigs[i], nullptr);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
