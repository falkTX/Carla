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

#include "../CarlaNative.hpp"

#include "DistrhoPluginMain.cpp"

#ifdef DISTRHO_UI_QT
# error We do not want Qt in the engine code!
#endif
#ifdef DISTRHO_UI_EXTERNAL
# error Not implemented yet
#endif

#define DISTRHO_PLUGIN_HAS_UI 1

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIMain.cpp"
# include "dgl/App.hpp"
# include "dgl/Window.hpp"
#endif

using namespace DGL;
using juce::ScopedPointer;

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------
// Carla UI

class UICarla
{
public:
    UICarla(const HostDescriptor* const host, PluginInternal* const plugin)
        : fHost(host),
          fPlugin(plugin),
          fUi(this, 0, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback),
          glApp(fUi.getApp()),
          glWindow(fUi.getWindow())
    {
        glWindow.setSize(fUi.getWidth(), fUi.getHeight());
        glWindow.setWindowTitle(host->uiName);
    }

    // ---------------------------------------------

    void carla_show(const bool yesNo)
    {
        glWindow.setVisible(yesNo);
    }

    void carla_idle()
    {
        fUi.idle();
    }

    void carla_setParameterValue(const uint32_t index, const float value)
    {
        fUi.parameterChanged(index, value);
    }

    void carla_setMidiProgram(const uint32_t realProgram)
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        fUi.programChanged(realProgram);
#else
        return;
        // unused
        (void)realProgram;
#endif
    }

    void carla_setCustomData(const char* const key, const char* const value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        fUi.stateChanged(key, value);
#else
        return;
        // unused
        (void)key;
        (void)value;
#endif
    }

    // ---------------------------------------------

protected:
    void editParameter(uint32_t, bool)
    {
        // TODO
    }

    void setParameterValue(uint32_t rindex, float value)
    {
        fHost->ui_parameter_changed(fHost->handle, rindex, value);
    }

    void setState(const char* key, const char* value)
    {
        fHost->ui_custom_data_changed(fHost->handle, key, value);
    }

    void sendNote(bool, uint8_t, uint8_t, uint8_t)
    {
        // TODO
    }

    void uiResize(unsigned int /*width*/, unsigned int /*height*/)
    {
        // TODO
    }

    // ---------------------------------------------

private:
    // Plugin stuff
    const HostDescriptor* const fHost;
    PluginInternal* const fPlugin;

    // UI
    UIInternal fUi;

    // OpenGL stuff
    App& glApp;
    Window& glWindow;

    // ---------------------------------------------
    // Callbacks

    #define handlePtr ((UICarla*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->editParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->setState(key, value);
    }

    static void sendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(onOff, channel, note, velocity);
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        handlePtr->uiResize(width, height);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICarla)
};
#endif // DISTRHO_PLUGIN_HAS_UI

// -----------------------------------------------------------------------
// Carla Plugin

class PluginCarla : public PluginClass
{
public:
    PluginCarla(const HostDescriptor* const host)
        : PluginClass(host)
    {
#if DISTRHO_PLUGIN_HAS_UI
        fUiPtr = nullptr;
#endif
    }

    ~PluginCarla() override
    {
#if DISTRHO_PLUGIN_HAS_UI
        fUiPtr = nullptr;
#endif
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return fPlugin.getParameterCount();
    }

    const ::Parameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getParameterCount());

        static ::Parameter param;

        // reset
        param.hints = ::PARAMETER_IS_ENABLED;
        param.scalePointCount = 0;
        param.scalePoints = nullptr;

        {
            int      nativeParamHints = ::PARAMETER_IS_ENABLED;
            const uint32_t paramHints = fPlugin.getParameterHints(index);

            if (paramHints & PARAMETER_IS_AUTOMABLE)
                nativeParamHints |= ::PARAMETER_IS_AUTOMABLE;
            if (paramHints & PARAMETER_IS_BOOLEAN)
                nativeParamHints |= ::PARAMETER_IS_BOOLEAN;
            if (paramHints & PARAMETER_IS_INTEGER)
                nativeParamHints |= ::PARAMETER_IS_INTEGER;
            if (paramHints & PARAMETER_IS_LOGARITHMIC)
                nativeParamHints |= ::PARAMETER_IS_LOGARITHMIC;
            if (paramHints & PARAMETER_IS_OUTPUT)
                nativeParamHints |= ::PARAMETER_IS_OUTPUT;

            param.hints = static_cast<ParameterHints>(nativeParamHints);
        }

        param.name = fPlugin.getParameterName(index);
        param.unit = fPlugin.getParameterUnit(index);

        {
            const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

            param.ranges.def = ranges.def;
            param.ranges.min = ranges.min;
            param.ranges.max = ranges.max;
            param.ranges.step = ranges.step;
            param.ranges.stepSmall = ranges.stepSmall;
            param.ranges.stepLarge = ranges.stepLarge;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getParameterCount());

        return fPlugin.getParameterValue(index);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getMidiProgramCount() const override
    {
        return fPlugin.getProgramCount();
    }

    const ::MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= fPlugin.getProgramCount())
            return nullptr;

        static ::MidiProgram midiProgram;

        midiProgram.bank    = index / 128;
        midiProgram.program = index % 128;
        midiProgram.name    = fPlugin.getProgramName(index);

        return &midiProgram;
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(index < getParameterCount());

        fPlugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.getProgramCount())
            return;

        fPlugin.setProgram(realProgram);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        fPlugin.setState(key, value);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        fPlugin.activate();
    }

    void deactivate() override
    {
        fPlugin.deactivate();
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const ::MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        uint32_t i;

        for (i=0; i < midiEventCount && i < MAX_MIDI_EVENTS; ++i)
        {
            const ::MidiEvent* const midiEvent = &midiEvents[i];
            MidiEvent* const realMidiEvent = &fRealMidiEvents[i];

            realMidiEvent->frame = midiEvent->time;
            realMidiEvent->size  = midiEvent->size;

            carla_copy<uint8_t>(realMidiEvent->buf, midiEvent->data, midiEvent->size);
        }

        fPlugin.run(inBuffer, outBuffer, frames, i, fRealMidiEvents);
    }
#else
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const ::MidiEvent* const, const uint32_t) override
    {
        fPlugin.run(inBuffer, outBuffer, frames, 0, nullptr);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin UI calls

#if DISTRHO_PLUGIN_HAS_UI
    void uiShow(const bool show) override
    {
        if (show)
            createUiIfNeeded();

        if (fUiPtr != nullptr)
            fUiPtr->carla_show(show);
    }

    void uiIdle() override
    {
        CARLA_ASSERT(fUiPtr != nullptr);

        if (fUiPtr != nullptr)
            fUiPtr->carla_idle();
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);
        CARLA_ASSERT(index < getParameterCount());

        if (fUiPtr != nullptr)
            fUiPtr->carla_setParameterValue(index, value);
    }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    void uiSetMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);

        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.getProgramCount())
            return;

        if (fUiPtr != nullptr)
            fUiPtr->carla_setMidiProgram(realProgram);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (fUiPtr != nullptr)
            fUiPtr->carla_setCustomData(key, value);
    }
# endif
#endif

    // -------------------------------------------------------------------

private:
    PluginInternal fPlugin;

#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent fRealMidiEvents[MAX_MIDI_EVENTS];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    ScopedPointer<UICarla> fUiPtr;

    void createUiIfNeeded()
    {
        if (fUiPtr == nullptr)
        {
            d_lastUiSampleRate = getSampleRate();
            fUiPtr = new UICarla(getHostHandle(), &fPlugin);
        }
    }
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCarla)

    // -------------------------------------------------------------------

public:
    static PluginHandle _instantiate(const HostDescriptor* host)
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
