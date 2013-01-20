/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#ifdef DISTRHO_PLUGIN_TARGET_JACK

#include "DistrhoDefines.h"

#if ! DISTRHO_PLUGIN_HAS_UI
# error Standalone JACK mode requires UI
#endif

#include "DistrhoPluginInternal.h"
#include "DistrhoUIInternal.h"

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>

#include <QtCore/QSettings>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

// -------------------------------------------------

START_NAMESPACE_DISTRHO

class PluginJack : public QMainWindow
{
public:
    PluginJack(jack_client_t* client_)
        : QMainWindow(nullptr),
          widget(this),
          settings("DISTRHO", DISTRHO_PLUGIN_NAME),
          ui(this, widget.winId(), setParameterCallback, setStateCallback, nullptr, uiNoteCallback, uiResizeCallback),
          client(client_)
    {
        setCentralWidget(&widget);
        setFixedSize(ui.getWidth(), ui.getHeight());
        setWindowTitle(DISTRHO_PLUGIN_NAME);

        if (DISTRHO_PLUGIN_NUM_INPUTS > 0)
        {
            portsIn = new jack_port_t* [DISTRHO_PLUGIN_NUM_INPUTS];

            for (int i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++)
                portsIn[i] = jack_port_register(client, d_string("Audio Input ") + d_string(i+1), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }
        else
            portsIn = nullptr;

        if (DISTRHO_PLUGIN_NUM_OUTPUTS > 0)
        {
            portsOut = new jack_port_t* [DISTRHO_PLUGIN_NUM_OUTPUTS];

            for (int i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++)
                portsOut[i] = jack_port_register(client, d_string("Audio Output ") + d_string(i+1), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }
        else
            portsOut = nullptr;

#if DISTRHO_PLUGIN_IS_SYNTH
        portMidi = jack_port_register(client, "Midi Input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
#endif

        jack_set_process_callback(client, jackProcessCallback, this);

        uiTimer = startTimer(30);

        // load settings
        restoreGeometry(settings.value("Global/Geometry", QByteArray()).toByteArray());

        for (uint32_t i=0; i < plugin.parameterCount(); i++)
        {
            bool ok;
            float value = settings.value(QString("Parameters/%1").arg(i)).toFloat(&ok);

            if (ok)
            {
                plugin.setParameterValue(i, value);
                ui.parameterChanged(i, value);
            }
        }

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0; i < plugin.stateCount(); i++)
        {
            const char* key = plugin.stateKey(i);
            QString stringValue(settings.value(key).toString());

            if (! stringValue.isEmpty())
                ui.stateChanged(key, stringValue.toUtf8().constData());
        }
#endif
    }

    ~PluginJack()
    {
        // save settings
        settings.setValue("Global/Geometry", saveGeometry());

        if (uiTimer)
            killTimer(uiTimer);

        if (portsIn)
            delete[] portsIn;

        if (portsOut)
            delete[] portsOut;
    }

    // ---------------------------------------------

protected:
    void setParameterValue(uint32_t index, float value)
    {
        plugin.setParameterValue(index, value);
        settings.setValue(QString("Parameters/%1").arg(index), value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* key, const char* value)
    {
        plugin.setState(key, value);
        settings.setValue(key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void uiNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        // TODO
    }
#endif

    void uiResize(unsigned int width, unsigned int height)
    {
        widget.setFixedSize(width, height);
        setFixedSize(width, height);
    }

    int jackProcess(jack_nframes_t nframes)
    {
        if (nframes <= 1)
            return 1;

        // Check for updated bufferSize
        if (nframes != d_lastBufferSize)
        {
            d_lastBufferSize = nframes;
            plugin.setBufferSize(nframes, true);
        }

        const float* ins[DISTRHO_PLUGIN_NUM_INPUTS];
        float*      outs[DISTRHO_PLUGIN_NUM_OUTPUTS];

        for (int i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++)
            ins[i] = (float*)jack_port_get_buffer(portsIn[i], nframes);

        for (int i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++)
            outs[i] = (float*)jack_port_get_buffer(portsOut[i], nframes);

#if DISTRHO_PLUGIN_IS_SYNTH
        uint32_t midiEventCount = 0;

        void* mIn = jack_port_get_buffer(portMidi, nframes);
        // TODO

        plugin.run(ins, outs, nframes, midiEventCount, midiEvents);
#else
        plugin.run(ins, outs, nframes, 0, nullptr);
#endif

        return 0;
    }

    // ---------------------------------------------

    void closeEvent(QCloseEvent* event)
    {
        QMainWindow::closeEvent(event);

        qApp->quit();
    }

    void timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == uiTimer)
            ui.idle();

        QMainWindow::timerEvent(event);
    }

    // ---------------------------------------------

private:
    // Qt4 stuff
    int uiTimer;
    QWidget widget;
    QSettings settings;

    PluginInternal plugin;
    UIInternal     ui;

    jack_client_t* const client;
    jack_port_t** portsIn;
    jack_port_t** portsOut;

#if DISTRHO_PLUGIN_IS_SYNTH
    jack_port_t* portMidi;
    MidiEvent midiEvents[MAX_MIDI_EVENTS];
#endif

    // ---------------------------------------------
    // Callbacks

    static void setParameterCallback(void* ptr, uint32_t index, float value)
    {
        PluginJack* _this_ = (PluginJack*)ptr;
        assert(_this_);

        _this_->setParameterValue(index, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        PluginJack* _this_ = (PluginJack*)ptr;
        assert(_this_);

        _this_->setState(key, value);
#else
        Q_UNUSED(ptr);
        Q_UNUSED(key);
        Q_UNUSED(value);
#endif
    }

    static void uiNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        PluginJack* _this_ = (PluginJack*)ptr;
        assert(_this_);

        _this_->uiNote(onOff, channel, note, velocity);
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
        PluginJack* _this_ = (PluginJack*)ptr;
        assert(_this_);

        _this_->uiResize(width, height);
    }

    static int jackProcessCallback(jack_nframes_t nframes, void* arg)
    {
        PluginJack* _this_ = (PluginJack*)arg;
        assert(_this_);

        return _this_->jackProcess(nframes);
    }
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

// -------------------------------------------------

std::string jack_status_get_error_string(const jack_status_t& status)
{
    std::string errorString;

    if (status & JackFailure)
        errorString += "Overall operation failed;\n";
    if (status & JackInvalidOption)
        errorString += "The operation contained an invalid or unsupported option;\n";
    if (status & JackNameNotUnique)
        errorString += "The desired client name was not unique;\n";
    if (status & JackServerStarted)
        errorString += "The JACK server was started as a result of this operation;\n";
    if (status & JackServerFailed)
        errorString += "Unable to connect to the JACK server;\n";
    if (status & JackServerError)
        errorString += "Communication error with the JACK server;\n";
    if (status & JackNoSuchClient)
        errorString += "Requested client does not exist;\n";
    if (status & JackLoadFailure)
        errorString += "Unable to load internal client;\n";
    if (status & JackInitFailure)
        errorString += "Unable to initialize client;\n";
    if (status & JackShmFailure)
        errorString += "Unable to access shared memory;\n";
    if (status & JackVersionError)
        errorString += "Client's protocol version does not match;\n";
    if (status & JackBackendError)
        errorString += "Backend Error;\n";
    if (status & JackClientZombie)
        errorString += "Client is being shutdown against its will;\n";

    if (errorString.size() > 0)
        errorString.replace(errorString.size()-2, 2, ".");

    return errorString;
}

int main(int argc, char* argv[])
{
    USE_NAMESPACE_DISTRHO
    QApplication app(argc, argv, true);

    jack_status_t status;
    jack_client_t* client = jack_client_open(DISTRHO_PLUGIN_NAME, JackNullOption, &status);

    if (! client)
    {
        std::string errorString(jack_status_get_error_string(status));
        QMessageBox::critical(nullptr, app.translate(DISTRHO_PLUGIN_NAME, "Error"),
                                       app.translate(DISTRHO_PLUGIN_NAME,
                                                     "Could not connect to JACK, possible reasons:\n"
                                                     "%1").arg(QString::fromStdString(errorString)));
        return 1;
    }

    d_lastBufferSize = jack_get_buffer_size(client);
    d_lastSampleRate = jack_get_sample_rate(client);
    setLastUiSampleRate(d_lastSampleRate);

    PluginJack plugin(client);
    plugin.show();

    jack_activate(client);

    int ret = app.exec();

    jack_deactivate(client);
    jack_client_close(client);

    return ret;
}

// -------------------------------------------------

#endif // DISTRHO_PLUGIN_TARGET_JACK
