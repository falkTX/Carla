/*
 * Carla Utility Tests
 * Copyright (C) 2013-2016 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPipeUtils.hpp"
#include "juce_core/juce_core.h"

// -----------------------------------------------------------------------

class CarlaPipeClient2 : public CarlaPipeClient
{
public:
    CarlaPipeClient2()
        : CarlaPipeClient() {}

    bool msgReceived(const char* const msg) noexcept override
    {
        carla_stdout("CLIENT RECEIVED: \"%s\"", msg);
        return true;
    }
};

class CarlaPipeServer2 : public CarlaPipeServer
{
public:
    CarlaPipeServer2()
        : CarlaPipeServer() {}

    bool msgReceived(const char* const msg) noexcept override
    {
        carla_stdout("SERVER RECEIVED: \"%s\"", msg);
        return true;
    }
};

// -----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc != 1)
    {
        carla_stdout("CLIENT STARTED %i", argc);
        std::fflush(stdout);

        CarlaPipeClient2 p;
        const bool ok = p.initPipeClient(argv);
        CARLA_SAFE_ASSERT_RETURN(ok,1);

        p.lockPipe();
        p.writeMessage("CLIENT=>SERVER\n");
        p.writeAndFixMessage("heheheheheh");
        p.unlockPipe();

        carla_msleep(500);
        carla_stderr2("CLIENT idle start");
        std::fflush(stdout);
        p.idlePipe();
        carla_stderr2("CLIENT idle end");
        std::fflush(stdout);
        carla_msleep(500);
    }
    else
    {
        carla_stdout("SERVER STARTED %i", argc);

        using juce::File;
        using juce::String;
        String path = File(File::getSpecialLocation(File::currentExecutableFile)).getFullPathName();

        CarlaPipeServer2 p;
        const bool ok = p.startPipeServer(path.toRawUTF8(), "/home/falktx/Videos", "/home/falktx");
        CARLA_SAFE_ASSERT_RETURN(ok,1);

        p.lockPipe();
        p.writeMessage("SERVER=>CLIENT\n");
        p.unlockPipe();

        carla_msleep(500);
        carla_stderr2("SERVER idle start");
        p.idlePipe();
        carla_stderr2("SERVER idle end");
        carla_msleep(500);
    }

    return 0;
}

// -----------------------------------------------------------------------

#include "../utils/CarlaPipeUtils.cpp"

// -----------------------------------------------------------------------
