/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoUIInternal.hpp"

#ifdef DISTRHO_UI_EXTERNAL
# error DSSI always uses external UI, no wrapper neeed!
#endif

#include <lo/lo.h>

#ifdef DISTRHO_UI_QT
# include <QtGui/QApplication>
# include <QtGui/QMainWindow>
#else
# include "dgl/App.hpp"
# include "dgl/Window.hpp"
#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------

struct OscData {
    lo_address  addr;
    const char* path;
    lo_server   server;
};

void osc_send_configure(const OscData* oscData, const char* const key, const char* const value)
{
    char targetPath[strlen(oscData->path)+11];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/configure");
    lo_send(oscData->addr, targetPath, "ss", key, value);
}

void osc_send_control(const OscData* oscData, const int32_t index, const float value)
{
    char targetPath[strlen(oscData->path)+9];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/control");
    lo_send(oscData->addr, targetPath, "if", index, value);
}

void osc_send_midi(const OscData* oscData, unsigned char data[4])
{
    char targetPath[strlen(oscData->path)+6];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/midi");
    lo_send(oscData->addr, targetPath, "m", data);
}

void osc_send_update(const OscData* oscData, const char* const url)
{
    char targetPath[strlen(oscData->path)+8];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/update");
    lo_send(oscData->addr, targetPath, "s", url);
}

void osc_send_exiting(const OscData* oscData)
{
    char targetPath[strlen(oscData->path)+9];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/exiting");
    lo_send(oscData->addr, targetPath, "");
}

// -------------------------------------------------

#ifdef DISTRHO_UI_QT
class UIDssi : public QMainWindow
#else
class UIDssi
#endif
{
public:
    UIDssi(const OscData* const oscData, const char* const uiTitle)
#ifdef DISTRHO_UI_QT
        : QMainWindow(nullptr),
          fUI(this, 0, nullptr, setParameterCallback, setStateCallback, uiSendNoteCallback, uiResizeCallback),
          fHostClosed(false),
          qtTimer(0),
#else
        : fUI(this, 0, nullptr, setParameterCallback, setStateCallback, uiSendNoteCallback, uiResizeCallback),
          fHostClosed(false),
          glApp(fUI.getApp()),
          glWindow(fUI.getWindow()),
#endif
          kOscData(oscData)
    {
#ifdef DISTRHO_UI_QT
        QtUI* const qtUI(fUI.getQt4Ui());

        setCentralWidget(qtUI);
        setWindowTitle(uiTitle);

        if (qtUI->d_resizable())
        {
            adjustSize();
        }
        else
        {
            setFixedSize(qtUI->width(), qtUI->height());
# ifdef DISTRHO_OS_WINDOWS
            setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint);
# endif
        }

        qtTimer = startTimer(30);
#else
        glWindow.setSize(fUI.width(), fUI.height());
        glWindow.setWindowTitle(uiTitle);
#endif
    }

    ~UIDssi()
    {
#ifdef DISTRHO_UI_QT
        if (qtTimer != 0)
        {
            killTimer(qtTimer);
            qtTimer = 0;
        }
#endif
        if (kOscData->server && ! fHostClosed)
            osc_send_exiting(kOscData);
    }

#ifndef DISTRHO_UI_QT
    void glExec()
    {
        while (! glApp.isQuiting())
        {
            if (kOscData->server != nullptr)
            {
                while (lo_server_recv_noblock(kOscData->server, 0) != 0) {}
            }

            glApp.idle();

            dgl_msleep(10);
        }
    }
#endif

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_STATE
    void dssiui_configure(const char* key, const char* value)
    {
        fUI.stateChanged(key, value);
    }
#endif

    void dssiui_control(unsigned long index, float value)
    {
        fUI.parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void dssiui_program(unsigned long bank, unsigned long program)
    {
        fUI.programChanged(bank * 128 + program);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void dssiui_midi(uint8_t data[4])
    {
        uint8_t status  = data[1] & 0xF0;
        uint8_t channel = data[1] & 0x0F;

        // fix bad note-off
        if (status == 0x90 && data[3] == 0)
            status -= 0x10;

        if (status == 0x80)
        {
            uint8_t note = data[2];
            fUI.noteReceived(false, channel, note, 0);
        }
        else if (status == 0x90)
        {
            uint8_t note = data[2];
            uint8_t velo = data[3];
            fUI.noteReceived(true, channel, note, velo);
        }
    }
#endif

    void dssiui_show()
    {
#ifdef DISTRHO_UI_QT
        show();
#else
        glWindow.show();
#endif
    }

    void dssiui_hide()
    {
#ifdef DISTRHO_UI_QT
        hide();
#else
        glWindow.hide();
#endif
    }

    void dssiui_quit()
    {
        fHostClosed = true;

#ifdef DISTRHO_UI_QT
        if (qtTimer != 0)
        {
            killTimer(qtTimer);
            qtTimer = 0;
        }
        close();
        qApp->quit();
#else
        glWindow.close();
        glApp.quit();
#endif
    }

    // ---------------------------------------------

protected:
    void setParameterValue(uint32_t rindex, float value)
    {
        if (kOscData->server == nullptr)
            return;

        osc_send_control(kOscData, rindex, value);
    }

    void setState(const char* key, const char* value)
    {
        if (kOscData->server == nullptr)
            return;

        osc_send_configure(kOscData, key, value);
    }

    void uiSendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if (kOscData->server == nullptr)
            return;
        if (channel > 0xF)
            return;

        uint8_t mdata[4] = { 0, channel, note, velocity };
        mdata[1] += onOff ? 0x90 : 0x80;

        osc_send_midi(kOscData, mdata);
    }

    void uiResize(unsigned int width, unsigned int height)
    {
#ifdef DISTRHO_UI_QT
        setFixedSize(width, height);
#else
        glWindow.setSize(width, height);
#endif
    }

#ifdef DISTRHO_UI_QT
    void timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == uiTimer)
        {
            fUI.idle();

            while (lo_server_recv_noblock(kOscData->server, 0) != 0) {}
        }

        QMainWindow::timerEvent(event);
    }
#endif

private:
    // Plugin UI
    UIInternal fUI;
    bool fHostClosed;

#ifdef DISTRHO_UI_QT
    // Qt4 stuff
    int qtTimer;
#else
    // OpenGL stuff
    App& glApp;
    Window& glWindow;
#endif

    const OscData* const kOscData;

    // ---------------------------------------------
    // Callbacks

    #define uiPtr ((UIDssi*)ptr)

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        uiPtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        uiPtr->setState(key, value);
    }

    static void uiSendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uiPtr->uiSendNote(onOff, channel, note, velocity);
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        uiPtr->uiResize(width, height);
    }

    #undef uiPtr
};

// -------------------------------------------------

static UIDssi*     globalUI = nullptr;
static OscData     gOscData = { nullptr, nullptr, nullptr };
static const char* gUiTitle = nullptr;

static void initUiIfNeeded()
{
    if (globalUI != nullptr)
        return;

    if (d_lastUiSampleRate == 0.0)
        d_lastUiSampleRate = 44100.0;

    globalUI = new UIDssi(&gOscData, gUiTitle);
}

// -------------------------------------------------

int osc_debug_handler(const char* path, const char*, lo_arg**, int, lo_message, void*)
{
    d_debug("osc_debug_handler(\"%s\")", path);
    return 0;
}

void osc_error_handler(int num, const char* msg, const char* path)
{
    d_stderr("osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
}

#if DISTRHO_PLUGIN_WANT_STATE
int osc_configure_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const char* const key   = &argv[0]->s;
    const char* const value = &argv[1]->s;
    d_debug("osc_configure_handler(\"%s\", \"%s\")", key, value);

    initUiIfNeeded();

    globalUI->dssiui_configure(key, value);

    return 0;
}
#endif

int osc_control_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t rindex = argv[0]->i;
    const float   value  = argv[1]->f;
    d_debug("osc_control_handler(%i, %f)", rindex, value);

    int32_t index = rindex - DISTRHO_PLUGIN_NUM_INPUTS - DISTRHO_PLUGIN_NUM_OUTPUTS;

    // latency
#if DISTRHO_PLUGIN_WANT_LATENCY
    index -= 1;
#endif

    if (index < 0)
        return 0;

    initUiIfNeeded();

    globalUI->dssiui_control(index, value);

    return 0;
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
int osc_program_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t bank    = argv[0]->i;
    const int32_t program = argv[1]->f;
    d_debug("osc_program_handler(%i, %i)", bank, program);

    initUiIfNeeded();

    globalUI->dssiui_program(bank, program);

    return 0;
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
int osc_midi_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    d_debug("osc_midi_handler()");

    initUiIfNeeded();

    globalUI->dssiui_midi(argv[0]->m);

    return 0;
}
#endif

int osc_sample_rate_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t sampleRate = argv[0]->i;
    d_debug("osc_sample_rate_handler(%i)", sampleRate);

    d_lastUiSampleRate = sampleRate;

    return 0;
}

int osc_show_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    d_debug("osc_show_handler()");

    initUiIfNeeded();

    globalUI->dssiui_show();

    return 0;
}

int osc_hide_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    d_debug("osc_hide_handler()");

    if (globalUI != nullptr)
       globalUI->dssiui_hide();

    return 0;
}

int osc_quit_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    d_debug("osc_quit_handler()");

    if (globalUI != nullptr)
        globalUI->dssiui_quit();
#ifdef DISTRHO_UI_QT
    else if (QApplication* app = qApp)
        app->quit();
#endif

    return 0;
}

END_NAMESPACE_DISTRHO

int main(int argc, char* argv[])
{
    USE_NAMESPACE_DISTRHO

    // dummy test mode
    if (argc == 1)
    {
#ifdef DISTRHO_UI_QT
        QApplication app(argc, argv, true);
#endif

        gUiTitle = strdup("DSSI UI Test");

        initUiIfNeeded();
        globalUI->dssiui_show();

#ifdef DISTRHO_UI_QT
        app.exec();
#else
        globalUI->glExec();
#endif

        free((void*)gUiTitle);

        return 0;
    }

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <osc-url> <plugin-dll> <plugin-label> <instance-name>\n", argv[0]);
        return 1;
    }

#ifdef DISTRHO_UI_QT
    QApplication app(argc, argv, true);
#endif

    const char* oscUrl  = argv[1];
    const char* uiTitle = argv[4];

    char* const oscHost = lo_url_get_hostname(oscUrl);
    char* const oscPort = lo_url_get_port(oscUrl);
    char* const oscPath = lo_url_get_path(oscUrl);
    size_t  oscPathSize = strlen(oscPath);
    lo_address  oscAddr = lo_address_new(oscHost, oscPort);
    lo_server oscServer = lo_server_new_with_proto(nullptr, LO_UDP, osc_error_handler);

    char* const oscServerPath = lo_server_get_url(oscServer);

    char pluginPath[strlen(oscServerPath)+oscPathSize];
    strcpy(pluginPath, oscServerPath);
    strcat(pluginPath, oscPath+1);

#if DISTRHO_PLUGIN_WANT_STATE
    char oscPathConfigure[oscPathSize+11];
    strcpy(oscPathConfigure, oscPath);
    strcat(oscPathConfigure, "/configure");
    lo_server_add_method(oscServer, oscPathConfigure, "ss", osc_configure_handler, nullptr);
#endif

    char oscPathControl[oscPathSize+9];
    strcpy(oscPathControl, oscPath);
    strcat(oscPathControl, "/control");
    lo_server_add_method(oscServer, oscPathControl, "if", osc_control_handler, nullptr);

    d_stdout("oscServerPath:  \"%s\"", oscServerPath);
    d_stdout("pluginPath:     \"%s\"", pluginPath);
    d_stdout("oscPathControl: \"%s\"", oscPathControl);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    char oscPathProgram[oscPathSize+9];
    strcpy(oscPathProgram, oscPath);
    strcat(oscPathProgram, "/program");
    lo_server_add_method(oscServer, oscPathProgram, "ii", osc_program_handler, nullptr);
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    char oscPathMidi[oscPathSize+6];
    strcpy(oscPathMidi, oscPath);
    strcat(oscPathMidi, "/midi");
    lo_server_add_method(oscServer, oscPathMidi, "m", osc_midi_handler, nullptr);
#endif

    char oscPathSampleRate[oscPathSize+13];
    strcpy(oscPathSampleRate, oscPath);
    strcat(oscPathSampleRate, "/sample-rate");
    lo_server_add_method(oscServer, oscPathSampleRate, "i", osc_sample_rate_handler, nullptr);

    char oscPathShow[oscPathSize+6];
    strcpy(oscPathShow, oscPath);
    strcat(oscPathShow, "/show");
    lo_server_add_method(oscServer, oscPathShow, "", osc_show_handler, nullptr);

    char oscPathHide[oscPathSize+6];
    strcpy(oscPathHide, oscPath);
    strcat(oscPathHide, "/hide");
    lo_server_add_method(oscServer, oscPathHide, "", osc_hide_handler, nullptr);

    char oscPathQuit[oscPathSize+6];
    strcpy(oscPathQuit, oscPath);
    strcat(oscPathQuit, "/quit");
    lo_server_add_method(oscServer, oscPathQuit, "", osc_quit_handler, nullptr);

    lo_server_add_method(oscServer, nullptr, nullptr, osc_debug_handler, nullptr);

    gOscData = { oscAddr, oscPath, oscServer };
    gUiTitle = uiTitle;

    osc_send_update(&gOscData, pluginPath);

    // wait for init
    for (int i=0; i < 1000; ++i)
    {
        lo_server_recv(oscServer);

        if (d_lastUiSampleRate != 0.0 || globalUI != nullptr)
            break;

        d_msleep(50);
    }

    int ret = 1;

    if (d_lastUiSampleRate != 0.0 || globalUI != nullptr)
    {
        initUiIfNeeded();

#ifdef DISTRHO_UI_QT
        app.exec();
#else
        globalUI->glExec();
#endif

        delete globalUI;
        globalUI = nullptr;

        ret = 0;
    }

#if DISTRHO_PLUGIN_WANT_STATE
    lo_server_del_method(oscServer, oscPathConfigure, "ss");
#endif
    lo_server_del_method(oscServer, oscPathControl, "if");
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    lo_server_del_method(oscServer, oscPathProgram, "ii");
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    lo_server_del_method(oscServer, oscPathMidi, "m");
#endif
    lo_server_del_method(oscServer, oscPathSampleRate, "i");
    lo_server_del_method(oscServer, oscPathShow, "");
    lo_server_del_method(oscServer, oscPathHide, "");
    lo_server_del_method(oscServer, oscPathQuit, "");
    lo_server_del_method(oscServer, nullptr, nullptr);

    free(oscServerPath);
    free(oscHost);
    free(oscPort);
    free(oscPath);

    lo_address_free(oscAddr);
    lo_server_free(oscServer);

    return ret;
}
