/*
 * Thread-safe fftw
 * Copyright (C) 2018-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef THREAD_SAFE_FFTW_HPP_INCLUDED
#define THREAD_SAFE_FFTW_HPP_INCLUDED

#include "CarlaDefines.h"

#ifdef CARLA_OS_UNIX
// --------------------------------------------------------------------------------------------------------------------
// Thread-safe fftw

#include "CarlaLibUtils.hpp"

class ThreadSafeFFTW
{
public:
    typedef void (*void_func)(void);

    ThreadSafeFFTW()
        : libfftw3(nullptr),
          libfftw3f(nullptr),
          libfftw3l(nullptr),
          libfftw3q(nullptr)
    {
        if ((libfftw3 = lib_open("libfftw3_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3, "fftw_make_planner_thread_safe"))
                func();

        if ((libfftw3f = lib_open("libfftw3f_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3f, "fftwf_make_planner_thread_safe"))
                func();

        if ((libfftw3l = lib_open("libfftw3l_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3l, "fftwl_make_planner_thread_safe"))
                func();

        if ((libfftw3q = lib_open("libfftw3q_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3q, "fftwq_make_planner_thread_safe"))
                func();
    }

    ~ThreadSafeFFTW()
    {
        if (libfftw3 != nullptr)
        {
            lib_close(libfftw3);
            libfftw3 = nullptr;
        }

        if (libfftw3f != nullptr)
        {
            lib_close(libfftw3f);
            libfftw3f = nullptr;
        }

        if (libfftw3l != nullptr)
        {
            lib_close(libfftw3l);
            libfftw3l = nullptr;
        }

        if (libfftw3q != nullptr)
        {
            lib_close(libfftw3q);
            libfftw3q = nullptr;
        }
    }

private:
    lib_t libfftw3;
    lib_t libfftw3f;
    lib_t libfftw3l;
    lib_t libfftw3q;

    CARLA_DECLARE_NON_COPYABLE(ThreadSafeFFTW)
};
#endif

// --------------------------------------------------------------------------------------------------------------------

#endif // THREAD_SAFE_FFTW_HPP_INCLUDED
