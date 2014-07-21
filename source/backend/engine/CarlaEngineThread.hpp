/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_THREAD_HPP_INCLUDED
#define CARLA_ENGINE_THREAD_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaThread.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// CarlaEngineThread

class CarlaEngineThread : public CarlaThread
{
public:
    CarlaEngineThread(CarlaEngine* const engine) noexcept;
    ~CarlaEngineThread() noexcept override;

protected:
    void run() noexcept override;

private:
    CarlaEngine* const kEngine;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineThread)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_THREAD_HPP_INCLUDED
