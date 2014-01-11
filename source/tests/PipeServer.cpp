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

    ~ExternalPluginUI() override
    {
    }

    void fail(const char* const error) override
    {
        carla_stderr2(error);
        //fHost->dispatcher(fHost->handle, HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
    }

    void msgReceived(const char* const msg) override
    {
        carla_stderr("msgReceived : %s", msg);

        if (std::strcmp(msg, "exiting") == 0)
        {
            waitChildClose();
            gStopNow = true;
        }

#if 0
        if (std::strcmp(msg, "control") == 0)
        {
            int index;
            float value;

            if (readNextLineAsInt(index) && readNextLineAsFloat(value))
                handleSetParameterValue(index, value);
        }
        else if (std::strcmp(msg, "configure") == 0)
        {
            char* key;
            char* value;

            if (readNextLineAsString(key) && readNextLineAsString(value))
            {
                handleSetState(key, value);
                std::free(key);
                std::free(value);
            }
        }
        else if (std::strcmp(msg, "exiting") == 0)
        {
            waitChildClose();
            fHost->ui_closed(fHost->handle);
        }
        else
        {
            carla_stderr("unknown message HOST: \"%s\"", msg);
        }
#endif
    }
};

int main()
{
    ExternalPluginUI ui;

    if (! ui.start("/home/falktx/FOSS/GIT-mine/Carla/source/notes-ui", "44100.0", "Ui title here"))
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

    //ui.stop();

    return 0;
}
