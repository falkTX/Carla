/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPipeUtils.cpp"

#include "zita-rev1/png2img.cc"
#include "zita-rev1/guiclass.cc"
#include "zita-rev1/mainwin.cc"
#include "zita-rev1/rotary.cc"
#include "zita-rev1/styles.cc"

using namespace REV1;

static Mainwin* mainwin = nullptr;

// --------------------------------------------------------------------------------------------

class ZitaPipeClient : public CarlaPipeClient,
                       public Mainwin::ValueChangedCallback
{
public:
    ZitaPipeClient() noexcept
        : CarlaPipeClient(),
          fQuitReceived(false) {}

    ~ZitaPipeClient() noexcept override
    {
        if (fQuitReceived || ! isPipeRunning())
            return;

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("exiting\n");
        flushMessages();
    }

    bool quitRequested() const noexcept
    {
        return fQuitReceived;
    }

protected:
    bool msgReceived(const char* const msg) noexcept override
    {
        if (std::strcmp(msg, "control") == 0)
        {
            uint index;
            float value;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (index == Mainwin::R_OPMIX && mainwin->_ambis)
                index = Mainwin::R_RGXYZ;

            mainwin->_rotary[index]->set_value(value);
            return true;
        }

        if (std::strcmp(msg, "show") == 0)
        {
            mainwin->x_map();
            return true;
        }

        if (std::strcmp(msg, "hide") == 0)
        {
            mainwin->x_unmap();
            return true;
        }

        if (std::strcmp(msg, "focus") == 0)
        {
            mainwin->x_mapraised();
            return true;
        }

        if (std::strcmp(msg, "uiTitle") == 0)
        {
            const char* uiTitle;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uiTitle), true);

            mainwin->x_set_title(uiTitle);
            return true;
        }

        if (std::strcmp(msg, "quit") == 0)
        {
            fQuitReceived = true;
            mainwin->stop();
            return true;
        }

        carla_stderr("ZitaPipeClient::msgReceived : %s", msg);
        return false;
    }

    void valueChangedCallback(uint index, double value) override
    {
        if (index == Mainwin::R_RGXYZ)
            index = Mainwin::R_OPMIX;

        if (isPipeRunning())
            writeControlMessage(index, value);
    }

private:
    bool fQuitReceived;
};

// --------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    X_resman   xresman;
    X_display *display;
    X_handler *handler;
    X_rootwin *rootwin;
    int        ev, xp, yp, xs, ys;

    int   fake_argc   = 1;
    char* fake_argv[] = { (char*)"rev1" };
    xresman.init(&fake_argc, fake_argv, (char*)"rev1", nullptr, 0);

    display = new X_display(nullptr);
    if (display->dpy () == 0)
    {
        carla_stderr("Can't open display.");
        delete display;
        return 1;
    }

    ZitaPipeClient pipe;
    bool ambisonic      = false;
    const char* uiTitle = "Test UI";

    if (argc > 1)
    {
        ambisonic = std::strcmp(argv[1], "true") == 0;
        uiTitle   = argv[2];

        if (! pipe.initPipeClient(argv))
            return 1;
    }

    xp = yp = 100;
    xs = Mainwin::XSIZE + 4;
    ys = Mainwin::YSIZE + 30;
    xresman.geometry(".geometry", display->xsize(), display->ysize(), 1, xp, yp, xs, ys);

    styles_init(display, &xresman);
    rootwin = new X_rootwin(display);
    mainwin = new Mainwin(rootwin, &xresman, xp, yp, ambisonic, &pipe);
    mainwin->x_set_title(uiTitle);
    rootwin->handle_event();
    handler = new X_handler(display, mainwin, EV_X11);
    handler->next_event();
    XFlush(display->dpy());

    if (argc == 1)
        mainwin->x_map();

    do
    {
        ev = mainwin->process();
        if (ev == EV_X11)
        {
            rootwin->handle_event();
            handler->next_event();
        }
        else if (ev == Esync::EV_TIME)
        {
            handler->next_event();

            if (pipe.isPipeRunning())
                pipe.idlePipe();
        }
    }
    while (ev != EV_EXIT && ! pipe.quitRequested());

    styles_fini(display);
    delete handler;
    delete rootwin;
    delete display;

    return 0;
}

// --------------------------------------------------------------------------------------------
