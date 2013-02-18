/*
 * Carla Engine Thread
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_ENGINE_THREAD_HPP__
#define __CARLA_ENGINE_THREAD_HPP__

#include "CarlaBackend.hpp"
#include "CarlaMutex.hpp"
#include "CarlaThread.hpp"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

class CarlaEngineThread : public CarlaThread
{
public:
    CarlaEngineThread(CarlaEngine* const engine);
    ~CarlaEngineThread();

    void startNow();
    void stopNow();

    // ----------------------------------------------

protected:
    void run();

    // ----------------------------------------------

private:
    CarlaEngine* const kEngine;

    CarlaMutex fMutex;
    bool       fStopNow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineThread)
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_ENGINE_THREAD_HPP__
