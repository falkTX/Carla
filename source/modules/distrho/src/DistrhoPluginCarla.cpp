/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
#endif

#include "CarlaNative.hpp"

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static const writeMidiFunc writeMidiCallback = nullptr;
#endif

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------
// Carla UI

#if ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_IS_SYNTH
static const sendNoteFunc sendNoteCallback = nullptr;
#endif

class UICarla
{
public:
    UICarla(const NativeHostDescriptor* const host, PluginExporter* const plugin)
        : fHost(host),
          fUI(this, 0, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, setSizeCallback, plugin->getInstancePointer())
    {
        fUI.setWindowTitle(host->uiName);

        if (host->uiParentId != 0)
            fUI.setWindowTransientWinId(host->uiParentId);
    }

    ~UICarla()
    {
        fUI.quit();
    }

    // ---------------------------------------------

    void carla_show(const bool yesNo)
    {
        fUI.setWindowVisible(yesNo);
    }

    bool carla_idle()
    {
        return fUI.idle();
    }

    void carla_setParameterValue(const uint32_t index, const float value)
    {
        fUI.parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void carla_setMidiProgram(const uint32_t realProgram)
    {
        fUI.programLoaded(realProgram);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void carla_setCustomData(const char* const key, const char* const value)
    {
        fUI.stateChanged(key, value);
    }
#endif

    void carla_setUiTitle(const char* const uiTitle)
    {
        fUI.setWindowTitle(uiTitle);
    }

    // ---------------------------------------------

protected:
    void handleEditParameter(const uint32_t, const bool)
    {
        // TODO
    }

    void handleSetParameterValue(const uint32_t rindex, const float value)
    {
        fHost->ui_parameter_changed(fHost->handle, rindex, value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void handleSetState(const char* const key, const char* const value)
    {
        fHost->ui_custom_data_changed(fHost->handle, key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void handleSendNote(const uint8_t, const uint8_t, const uint8_t)
    {
        // TODO
    }
#endif

    void handleSetSize(const uint width, const uint height)
    {
        fUI.setWindowSize(width, height);
    }

    // ---------------------------------------------

private:
    // Plugin stuff
    const NativeHostDescriptor* const fHost;

    // UI
    UIExporter fUI;

    // ---------------------------------------------
    // Callbacks

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
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->handleSendNote(channel, note, velocity);
    }
#endif

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        handlePtr->handleSetSize(width, height);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICarla)
};
#endif // DISTRHO_PLUGIN_HAS_UI

// -----------------------------------------------------------------------
// Carla Plugin

class PluginCarla : public NativePluginClass
{
public:
    PluginCarla(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fPlugin(this, writeMidiCallback),
          fScalePointsCache(nullptr)
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

        if (fScalePointsCache != nullptr)
        {
            delete[] fScalePointsCache;
            fScalePointsCache = nullptr;
        }
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return fPlugin.getParameterCount();
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(), nullptr);

        static NativeParameter param;

        param.scalePointCount = 0;
        param.scalePoints = nullptr;

        {
            int      nativeParamHints = ::NATIVE_PARAMETER_IS_ENABLED;
            const uint32_t paramHints = fPlugin.getParameterHints(index);

            if (paramHints & kParameterIsAutomable)
                nativeParamHints |= ::NATIVE_PARAMETER_IS_AUTOMABLE;
            if (paramHints & kParameterIsBoolean)
                nativeParamHints |= ::NATIVE_PARAMETER_IS_BOOLEAN;
            if (paramHints & kParameterIsInteger)
                nativeParamHints |= ::NATIVE_PARAMETER_IS_INTEGER;
            if (paramHints & kParameterIsLogarithmic)
                nativeParamHints |= ::NATIVE_PARAMETER_IS_LOGARITHMIC;
            if (paramHints & kParameterIsOutput)
                nativeParamHints |= ::NATIVE_PARAMETER_IS_OUTPUT;

            param.hints = static_cast<NativeParameterHints>(nativeParamHints);
        }

        param.name = fPlugin.getParameterName(index);
        param.unit = fPlugin.getParameterUnit(index);

        {
            const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

            param.ranges.def = ranges.def;
            param.ranges.min = ranges.min;
            param.ranges.max = ranges.max;
        }

        {
            const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(index));

            if (const uint32_t scalePointCount = enumValues.count)
            {
                NativeParameterScalePoint* const scalePoints = new NativeParameterScalePoint[scalePointCount];

                for (uint32_t i=0; i<scalePointCount; ++i)
                {
                    scalePoints[i].label = enumValues.values[i].label.buffer();
                    scalePoints[i].value = enumValues.values[i].value;
                }

                param.scalePoints     = scalePoints;
                param.scalePointCount = scalePointCount;

                if (enumValues.restrictedMode)
                    param.hints = static_cast<NativeParameterHints>(param.hints|::NATIVE_PARAMETER_USES_SCALEPOINTS);
            }
            else if (fScalePointsCache != nullptr)
            {
                delete[] fScalePointsCache;
                fScalePointsCache = nullptr;
            }
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(), 0.0f);

        return fPlugin.getParameterValue(index);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getMidiProgramCount() const override
    {
        return fPlugin.getProgramCount();
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getMidiProgramCount(), nullptr);

        static NativeMidiProgram midiProgram;

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
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);

        fPlugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        const uint32_t realProgram(bank * 128 + program);

        CARLA_SAFE_ASSERT_RETURN(realProgram < getMidiProgramCount(),);

        fPlugin.loadProgram(realProgram);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

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

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        MidiEvent realMidiEvents[midiEventCount];

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const NativeMidiEvent& midiEvent(midiEvents[i]);
            MidiEvent& realMidiEvent(realMidiEvents[i]);

            realMidiEvent.frame = midiEvent.time;
            realMidiEvent.size  = midiEvent.size;

            uint8_t j=0;
            for (; j<midiEvent.size; ++j)
                realMidiEvent.data[j] = midiEvent.data[j];
            for (; j<midiEvent.size; ++j)
                realMidiEvent.data[j] = midiEvent.data[j];

            realMidiEvent.dataExt = nullptr;
        }

        fPlugin.run(const_cast<const float**>(inBuffer), outBuffer, frames, realMidiEvents, midiEventCount);
    }
#else
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        fPlugin.run(const_cast<const float**>(inBuffer), outBuffer, frames);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin UI calls

#if DISTRHO_PLUGIN_HAS_UI
    void uiShow(const bool show) override
    {
        if (show)
        {
            createUiIfNeeded();
            CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);

            fUiPtr->carla_show(show);
        }
        else if (fUiPtr != nullptr)
        {
            delete fUiPtr;
            fUiPtr = nullptr;
        }
    }

    void uiIdle() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);

        if (! fUiPtr->carla_idle())
        {
            uiClosed();

            delete fUiPtr;
            fUiPtr = nullptr;
        }
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);

        fUiPtr->carla_setParameterValue(index, value);
    }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    void uiSetMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);

        const uint32_t realProgram(bank * 128 + program);

        CARLA_SAFE_ASSERT_RETURN(realProgram < getMidiProgramCount(),);

        fUiPtr->carla_setMidiProgram(realProgram);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

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
        CARLA_SAFE_ASSERT_RETURN(fUiPtr != nullptr,);

        fUiPtr->carla_setUiTitle(uiName);
    }
#endif

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;
    mutable NativeParameterScalePoint* fScalePointsCache;

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

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        if (midiEvent.size > 4)
            return;

        const NativeMidiEvent event = {
            midiEvent.frame, 0, midiEvent.size, midiEvent.data
        };

        return ((PluginCarla*)ptr)->fPlugin.writeMidiEvent(midiEvent);
    }
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCarla)

    // -------------------------------------------------------------------

public:
    static NativePluginHandle _instantiate(const NativeHostDescriptor* host)
    {
        d_lastBufferSize = host->get_buffer_size(host->handle);
        d_lastSampleRate = host->get_sample_rate(host->handle);
        return new PluginCarla(host);
    }

    static void _cleanup(NativePluginHandle handle)
    {
        delete (PluginCarla*)handle;
    }
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
