/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#if defined(DISTRHO_PLUGIN_TARGET_DSSI) && DISTRHO_PLUGIN_HAS_UI

#include "DistrhoUIInternal.h"

#include <QtCore/QSettings>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

#include <lo/lo.h>

// -------------------------------------------------

START_NAMESPACE_DISTRHO

struct OscData {
    lo_address  addr;
    const char* path;
    lo_server   server;
};

struct StringData {
    d_string key;
    d_string value;
};

#if DISTRHO_PLUGIN_WANT_STATE
void osc_send_configure(const OscData* oscData, const char* const key, const char* const value)
{
    char targetPath[strlen(oscData->path)+11];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/configure");
    lo_send(oscData->addr, targetPath, "ss", key, value);
}
#endif

void osc_send_control(const OscData* oscData, const int32_t index, const float value)
{
    char targetPath[strlen(oscData->path)+9];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/control");
    lo_send(oscData->addr, targetPath, "if", index, value);
}

#if DISTRHO_PLUGIN_IS_SYNTH
void osc_send_midi(const OscData* oscData, unsigned char data[4])
{
    char targetPath[strlen(oscData->path)+6];
    strcpy(targetPath, oscData->path);
    strcat(targetPath, "/midi");
    lo_send(oscData->addr, targetPath, "m", data);
}
#endif

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

// stuff that we might receive while waiting for sample-rate
static bool globalShow = false;

class UIDssi : public QMainWindow
{
public:
    UIDssi(const OscData* oscData_, const char* title)
        : QMainWindow(nullptr),
          widget(this),
          settings("DISTRHO", DISTRHO_PLUGIN_NAME),
          ui(this, widget.winId(), setParameterCallback, setStateCallback, nullptr, uiSendNoteCallback, uiResizeCallback),
          oscData(oscData_)
    {
        setCentralWidget(&widget);
        setFixedSize(ui.getWidth(), ui.getHeight());
        setWindowTitle(title);

        uiTimer = startTimer(30);

        // load settings
        restoreGeometry(settings.value("Global/Geometry", QByteArray()).toByteArray());

#if DISTRHO_PLUGIN_WANT_STATE
        for (size_t i=0; i < globalConfigures.size(); i++)
            dssiui_configure(globalConfigures.at(i).key, globalConfigures.at(i).value);
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (globalProgram[0] >= 0 && globalProgram[1] >= 0)
            dssiui_program(globalProgram[0], globalProgram[1]);
#endif

        for (size_t i=0; i < globalControls.size(); i++)
            dssiui_control(i, globalControls.at(i));

        if (globalShow)
            show();
    }

    ~UIDssi()
    {
        // save settings
        settings.setValue("Global/Geometry", saveGeometry());

        if (uiTimer)
        {
            killTimer(uiTimer);
            osc_send_exiting(oscData);
        }
    }

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_STATE
    void dssiui_configure(const char* key, const char* value)
    {
        ui.stateChanged(key, value);
    }
#endif

    void dssiui_control(unsigned long index, float value)
    {
        ui.parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void dssiui_program(unsigned long bank, unsigned long program)
    {
        unsigned long index = bank * 128 + program;
        ui.programChanged(index);
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
            ui.noteReceived(false, channel, note, 0);
        }
        else if (status == 0x90)
        {
            uint8_t note = data[2];
            uint8_t velo = data[3];
            ui.noteReceived(true, channel, note, velo);
        }
    }
#endif

    void dssiui_quit()
    {
        if (uiTimer)
        {
            killTimer(uiTimer);
            uiTimer = 0;
        }
        close();
        qApp->quit();
    }

    // ---------------------------------------------

protected:
    void setParameterValue(uint32_t rindex, float value)
    {
        osc_send_control(oscData, rindex, value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* key, const char* value)
    {
        osc_send_configure(oscData, key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void uiSendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uint8_t mdata[4] = { 0, channel, note, velocity };
        mdata[1] += onOff ? 0x90 : 0x80;

        osc_send_midi(oscData, mdata);
    }
#endif

    void uiResize(unsigned int width, unsigned int height)
    {
        widget.setFixedSize(width, height);
        setFixedSize(width, height);
    }

    void timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == uiTimer)
        {
            ui.idle();

            while (lo_server_recv_noblock(oscData->server, 0) != 0) {}
        }

        QMainWindow::timerEvent(event);
    }

private:
    // Qt4 stuff
    int uiTimer;
    QWidget widget;
    QSettings settings;

    // Plugin UI
    UIInternal ui;

    const OscData* const oscData;

    // ---------------------------------------------
    // Callbacks

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        UIDssi* _this_ = (UIDssi*)ptr;
        assert(_this_);

        _this_->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        UIDssi* _this_ = (UIDssi*)ptr;
        assert(_this_);

        _this_->setState(key, value);
#else
        Q_UNUSED(ptr);
        Q_UNUSED(key);
        Q_UNUSED(value);
#endif
    }

    static void uiSendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        UIDssi* _this_ = (UIDssi*)ptr;
        assert(_this_);

        _this_->uiSendNote(onOff, channel, note, velocity);
#else
        Q_UNUSED(ptr);
        Q_UNUSED(onOff);
        Q_UNUSED(channel);
        Q_UNUSED(note);
        Q_UNUSED(velocity);
#endif
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        UIDssi* _this_ = (UIDssi*)ptr;
        assert(_this_);

        _this_->uiResize(width, height);
    }
};

// -------------------------------------------------

static UIDssi* globalUI = nullptr;

void osc_error_handler(int num, const char* msg, const char* path)
{
    qCritical("osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
}

#if DISTRHO_PLUGIN_WANT_STATE
int osc_configure_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const char* const key   = &argv[0]->s;
    const char* const value = &argv[1]->s;
    qDebug("osc_configure_handler(\"%s\", \"%s\")", key, value);

    if (globalUI)
    {
        globalUI->dssiui_configure(key, value);
    }
    else
    {
        StringData data = { key, value };
        globalConfigures.push_back(data);
    }

    return 0;
}
#endif

int osc_control_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t rindex = argv[0]->i;
    const float   value  = argv[1]->f;
    qDebug("osc_control_handler(%i, %f)", rindex, value);

    int32_t index = rindex - DISTRHO_PLUGIN_NUM_INPUTS - DISTRHO_PLUGIN_NUM_OUTPUTS;

    // latency
#if DISTRHO_PLUGIN_WANT_LATENCY
    index -= 1;
#endif
    // sample-rate
    index -= 1;

    if (index == -1)
    {
        setLastUiSampleRate(value);
        return 0;
    }

    if (index < 0)
        return 0;

    if (globalUI)
    {
        globalUI->dssiui_control(index, value);
    }
    else
    {
        if (index >= (int32_t)globalControls.size())
        {
            for (int32_t i=globalControls.size(); i < index; i++)
                globalControls.push_back(0.0f);
        }

        globalControls[index] = value;
    }

    return 0;
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
int osc_program_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t bank    = argv[0]->i;
    const int32_t program = argv[1]->f;
    qDebug("osc_program_handler(%i, %i)", bank, program);

    if (globalUI)
    {
        globalUI->dssiui_program(bank, program);
    }
    else
    {
        globalProgram[0] = bank;
        globalProgram[1] = program;
    }

    return 0;
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
int osc_midi_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    qDebug("osc_midi_handler()");

    if (globalUI)
        globalUI->dssiui_midi(argv[0]->m);

    return 0;
}
#endif

int osc_sample_rate_handler(const char*, const char*, lo_arg** argv, int, lo_message, void*)
{
    const int32_t sampleRate = argv[0]->i;
    qDebug("osc_sample_rate_handler(%i)", sampleRate);

    setLastUiSampleRate(sampleRate);

    return 0;
}

int osc_show_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    qDebug("osc_show_handler()");

    if (globalUI)
        globalUI->show();

    return 0;
}

int osc_hide_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    qDebug("osc_hide_handler()");

    if (globalUI)
        globalUI->hide();

    return 0;
}

int osc_quit_handler(const char*, const char*, lo_arg**, int, lo_message, void*)
{
    qDebug("osc_quit_handler()");

    if (globalUI)
        globalUI->dssiui_quit();

    return 0;
}

int osc_debug_handler(const char* path, const char*, lo_arg**, int, lo_message, void*)
{
    qDebug("osc_debug_handler(\"%s\")", path);

    return 0;
}

END_NAMESPACE_DISTRHO

// -------------------------------------------------

int main(int argc, char* argv[])
{
    USE_NAMESPACE_DISTRHO

    if (argc != 5)
    {
        qWarning("Usage: %s <osc-url> <so-file> <plugin-label> <instance-name>", argv[0]);
        return 1;
    }

    QApplication app(argc, argv, true);

    const char* const oscUrl  = argv[1];
    const char* const uiTitle = argv[4];

    char* const oscHost = lo_url_get_hostname(oscUrl);
    char* const oscPort = lo_url_get_port(oscUrl);
    char* const oscPath = lo_url_get_path(oscUrl);
    size_t  oscPathSize = strlen(oscPath);
    lo_address  oscAddr = lo_address_new(oscHost, oscPort);
    lo_server oscServer = lo_server_new(nullptr, osc_error_handler);

    OscData oscData = { oscAddr, oscPath, oscServer };

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

    char* const serverPath = lo_server_get_url(oscServer);
    char* const pluginPath = strdup(QString("%1%2").arg(serverPath).arg(oscPathSize > 1 ? oscPath + 1 : oscPath).toUtf8().constData());
    free(serverPath);

    // send update msg and wait for sample-rate
    osc_send_update(&oscData, pluginPath);

    // wait for sample-rate
    for (int i=0; i < 1000; i++)
    {
        if (d_lastUiSampleRate != 0.0)
            break;

        lo_server_recv(oscServer);
        d_msleep(50);
    }

    int ret;

    if (d_lastUiSampleRate != 0.0)
    {
        globalUI = new UIDssi(&oscData, uiTitle);

        ret = app.exec();

        delete globalUI;
        globalUI = nullptr;
    }
    else
    {
        ret = 1;
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

    free(oscHost);
    free(oscPort);
    free(oscPath);
    free(pluginPath);

    lo_address_free(oscAddr);
    lo_server_free(oscServer);

    return ret;
}

// -------------------------------------------------

#endif // DISTRHO_PLUGIN_TARGET_DSSI && DISTRHO_PLUGIN_HAS_UI
