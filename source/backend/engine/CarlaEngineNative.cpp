/*
 * Carla Plugin Engine (Native)
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifdef BUILD_BRIDGE
# error This file should not be compiled if building bridge
#endif

#include "CarlaEngineInternal.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaNative.hpp"

CARLA_BACKEND_START_NAMESPACE

#if 0
// -----------------------------------------------------------------------

class CarlaEngineNativeThread : public QThread
{
public:
    enum UiState {
        UiNone = 0,
        UiHide,
        UiShow,
        UiCrashed
    };

    CarlaEngineNativeThread(CarlaEngine* const engine)
        : kEngine(engine),
          fBinary("carla-control"),
          fProcess(nullptr),
          fUiState(UiNone)
    {
        carla_debug("CarlaEngineNativeThread::CarlaEngineNativeThread(engine:\"%s\")", engine->getName());
    }

    ~CarlaEngineNativeThread()
    {
        CARLA_ASSERT_INT(fUiState == UiNone, fUiState);
        carla_debug("CarlaEngineNativeThread::~CarlaEngineNativeThread()");

        if (fProcess != nullptr)
        {
            delete fProcess;
            fProcess = nullptr;
        }
    }

    void setOscData(const char* const binary)
    {
        fBinary = binary;
    }

    UiState getUiState()
    {
        const UiState state(fUiState);
        fUiState = UiNone;

        return state;
    }

    void stop()
    {
        if (fProcess == nullptr)
            return;

        fUiState = UiNone;
        fProcess->kill();
        //fProcess->close();
    }

protected:
    void run()
    {
        carla_debug("CarlaEngineNativeThread::run() - binary:\"%s\"", (const char*)fBinary);

        if (fProcess == nullptr)
        {
            fProcess = new QProcess(nullptr);
            fProcess->setProcessChannelMode(QProcess::ForwardedChannels);
        }
        else if (fProcess->state() == QProcess::Running)
        {
            carla_stderr("CarlaEngineNativeThread::run() - already running, giving up...");

            fUiState = UiCrashed;
            fProcess->terminate();
            //kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), -1, 0, 0.0f, nullptr);
            // TODO: tell master to hide UI
            return;
        }

        QStringList arguments;
        arguments << kEngine->getOscServerPathTCP();

        fProcess->start((const char*)fBinary, arguments);
        fProcess->waitForStarted();

        fUiState = UiShow;

        fProcess->waitForFinished(-1);

        if (fProcess->exitCode() == 0)
        {
            // Hide
            fUiState = UiHide;
            carla_stdout("CarlaEngineNativeThread::run() - GUI closed");
        }
        else
        {
            // Kill
            fUiState = UiCrashed;
            carla_stderr("CarlaEngineNativeThread::run() - GUI crashed while running");
        }
    }

private:
    CarlaEngine* const kEngine;

    CarlaString fBinary;
    QProcess*   fProcess;

    UiState fUiState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNativeThread)
};

// -----------------------------------------------------------------------
#endif

class CarlaEngineNative : public NativePluginClass,
                          public CarlaEngine
{
public:
    CarlaEngineNative(const NativeHostDescriptor* const host, const bool isPatchbay)
        : NativePluginClass(host),
          CarlaEngine(),
          fIsPatchbay(isPatchbay)
          /*fIsRunning(true),
          fThread(this)*/
    {
        carla_debug("CarlaEngineNative::CarlaEngineNative()");

        // set-up engine
        if (fIsPatchbay)
        {
            pData->options.processMode         = ENGINE_PROCESS_MODE_PATCHBAY;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = false;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Patchbay");
        }
        else
        {
            pData->options.processMode         = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = true;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Rack");
        }

#if 0
        // set control thread binary
        CarlaString threadBinary(getResourceDir());
        threadBinary += "/../";
        threadBinary += "carla_control.py";

        fThread.setOscData(threadBinary);

        // TESTING
//         if (! addPlugin(PLUGIN_INTERNAL, nullptr, "MIDI Transpose", "midiTranspose"))
//             carla_stdout("TESTING PLUG1 ERROR:\n%s", getLastError());
//         if (! addPlugin(PLUGIN_INTERNAL, nullptr, "ZynAddSubFX", "zynaddsubfx"))
//             carla_stdout("TESTING PLUG2 ERROR:\n%s", getLastError());
//         if (! addPlugin(PLUGIN_INTERNAL, nullptr, "Ping Pong Pan", "PingPongPan"))
//             carla_stdout("TESTING PLUG3 ERROR:\n%s", getLastError());
#endif
    }

    ~CarlaEngineNative() override
    {
        carla_debug("CarlaEngineNative::~CarlaEngineNative()");

        //fIsRunning = false;

        setAboutToClose();
        removeAllPlugins();
        close();
    }

protected:
    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineNative::init(\"%s\")", clientName);

        pData->bufferSize = NativePluginClass::getBufferSize();
        pData->sampleRate = NativePluginClass::getSampleRate();

        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineNative::close()");

        runPendingRtEvents();
        return CarlaEngine::close();
    }

    bool isRunning() const noexcept override
    {
        return false; //fIsRunning;
    }

    bool isOffline() const noexcept override
    {
        return false;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypePlugin;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "Plugin";
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            return plugin->getParameterCount();

        return 0;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (plugin->getParameterCount() < index)
            {
                static NativeParameter param;
                static char strBufName[STR_MAX+1];
                static char strBufUnit[STR_MAX+1];

                const ParameterData& paramData(plugin->getParameterData(index));
                const ParameterRanges& paramRanges(plugin->getParameterRanges(index));

                plugin->getParameterName(index, strBufName);
                plugin->getParameterUnit(index, strBufUnit);

                unsigned int hints = 0x0;

                if (paramData.hints & PARAMETER_IS_BOOLEAN)
                    hints |= ::PARAMETER_IS_BOOLEAN;
                if (paramData.hints & PARAMETER_IS_INTEGER)
                    hints |= ::PARAMETER_IS_INTEGER;
                if (paramData.hints & PARAMETER_IS_LOGARITHMIC)
                    hints |= ::PARAMETER_IS_LOGARITHMIC;
                if (paramData.hints & PARAMETER_IS_AUTOMABLE)
                    hints |= ::PARAMETER_IS_AUTOMABLE;
                if (paramData.hints & PARAMETER_USES_SAMPLERATE)
                    hints |= ::PARAMETER_USES_SAMPLE_RATE;
                if (paramData.hints & PARAMETER_USES_SCALEPOINTS)
                    hints |= ::PARAMETER_USES_SCALEPOINTS;
                if (paramData.hints & PARAMETER_USES_CUSTOM_TEXT)
                    hints |= ::PARAMETER_USES_CUSTOM_TEXT;

                if (paramData.type == PARAMETER_INPUT || paramData.type == PARAMETER_OUTPUT)
                {
                    if (paramData.hints & PARAMETER_IS_ENABLED)
                        hints |= ::PARAMETER_IS_ENABLED;
                    if (paramData.type == PARAMETER_OUTPUT)
                        hints |= ::PARAMETER_IS_OUTPUT;
                }

                param.hints = static_cast<NativeParameterHints>(hints);
                param.name  = strBufName;
                param.unit  = strBufUnit;
                param.ranges.def = paramRanges.def;
                param.ranges.min = paramRanges.min;
                param.ranges.max = paramRanges.max;
                param.ranges.step = paramRanges.step;
                param.ranges.stepSmall = paramRanges.stepSmall;
                param.ranges.stepLarge = paramRanges.stepLarge;
                param.scalePointCount = 0; // TODO
                param.scalePoints = nullptr;

                return &param;
            }
        }

        return nullptr;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (plugin->getParameterCount() < index)
                return plugin->getParameterValue(index);
        }

        return 0.0f;
    }

    const char* getParameterText(const uint32_t index, const float value) const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (plugin->getParameterCount() < index)
            {
                static char strBuf[STR_MAX+1];
                carla_zeroChar(strBuf, STR_MAX+1);

                plugin->getParameterText(index, value, strBuf);

                return strBuf;
            }
        }

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            return plugin->getMidiProgramCount();

        return 0;
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (plugin->getMidiProgramCount() < index)
            {
                static NativeMidiProgram midiProg;

                {
                    const MidiProgramData& midiProgData(plugin->getMidiProgramData(index));

                    midiProg.bank    = midiProgData.bank;
                    midiProg.program = midiProgData.program;
                    midiProg.name    = midiProgData.name;
                }

                return &midiProg;
            }
        }

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (plugin->getParameterCount() < index)
                plugin->setParameterValue(index, value, false, false, false);
        }
    }

    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            plugin->setMidiProgramById(bank, program, false, false, false);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        // TODO
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
#if 0
        for (uint32_t i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            plugin->setActive(true, true, false);
        }
#endif
    }

    void deactivate() override
    {
#if 0
        for (uint32_t i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            plugin->setActive(false, true, false);
        }
#endif

        // just in case
        runPendingRtEvents();
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (pData->curPluginCount == 0 && ! fIsPatchbay)
        {
            FLOAT_COPY(outBuffer[0], inBuffer[0], frames);
            FLOAT_COPY(outBuffer[1], inBuffer[1], frames);

            return runPendingRtEvents();;
        }

        // ---------------------------------------------------------------
        // Time Info

        const NativeTimeInfo* const timeInfo(NativePluginClass::getTimeInfo());

        pData->timeInfo.playing = timeInfo->playing;
        pData->timeInfo.frame   = timeInfo->frame;
        pData->timeInfo.usecs   = timeInfo->usecs;
        pData->timeInfo.valid   = 0x0;

        if (timeInfo->bbt.valid)
        {
            pData->timeInfo.valid |= EngineTimeInfo::kValidBBT;

            pData->timeInfo.bbt.bar = timeInfo->bbt.bar;
            pData->timeInfo.bbt.beat = timeInfo->bbt.beat;
            pData->timeInfo.bbt.tick = timeInfo->bbt.tick;
            pData->timeInfo.bbt.barStartTick = timeInfo->bbt.barStartTick;

            pData->timeInfo.bbt.beatsPerBar = timeInfo->bbt.beatsPerBar;
            pData->timeInfo.bbt.beatType = timeInfo->bbt.beatType;

            pData->timeInfo.bbt.ticksPerBeat = timeInfo->bbt.ticksPerBeat;
            pData->timeInfo.bbt.beatsPerMinute = timeInfo->bbt.beatsPerMinute;
        }

#if 0
        // ---------------------------------------------------------------
        // initialize input events

        carla_zeroStruct<EngineEvent>(pData->bufEvents.in, INTERNAL_EVENT_COUNT);
        {
            uint32_t engineEventIndex = 0;

            for (uint32_t i=0; i < midiEventCount && engineEventIndex < INTERNAL_EVENT_COUNT; ++i)
            {
                const ::MidiEvent& midiEvent(midiEvents[i]);

                if (midiEvent.size > 4)
                    continue;

                const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent.data);

                // we don't want some events
                if (status == MIDI_STATUS_PROGRAM_CHANGE)
                    continue;

                // handle note/sound off properly
                if (status == MIDI_STATUS_CONTROL_CHANGE)
                {
                    const uint8_t control = midiEvent.data[1];

                    if (MIDI_IS_CONTROL_BANK_SELECT(control))
                        continue;

                    if (control == MIDI_CONTROL_ALL_SOUND_OFF || control == MIDI_CONTROL_ALL_NOTES_OFF)
                    {
                        EngineEvent& engineEvent(pData->bufEvents.in[engineEventIndex++]);
                        engineEvent.clear();

                        engineEvent.type    = kEngineEventTypeControl;
                        engineEvent.time    = midiEvent.time;
                        engineEvent.channel = channel;

                        engineEvent.ctrl.type  = (control == MIDI_CONTROL_ALL_SOUND_OFF) ? kEngineControlEventTypeAllSoundOff : kEngineControlEventTypeAllNotesOff;
                        engineEvent.ctrl.param = 0;
                        engineEvent.ctrl.value = 0.0f;

                        continue;
                    }
                }

                EngineEvent& engineEvent(pData->bufEvents.in[engineEventIndex++]);
                engineEvent.clear();

                engineEvent.type    = kEngineEventTypeMidi;
                engineEvent.time    = midiEvent.time;
                engineEvent.channel = channel;

                engineEvent.midi.data[0] = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                engineEvent.midi.data[1] = midiEvent.data[1];
                engineEvent.midi.data[2] = midiEvent.data[2];
                engineEvent.midi.data[3] = midiEvent.data[3];
                engineEvent.midi.size    = midiEvent.size;
            }
        }

        // ---------------------------------------------------------------
        // create audio buffers

        float* inBuf[2]  = { inBuffer[0], inBuffer[1] };
        float* outBuf[2] = { outBuffer[0], outBuffer[1] };

        // ---------------------------------------------------------------
        // process

        processRack(inBuf, outBuf, frames);
#endif
        runPendingRtEvents();
        return;

        // TODO
        (void)midiEvents;
        (void)midiEventCount;
    }

#if 0
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            fThread.start();
        }
        else
        {
#if 0
            for (uint32_t i=0; i < pData->curPluginCount; ++i)
            {
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin == nullptr || ! plugin->enabled())
                    continue;

                plugin->showGui(false);
            }
#endif

            fThread.stop();
        }
    }

    void uiIdle() override
    {
        CarlaEngine::idle();

        switch(fThread.getUiState())
        {
        case CarlaEngineNativeThread::UiNone:
        case CarlaEngineNativeThread::UiShow:
            break;
        case CarlaEngineNativeThread::UiCrashed:
            hostUiUnavailable();
            break;
        case CarlaEngineNativeThread::UiHide:
            uiClosed();
            break;
        }
    }
#endif

#if 0
    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        if (index >= getParameterCount())
            return;

        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->isEnabled())
            return;

        plugin->uiParameterChange(index, value);
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        return;

        // TODO

        // unused
        (void)channel;
        (void)bank;
        (void)program;
    }

    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        return;

        // TODO

        // unused
        (void)key;
        (void)value;
    }
#endif

#if 0
    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        QString string;
        QTextStream out(&string);

        out << "<?xml version='1.0' encoding='UTF-8'?>\n";
        out << "<!DOCTYPE CARLA-PROJECT>\n";
        out << "<CARLA-PROJECT VERSION='1.0'>\n";

        bool firstPlugin = true;
        char strBuf[STR_MAX+1];

        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr && plugin->isEnabled())
            {
                if (! firstPlugin)
                    out << "\n";

                plugin->getRealName(strBuf);

                if (*strBuf != 0)
                    out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

                QString content;
                fillXmlStringFromSaveState(content, plugin->getSaveState());

                out << " <Plugin>\n";
                out << content;
                out << " </Plugin>\n";

                firstPlugin = false;
            }
        }

        out << "</CARLA-PROJECT>\n";

        return strdup(string.toUtf8().constData());
    }

    void setState(const char* const data) override
    {
        QDomDocument xml;
        xml.setContent(QString(data));

        QDomNode xmlNode(xml.documentElement());

        if (xmlNode.toElement().tagName() != "CARLA-PROJECT")
        {
            carla_stderr2("Not a valid Carla project");
            return;
        }

        QDomNode node(xmlNode.firstChild());

        while (! node.isNull())
        {
            if (node.toElement().tagName() == "Plugin")
            {
                SaveState saveState;
                fillSaveStateFromXmlNode(saveState, node);

                CARLA_SAFE_ASSERT_CONTINUE(saveState.type != nullptr)

                const void* extraStuff = nullptr;

                // FIXME
                //if (std::strcmp(saveState.type, "DSSI") == 0)
                //    extraStuff = findDSSIGUI(saveState.binary, saveState.label);

                // TODO - proper find&load plugins
                if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
                {
                    if (CarlaPlugin* plugin = getPlugin(pData->curPluginCount-1))
                        plugin->loadSaveState(saveState);
                }
            }

            node = node.nextSibling();
        }
    }
#endif

    // -------------------------------------------------------------------

public:
    static NativePluginHandle _instantiateRack(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, false);
    }

#ifdef HAVE_JUCE
    static NativePluginHandle _instantiatePatchbay(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true);
    }
#endif

    static void _cleanup(NativePluginHandle handle)
    {
        delete (CarlaEngineNative*)handle;
    }

private:
    const bool fIsPatchbay; // rack if false
#if 0
    bool fIsRunning;
    CarlaEngineNativeThread fThread;
#endif

    CarlaPlugin* _getFirstPlugin() const noexcept
    {
        if (pData->curPluginCount == 0 || pData->plugins == nullptr)
            return nullptr;

        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->isEnabled())
            return nullptr;

        return pData->plugins[0].plugin;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNative)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor carlaRackDesc = {
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(::PLUGIN_IS_SYNTH|::PLUGIN_HAS_UI|::PLUGIN_NEEDS_FIXED_BUFFERS|::PLUGIN_NEEDS_SINGLE_THREAD|::PLUGIN_USES_STATE|::PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(::PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Rack",
    /* label     */ "carla",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiateRack,
    CarlaEngineNative::_cleanup,
    PluginDescriptorFILL2(CarlaEngineNative)
};

#ifdef HAVE_JUCE
static const NativePluginDescriptor carlaPatchbayDesc = {
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(::PLUGIN_IS_SYNTH|::PLUGIN_HAS_UI|::PLUGIN_NEEDS_FIXED_BUFFERS|::PLUGIN_NEEDS_SINGLE_THREAD|::PLUGIN_USES_STATE|::PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(::PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay",
    /* label     */ "carla",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay,
    CarlaEngineNative::_cleanup,
    PluginDescriptorFILL2(CarlaEngineNative)
};
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

CARLA_EXPORT
void carla_register_native_plugin_carla()
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_register_native_plugin(&carlaRackDesc);
#ifdef HAVE_JUCE
    carla_register_native_plugin(&carlaPatchbayDesc);
#endif
}

// -----------------------------------------------------------------------
// Extra stuff for linking purposes

#ifdef CARLA_PLUGIN_EXPORT

CARLA_BACKEND_START_NAMESPACE

CarlaEngine* CarlaEngine::newJack() { return nullptr; }

CarlaEngine*       CarlaEngine::newRtAudio(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getRtAudioApiCount() { return 0; }
const char*        CarlaEngine::getRtAudioApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getRtAudioApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const unsigned int, const char* const) { return nullptr; }

# ifdef HAVE_JUCE
CarlaEngine*       CarlaEngine::newJuce(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getJuceApiCount() { return 0; }
const char*        CarlaEngine::getJuceApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getJuceApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const unsigned int, const char* const) { return nullptr; }
# endif

CARLA_BACKEND_END_NAMESPACE

#endif

// -----------------------------------------------------------------------
