/*
 * Carla Pipe Tests
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

static bool gStopNow = false;

class ExternalPluginUI : public CarlaPipeServer
{
public:
    ExternalPluginUI()
    {
    }

    void fail(const char* const error) override
    {
        carla_stderr2(error);
    }

    bool msgReceived(const char* const msg) noexcept override
    {
        carla_stderr("msgReceived : %s", msg);

        if (std::strcmp(msg, "exiting") == 0)
        {
            waitChildClose();
            gStopNow = true;
        }

        return false;
    }
};

int main()
{
    ExternalPluginUI ui;

    if (! ui.start("/home/falktx/FOSS/GIT-mine/Carla/bin/resources/carla-plugin", "44100.0", "Ui title here"))
    {
        carla_stderr("failed to start");
        return 1;
    }

    ui.writeMsg("show\n", 5);

    for (int i=0; i < 500 && ! gStopNow; ++i)
    {
        ui.idle();
        carla_msleep(10);
    }

    return 0;
}
