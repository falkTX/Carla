/*
 * Carla Utility Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "../utils/CarlaPipeUtils.cpp"

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

int main(int argc, char* argv[])
{
    if (argc != 1)
    {
        carla_stdout("CLIENT STARTED %i", argc);

        CarlaPipeClient2 p;
        const bool ok = p.start(argv);
        CARLA_SAFE_ASSERT_RETURN(ok,1);

        p.lock();
        p.writeMsg("CLIENT=>SERVER\n");
        p.writeAndFixMsg("heheheheheh");
        p.unlock();

        carla_sleep(2);
        carla_stderr2("CLIENT idle start");
        p.idle();
        carla_stderr2("CLIENT idle end");
        carla_sleep(2);
    }
    else
    {
        carla_stdout("SERVER STARTED %i", argc);

        CarlaPipeServer2 p;
#ifdef CARLA_OS_WIN
        const bool ok = p.start("H:\\Source\\falkTX\\Carla\\source\\tests\\CarlaPipeUtils.exe", "/home/falktx/Videos", "/home/falktx");
#else
        const bool ok = p.start("/home/falktx/Source/falkTX/Carla/source/tests/CarlaPipeUtils", "/home/falktx/Videos", "/home/falktx");
#endif
        CARLA_SAFE_ASSERT_RETURN(ok,1);

        p.lock();
        p.writeMsg("SERVER=>CLIENT\n");
        p.unlock();

        carla_sleep(2);
        carla_stderr2("SERVER idle start");
        p.idle();
        carla_stderr2("SERVER idle end");
        carla_sleep(2);
    }

    return 0;
}

// -----------------------------------------------------------------------
