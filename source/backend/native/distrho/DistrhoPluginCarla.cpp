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

#include "carla_native.hpp"
#include "carla_utils.hpp"

#include <QtGui/QMainWindow>

#include "DistrhoPluginMain.cpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIMain.cpp"
#endif

// -------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------
// Carla UI

class UICarla : public QMainWindow
{
public:
    UICarla(const HostDescriptor* const host_, PluginInternal* const plugin_)
        : QMainWindow(nullptr),
          host(host_),
          plugin(plugin_),
          widget(this),
          ui(this, (intptr_t)widget.winId(), setParameterCallback, setStateCallback, uiEditParameterCallback, uiSendNoteCallback, uiResizeCallback)
    {
        setCentralWidget(&widget);
        setFixedSize(ui.getWidth(), ui.getHeight());
        setWindowTitle(DISTRHO_PLUGIN_NAME);
    }

    ~UICarla()
    {
    }

    // ---------------------------------------------

    void carla_show(const bool yesNo)
    {
        setVisible(yesNo);
    }

    void carla_idle()
    {
        ui.idle();
    }

    void carla_setParameterValue(const uint32_t index, const float value)
    {
        ui.parameterChanged(index, value);
    }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    void carla_setMidiProgram(const uint32_t realProgram)
    {
        ui.programChanged(realProgram);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void carla_setCustomData(const char* const key, const char* const value)
    {
        ui.stateChanged(key, value);
    }
# endif

    // ---------------------------------------------

protected:
    void setParameterValue(uint32_t rindex, float value)
    {
        host->ui_parameter_changed(host->handle, rindex, value);
    }

# if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* key, const char* value)
    {
        host->ui_custom_data_changed(host->handle, key, value);
    }
# endif

    void uiEditParameter(uint32_t, bool)
    {
        // TODO
    }

# if DISTRHO_PLUGIN_IS_SYNTH
    void uiSendNote(bool, uint8_t, uint8_t, uint8_t)
    {
        // TODO
    }
# endif

    void uiResize(int width, int height)
    {
        setFixedSize(width, height);
    }

    void closeEvent(QCloseEvent* event)
    {
        host->ui_closed(host->handle);

        // FIXME - ignore event?
        QMainWindow::closeEvent(event);
    }

private:
    // Plugin stuff
    const HostDescriptor* const host;
    PluginInternal* const plugin;

    // Qt4 stuff
    QWidget widget;

    // UI
    UIInternal ui;

    // ---------------------------------------------
    // Callbacks

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        if (UICarla* _this_ = (UICarla*)ptr)
            _this_->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
# if DISTRHO_PLUGIN_WANT_STATE
        if (UICarla* _this_ = (UICarla*)ptr)
            _this_->setState(key, value);
# else
        return;

        // unused
        Q_UNUSED(ptr);
        Q_UNUSED(key);
        Q_UNUSED(value);
# endif
    }

    static void uiEditParameterCallback(void* ptr, uint32_t index, bool started)
    {
        if (UICarla* _this_ = (UICarla*)ptr)
            _this_->uiEditParameter(index, started);
    }

    static void uiSendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
# if DISTRHO_PLUGIN_IS_SYNTH
        if (UICarla* _this_ = (UICarla*)ptr)
            _this_->uiSendNote(onOff, channel, note, velocity);
# else
        return;

        // unused
        Q_UNUSED(ptr);
        Q_UNUSED(onOff);
        Q_UNUSED(channel);
        Q_UNUSED(note);
        Q_UNUSED(velocity);
# endif
    }

    static void uiResizeCallback(void* ptr, int width, int height)
    {
        if (UICarla* _this_ = (UICarla*)ptr)
            _this_->uiResize(width, height);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICarla)
};
#endif

// -----------------------------------------------------------------------
// Carla Plugin

class PluginCarla : public PluginDescriptorClass
{
public:
    PluginCarla(const HostDescriptor* const host)
        : PluginDescriptorClass(host)
    {
#if DISTRHO_PLUGIN_HAS_UI
        uiPtr = nullptr;
#endif
    }

    ~PluginCarla()
    {
#if DISTRHO_PLUGIN_HAS_UI
        uiPtr = nullptr;
#endif
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return plugin.parameterCount();
    }

    const ::Parameter* getParameterInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        static ::Parameter param;

        // reset
        param.hints = ::PARAMETER_IS_ENABLED;
        param.scalePointCount = 0;
        param.scalePoints = nullptr;

        {
            int nativeparamHints = 0;
            const uint32_t paramHints = plugin.parameterHints(index);

            if (paramHints & PARAMETER_IS_AUTOMABLE)
                nativeparamHints |= ::PARAMETER_IS_AUTOMABLE;
            if (paramHints & PARAMETER_IS_BOOLEAN)
                nativeparamHints |= ::PARAMETER_IS_BOOLEAN;
            if (paramHints & PARAMETER_IS_INTEGER)
                nativeparamHints |= ::PARAMETER_IS_INTEGER;
            if (paramHints & PARAMETER_IS_LOGARITHMIC)
                nativeparamHints |= ::PARAMETER_IS_LOGARITHMIC;
            if (paramHints & PARAMETER_IS_OUTPUT)
                nativeparamHints |= ::PARAMETER_IS_OUTPUT;

            param.hints = static_cast<ParameterHints>(nativeparamHints);
        }

        param.name = plugin.parameterName(index);
        param.unit = plugin.parameterUnit(index);

        {
            const ParameterRanges& ranges(plugin.parameterRanges(index));

            param.ranges.def = ranges.def;
            param.ranges.min = ranges.min;
            param.ranges.max = ranges.max;
            param.ranges.step = ranges.step;
            param.ranges.stepSmall = ranges.stepSmall;
            param.ranges.stepLarge = ranges.stepLarge;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return plugin.parameterValue(index);
    }

    // getParameterText unused

    // -------------------------------------------------------------------
    // Plugin midi-program calls

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual uint32_t getMidiProgramCount()
    {
        return plugin.programCount();
    }

    virtual const ::MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= plugin.programCount())
            return nullptr;

        static ::MidiProgram midiProgram;

        midiProgram.bank    = index / 128;
        midiProgram.program = index % 128;
        midiProgram.name    = plugin.programName(index);

        return &midiProgram;
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());

        plugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram = bank * 128 + program;

        if (realProgram >= plugin.programCount())
            return;

        plugin.setProgram(realProgram);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void setCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        plugin.setState(key, value);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        plugin.activate();
    }

    void deactivate()
    {
        plugin.deactivate();
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const ::MidiEvent* const midiEvents)
    {
        uint32_t i;

        for (i=0; i < midiEventCount && i < MAX_MIDI_EVENTS; i++)
        {
            const ::MidiEvent* const midiEvent = &midiEvents[i];
            MidiEvent* const realMidiEvent = &realMidiEvents[i];

            realMidiEvent->buffer[0] = midiEvent->data[0];
            realMidiEvent->buffer[1] = midiEvent->data[1];
            realMidiEvent->buffer[2] = midiEvent->data[2];
            realMidiEvent->frame     = midiEvent->time;
        }

        plugin.run(inBuffer, outBuffer, frames, i, realMidiEvents);
    }
#else
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t, const ::MidiEvent* const)
    {
        plugin.run(inBuffer, outBuffer, frames, 0, nullptr);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin UI calls

#if DISTRHO_PLUGIN_HAS_UI
    void uiShow(const bool show)
    {
        if (show)
            createUiIfNeeded();

        if (uiPtr != nullptr)
            uiPtr->carla_show(show);
    }

    void uiIdle()
    {
        CARLA_ASSERT(uiPtr);

        if (uiPtr != nullptr)
            uiPtr->carla_idle();
    }

    void uiSetParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(uiPtr);
        CARLA_ASSERT(index < getParameterCount());

        if (uiPtr != nullptr)
            uiPtr->carla_setParameterValue(index, value);
    }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    void uiSetMidiProgram(const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(uiPtr);

        uint32_t realProgram = bank * 128 + program;

        if (realProgram >= plugin.programCount())
            return;

        if (uiPtr != nullptr)
            uiPtr->carla_setMidiProgram(realProgram);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void uiSetCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(uiPtr);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (uiPtr != nullptr)
            uiPtr->carla_setCustomData(key, value);
    }
# endif
#endif

    // -------------------------------------------------------------------

private:
    PluginInternal plugin;

#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent realMidiEvents[MAX_MIDI_EVENTS];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    ScopedPointer<UICarla> uiPtr;

    void createUiIfNeeded()
    {
        if (uiPtr == nullptr)
        {
            d_lastUiSampleRate = getSampleRate();
            uiPtr = new UICarla(getHostHandle(), &plugin);
        }
    }
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCarla)

    // -------------------------------------------------------------------

public:
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        d_lastBufferSize = host->get_buffer_size(host->handle);
        d_lastSampleRate = host->get_sample_rate(host->handle);
        return new PluginCarla(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (PluginCarla*)handle;
    }
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
