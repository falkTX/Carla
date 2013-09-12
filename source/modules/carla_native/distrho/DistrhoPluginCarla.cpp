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

#ifdef DISTRHO_UI_QT
# error We do not want Qt in the engine code!
#endif

#include "CarlaNative.hpp"
#include "CarlaString.hpp"

#include "DistrhoPluginMain.cpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIMain.cpp"
# ifdef DISTRHO_UI_OPENGL
#  include "dgl/App.hpp"
#  include "dgl/Window.hpp"
# else
#  include "CarlaPipeUtils.hpp"
# endif
#endif

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------
// Carla UI

#ifdef DISTRHO_UI_EXTERNAL
class UICarla : public CarlaPipeServer
#else
class UICarla
#endif
{
public:
    UICarla(const HostDescriptor* const host, PluginInternal* const plugin)
        : fHost(host),
          fPlugin(plugin),
          fUi(this, 0, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback)
#ifdef DISTRHO_UI_OPENGL
        , glWindow(fUi.getWindow())
#endif
    {
#ifdef DISTRHO_UI_OPENGL
        //glWindow.setSize(fUi.getWidth(), fUi.getHeight());
        glWindow.setTitle(host->uiName);
#else
        CarlaString filename;
        filename += fHost->resourceDir;
#ifdef CARLA_OS_WIN
        filename += "\\";
#else
        filename += "/";
#endif
        filename += fUi.getExternalFilename();

        char sampleRateStr[12+1];
        sampleRateStr[12] = '\0';
        std::snprintf(sampleRateStr, 12, "%g", host->get_sample_rate(host->handle));

        CarlaPipeServer::start(filename, sampleRateStr, host->uiName);
#endif
    }

#ifdef DISTRHO_UI_EXTERNAL
    ~UICarla() override
    {
        CarlaPipeServer::stop();
    }

    void fail(const char* const error) override
    {
        carla_stderr2(error);
        fHost->dispatcher(fHost->handle, HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
    }

    void msgReceived(const char* const msg) override
    {
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
    }
#endif

    // ---------------------------------------------

    void carla_show(const bool yesNo)
    {
#ifdef DISTRHO_UI_OPENGL
        glWindow.setVisible(yesNo);
#else
        if (yesNo)
            writeMsg("show\n", 5);
        else
            writeMsg("hide\n", 5);
#endif
    }

    void carla_idle()
    {
        fUi.idle();

#ifdef DISTRHO_UI_EXTERNAL
        CarlaPipeServer::idle();
#endif
    }

    void carla_setParameterValue(const uint32_t index, const float value)
    {
#ifdef DISTRHO_UI_OPENGL
        fUi.parameterChanged(index, value);
#else
        char msgParamIndex[0xff+1];
        char msgParamValue[0xff+1];

        std::snprintf(msgParamIndex, 0xff, "%d\n", index);
        std::snprintf(msgParamValue, 0xff, "%f\n", value);

        msgParamIndex[0xff] = '\0';
        msgParamValue[0xff] = '\0';

        writeMsg("control\n", 8);
        writeMsg(msgParamIndex);
        writeMsg(msgParamValue);
#endif
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void carla_setMidiProgram(const uint32_t realProgram)
    {
 #ifdef DISTRHO_UI_OPENGL
        fUi.programChanged(realProgram);
 #else
        char msgProgram[0xff+1];

        std::snprintf(msgProgram, 0xff, "%d\n", realProgram);

        msgProgram[0xff] = '\0';

        writeMsg("program\n", 8);
        writeMsg(msgProgram);
 #endif
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void carla_setCustomData(const char* const key, const char* const value)
    {
 #ifdef DISTRHO_UI_OPENGL
        fUi.stateChanged(key, value);
 #else
        writeMsg("configure\n", 10);
        writeAndFixMsg(key);
        writeAndFixMsg(value);
 #endif
    }
#endif

    void carla_setUiTitle(const char* const uiTitle)
    {
#ifdef DISTRHO_UI_OPENGL
        glWindow.setTitle(uiTitle);
#else
        writeMsg("uiTitle\n", 8);
        writeAndFixMsg(uiTitle);
#endif
    }

    // ---------------------------------------------

protected:
    void handleEditParameter(uint32_t, bool)
    {
        // TODO
    }

    void handleSetParameterValue(uint32_t rindex, float value)
    {
        fHost->ui_parameter_changed(fHost->handle, rindex, value);
    }

    void handleSetState(const char* key, const char* value)
    {
        fHost->ui_custom_data_changed(fHost->handle, key, value);
    }

    void handleSendNote(bool, uint8_t, uint8_t, uint8_t)
    {
        // TODO
    }

    void handleUiResize(unsigned int /*width*/, unsigned int /*height*/)
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

#ifdef DISTRHO_UI_OPENGL
    // OpenGL stuff
    Window& glWindow;
#endif

    // ---------------------------------------------
    // Callbacks

#ifdef DISTRHO_UI_OPENGL
    #define handlePtr ((UICarla*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->handleEditParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->handleSetParameterValue(rindex, value);
    }

 #if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->handleSetState(key, value);
    }
 #else
    static constexpr setStateFunc setStateCallback = nullptr;
 #endif

 #if DISTRHO_PLUGIN_IS_SYNTH
    static void sendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->handleSendNote(onOff, channel, note, velocity);
    }
 #else
    static constexpr sendNoteFunc sendNoteCallback = nullptr;
 #endif

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        handlePtr->handleUiResize(width, height);
    }

    #undef handlePtr
#else
    static constexpr editParamFunc editParameterCallback = nullptr;
    static constexpr setParamFunc  setParameterCallback  = nullptr;
    static constexpr setStateFunc  setStateCallback      = nullptr;
    static constexpr sendNoteFunc  sendNoteCallback      = nullptr;
    static constexpr uiResizeFunc  uiResizeCallback      = nullptr;
#endif

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
        if (fUiPtr != nullptr)
        {
            delete fUiPtr;
            fUiPtr = nullptr;
        }
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

            for (uint8_t j=0; j < midiEvent->size; ++j)
                realMidiEvent->buf[j] = midiEvent->data[j];
        }

        fPlugin.run(inBuffer, outBuffer, frames, fRealMidiEvents, i);
    }
#else
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const ::MidiEvent* const, const uint32_t) override
    {
        fPlugin.run(inBuffer, outBuffer, frames, nullptr, 0);
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
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        fPlugin.setBufferSize(bufferSize, true);
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fPlugin.setSampleRate(sampleRate, true);
    }

#if DISTRHO_PLUGIN_HAS_UI
    void uiNameChanged(const char* const uiName) override
    {
        if (fUiPtr != nullptr)
            fUiPtr->carla_setUiTitle(uiName);
    }
#endif

    // -------------------------------------------------------------------

private:
    PluginInternal fPlugin;

#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent fRealMidiEvents[MAX_MIDI_EVENTS];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    UICarla* fUiPtr;

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
