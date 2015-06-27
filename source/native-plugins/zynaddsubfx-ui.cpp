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

#ifdef CARLA_OS_WIN
# define errx(...)
# define warnx(...)
#else
# include <err.h>
#endif

#define PLUGINVERSION
#define SOURCE_DIR "/usr/share/zynaddsubfx"
#undef override

#ifdef NTK_GUI
# include <dlfcn.h>

static CarlaString getResourceDir()
{
    Dl_info exeInfo;
    dladdr((void*)getResourceDir, &exeInfo);

    CarlaString filename(exeInfo.dli_fname);
    return filename.truncate(filename.rfind("-ui"));
}

CarlaString gUiPixmapPath(getResourceDir());
#endif

// base c-style headers
#include "zynaddsubfx/rtosc/rtosc.h"

// rtosc c includes
extern "C" {
#include "zynaddsubfx/rtosc/dispatch.c"
#include "zynaddsubfx/rtosc/rtosc.c"
}

// rtosc c++ includes
#include "zynaddsubfx/rtosc/cpp/midimapper.cpp"
#include "zynaddsubfx/rtosc/cpp/miditable.cpp"
#undef RTOSC_INVALID_MIDI
#undef MAX_UNHANDLED_PATH
#include "zynaddsubfx/rtosc/cpp/ports.cpp"
#undef __builtin_expect
#include "zynaddsubfx/rtosc/cpp/subtree-serialize.cpp"
#include "zynaddsubfx/rtosc/cpp/thread-link.cpp"
#undef off_t
#undef static
#include "zynaddsubfx/rtosc/cpp/undo-history.cpp"

// zynaddsubfx ui includes
#include "zynaddsubfx/UI/NioUI.cpp"
#include "zynaddsubfx/UI/WidgetPDial.cpp"
#include "zynaddsubfx/UI/ADnoteUI.cpp"
#include "zynaddsubfx/UI/BankUI.cpp"
#include "zynaddsubfx/UI/BankView.cpp"
#include "zynaddsubfx/UI/ConfigUI.cpp"
// #include "zynaddsubfx/UI/Connection.cpp"
#include "zynaddsubfx/UI/EffUI.cpp"
#include "zynaddsubfx/UI/EnvelopeFreeEdit.cpp"
#include "zynaddsubfx/UI/EnvelopeUI.cpp"
#include "zynaddsubfx/UI/FilterUI.cpp"
#include "zynaddsubfx/UI/Fl_EQGraph.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Button.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Check.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Choice.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Counter.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Dial.cpp"
#include "zynaddsubfx/UI/Fl_Osc_DialF.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Input.cpp"
#include "zynaddsubfx/UI/Fl_Osc_ListView.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Numeric_Input.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Output.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Pane.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Roller.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Slider.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Value.cpp"
#include "zynaddsubfx/UI/Fl_Osc_VSlider.cpp"
#include "zynaddsubfx/UI/Fl_Osc_Widget.cpp"
#include "zynaddsubfx/UI/Fl_Resonance_Graph.cpp"
#include "zynaddsubfx/UI/FormantFilterGraph.cpp"
#include "zynaddsubfx/UI/LFOUI.cpp"
#include "zynaddsubfx/UI/MasterUI.cpp"
#include "zynaddsubfx/UI/MicrotonalUI.cpp"
#include "zynaddsubfx/UI/OscilGenUI.cpp"
#include "zynaddsubfx/UI/PADnoteUI.cpp"
#include "zynaddsubfx/UI/PartNameButton.cpp"
#include "zynaddsubfx/UI/PartUI.cpp"
#include "zynaddsubfx/UI/PresetsUI.cpp"
#include "zynaddsubfx/UI/ResonanceUI.cpp"
#include "zynaddsubfx/UI/SUBnoteUI.cpp"
#include "zynaddsubfx/UI/VirKeyboard.cpp"
#include "zynaddsubfx/UI/guimain.cpp"

class ZynPipeClient : public CarlaPipeClient
{
public:
    ZynPipeClient() noexcept
        : CarlaPipeClient(),
          fQuitReceived(false) {}

    ~ZynPipeClient() noexcept override
    {
        if (fQuitReceived)
            return;

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("exiting\n");
        flushMessages();
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

            // TODO
            return true;
        }

        if (std::strcmp(msg, "program") == 0)
        {
            uint i;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(i), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(i), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(i), true);
            return true;
        }

        if (std::strcmp(msg, "show") == 0)
        {
            try {
                GUI::raiseUi(gui, "/show", "i", 1);
            } CARLA_SAFE_EXCEPTION("msgReceived show");
            return true;
        }

        if (std::strcmp(msg, "hide") == 0)
        {
            try {
                GUI::raiseUi(gui, "/hide", "");
            } CARLA_SAFE_EXCEPTION("msgReceived hide");
            return true;
        }

        if (std::strcmp(msg, "focus") == 0)
        {
            try {
                GUI::raiseUi(gui, "/focus", "");
            } CARLA_SAFE_EXCEPTION("msgReceived focus");
            return true;
        }

        if (std::strcmp(msg, "uiTitle") == 0)
        {
            const char* uiTitle;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uiTitle), true);

            try {
                MasterUI* const ui((MasterUI*)gui);
                ui->masterwindow->label(uiTitle);
            } CARLA_SAFE_EXCEPTION("msgReceived uiTitle");
            return true;
        }

        if (std::strcmp(msg, "quit") == 0)
        {
            fQuitReceived = true;

            try {
                GUI::raiseUi(gui, "/close-ui", "");
            } CARLA_SAFE_EXCEPTION("msgReceived quit");
            return true;
        }

        carla_stderr("ZynPipeClient::msgReceived : %s", msg);
        return false;
    }

private:
    bool fQuitReceived;
};

int main(int argc, const char* argv[])
{
    ZynPipeClient pipe;
    const char* uiTitle = nullptr;

    if (argc > 1)
    {
        sendtourl = argv[1];
        uiTitle   = argv[2];

        if (! pipe.initPipeClient(argv))
            return 1;

        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);
        lo_server_add_method(server, NULL, NULL, handler_function, 0);
    }

    gui = GUI::createUi(new UI_Interface(), &Pexitprogram);

    if (argc == 1)
        GUI::raiseUi(gui, "/show", "i", 1);

    if (uiTitle != nullptr)
    {
        MasterUI* const ui((MasterUI*)gui);
        ui->masterwindow->label(uiTitle);
    }

    for (; Pexitprogram == 0;)
    {
        if (server != nullptr) {
            for (; lo_server_recv_noblock(server, 0);) {}
        }

        pipe.idlePipe();
        GUI::tickUi(gui);
    }

    GUI::destroyUi(gui);
    gui = nullptr;
    return 0;
}

// --------------------------------------------------------------------------------------------
