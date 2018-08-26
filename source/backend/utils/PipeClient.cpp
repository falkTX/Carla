/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaPipeUtils.hpp"

namespace CB = CarlaBackend;

// -------------------------------------------------------------------------------------------------------------------

class ExposedCarlaPipeClient : public CarlaPipeClient
{
public:
    ExposedCarlaPipeClient(const CarlaPipeCallbackFunc callbackFunc, void* const callbackPtr) noexcept
        : CarlaPipeClient(),
          fCallbackFunc(callbackFunc),
          fCallbackPtr(callbackPtr),
          fLastReadLine(nullptr)
    {
        CARLA_SAFE_ASSERT(fCallbackFunc != nullptr);
    }

    ~ExposedCarlaPipeClient() override
    {
        if (fLastReadLine != nullptr)
        {
            delete[] fLastReadLine;
            fLastReadLine = nullptr;
        }
    }

    const char* readlineblock(const uint timeout) noexcept
    {
        delete[] fLastReadLine;
        fLastReadLine = CarlaPipeClient::_readlineblock(timeout);
        return fLastReadLine;
    }

    bool msgReceived(const char* const msg) noexcept override
    {
        if (fCallbackFunc != nullptr)
        {
            try {
                fCallbackFunc(fCallbackPtr, msg);
            } CARLA_SAFE_EXCEPTION("msgReceived");
        }

        return true;
    }

private:
    const CarlaPipeCallbackFunc fCallbackFunc;
    void* const fCallbackPtr;
    const char* fLastReadLine;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExposedCarlaPipeClient)
};

CarlaPipeClientHandle carla_pipe_client_new(const char* argv[], CarlaPipeCallbackFunc callbackFunc, void* callbackPtr)
{
    carla_debug("carla_pipe_client_new(%p, %p, %p)", argv, callbackFunc, callbackPtr);

    ExposedCarlaPipeClient* const pipe = new ExposedCarlaPipeClient(callbackFunc, callbackPtr);

    if (! pipe->initPipeClient(argv))
    {
        delete pipe;
        return nullptr;
    }

    return pipe;
}

void carla_pipe_client_idle(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

    ((ExposedCarlaPipeClient*)handle)->idlePipe();
}

bool carla_pipe_client_is_running(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((ExposedCarlaPipeClient*)handle)->isPipeRunning();
}

void carla_pipe_client_lock(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

    return ((ExposedCarlaPipeClient*)handle)->lockPipe();
}

const char* carla_pipe_client_readlineblock(CarlaPipeClientHandle handle, uint timeout)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    return ((ExposedCarlaPipeClient*)handle)->readlineblock(timeout);
}

bool carla_pipe_client_write_msg(CarlaPipeClientHandle handle, const char* msg)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((ExposedCarlaPipeClient*)handle)->writeMessage(msg);
}

bool carla_pipe_client_write_and_fix_msg(CarlaPipeClientHandle handle, const char* msg)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((ExposedCarlaPipeClient*)handle)->writeAndFixMessage(msg);
}

bool carla_pipe_client_flush(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((ExposedCarlaPipeClient*)handle)->flushMessages();
}

bool carla_pipe_client_flush_and_unlock(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    ExposedCarlaPipeClient* const pipe = (ExposedCarlaPipeClient*)handle;
    const bool ret = pipe->flushMessages();
    pipe->unlockPipe();
    return ret;
}

void carla_pipe_client_destroy(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
    carla_debug("carla_pipe_client_destroy(%p)", handle);

    ExposedCarlaPipeClient* const pipe = (ExposedCarlaPipeClient*)handle;
    pipe->closePipeClient();
    delete pipe;
}

// -------------------------------------------------------------------------------------------------------------------

#include "CarlaPipeUtils.cpp"
#include "water/misc/Time.cpp"

// -------------------------------------------------------------------------------------------------------------------
