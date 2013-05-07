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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef BUILD_BRIDGE

#define WANT_LV2

#include "CarlaEngineInternal.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaNative.hpp"

#ifdef WANT_LV2
# include "lv2/lv2.h"
# include "lv2/atom.h"
struct Lv2HostDescriptor {
    uint32_t bufferSize;
    double   sampleRate;
    TimeInfo timeInfo;
    float* audioPorts[4];
    LV2_Atom_Sequence* atomPort;

    Lv2HostDescriptor()
        : bufferSize(0),
          sampleRate(0.0),
          audioPorts{nullptr},
          atomPort(nullptr) {}
};
#else
struct Lv2HostDescriptor;
#endif

#include <QtCore/QTextStream>

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaEngineNative : public PluginDescriptorClass,
                          public CarlaEngine
{
public:
    CarlaEngineNative(const HostDescriptor* const host, Lv2HostDescriptor* const lv2Host = nullptr)
        : PluginDescriptorClass(host),
          CarlaEngine(),
          fLv2Host(lv2Host)
    {
        carla_debug("CarlaEngineNative::CarlaEngineNative()");

        // set-up engine
        fOptions.processMode   = PROCESS_MODE_CONTINUOUS_RACK;
        fOptions.transportMode = TRANSPORT_MODE_PLUGIN;
        fOptions.forceStereo   = true;
        fOptions.preferPluginBridges = false;
        fOptions.preferUiBridges = false;
        init("Carla-Plugin");
    }

    ~CarlaEngineNative() override
    {
        carla_debug("CarlaEngineNative::~CarlaEngineNative()");

        setAboutToClose();
        removeAllPlugins();
        close();

#ifdef WANT_LV2
        if (fLv2Host != nullptr)
        {
            delete fLv2Host;
            delete hostHandle();
        }
#endif
    }

protected:
    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineNative::init(\"%s\")", clientName);

        fBufferSize = PluginDescriptorClass::getBufferSize();
        fSampleRate = PluginDescriptorClass::getSampleRate();

        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineNative::close()");

        proccessPendingEvents();
        return CarlaEngine::close();
    }

    bool isRunning() const override
    {
        return true;
    }

    bool isOffline() const override
    {
        return false;
    }

    EngineType type() const override
    {
        return kEngineTypePlugin;
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() override
    {
        if (kData->curPluginCount == 0 || kData->plugins == nullptr)
            return 0;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return 0;

        return kData->plugins[0].plugin->parameterCount();
    }

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= getParameterCount())
            return nullptr;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return nullptr;

        static ::Parameter param;
        static char strBufName[STR_MAX+1];
        static char strBufUnit[STR_MAX+1];

        const ParameterData& paramData(plugin->parameterData(index));
        const ParameterRanges& paramRanges(plugin->parameterRanges(index));

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

        param.hints = static_cast< ::ParameterHints>(hints);
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

    float getParameterValue(const uint32_t index) override
    {
        if (index >= getParameterCount())
            return 0.0f;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return 0.0f;

        return plugin->getParameterValue(index);
    }

    const char* getParameterText(const uint32_t index) override
    {
        if (index >= getParameterCount())
            return nullptr;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return nullptr;

        static char strBuf[STR_MAX+1];

        plugin->getParameterText(index, strBuf);

        return strBuf;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() override
    {
        if (kData->curPluginCount == 0 || kData->plugins == nullptr)
            return 0;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return 0.0f;

        return plugin->midiProgramCount();
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= getMidiProgramCount())
            return nullptr;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return nullptr;

        static ::MidiProgram midiProg;

        {
            const MidiProgramData& midiProgData(plugin->midiProgramData(index));

            midiProg.bank    = midiProgData.bank;
            midiProg.program = midiProgData.program;
            midiProg.name    = midiProgData.name;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index >= getParameterCount())
            return;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return;

        plugin->setParameterValue(index, value, false, false, false);
    }

    void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        if (kData->curPluginCount == 0 || kData->plugins == nullptr)
            return;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return;

        plugin->setMidiProgramById(bank, program, false, false, false);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        return;

        // TODO

        // unused
        (void)key;
        (void)value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        for (uint32_t i=0; i < kData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(kData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->enabled())
                continue;

            plugin->setActive(true, true, false);
        }
    }

    void deactivate() override
    {
        for (uint32_t i=0; i < kData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(kData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->enabled())
                continue;

            plugin->setActive(false, true, false);
        }

        // just in case
        proccessPendingEvents();
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const ::MidiEvent* const midiEvents) override
    {
        if (kData->curPluginCount == 0)
        {
            carla_zeroFloat(outBuffer[0], frames);
            carla_zeroFloat(outBuffer[1], frames);
            return CarlaEngine::proccessPendingEvents();
        }

        // ---------------------------------------------------------------
        // Time Info

        const ::TimeInfo* timeInfo(PluginDescriptorClass::getTimeInfo());

        fTimeInfo.playing = timeInfo->playing;
        fTimeInfo.frame   = timeInfo->frame;
        fTimeInfo.usecs   = timeInfo->usecs;
        fTimeInfo.valid   = 0x0;

        if (timeInfo->bbt.valid)
        {
            fTimeInfo.valid |= EngineTimeInfo::ValidBBT;

            fTimeInfo.bbt.bar = timeInfo->bbt.bar;
            fTimeInfo.bbt.beat = timeInfo->bbt.beat;
            fTimeInfo.bbt.tick = timeInfo->bbt.tick;
            fTimeInfo.bbt.barStartTick = timeInfo->bbt.barStartTick;

            fTimeInfo.bbt.beatsPerBar = timeInfo->bbt.beatsPerBar;
            fTimeInfo.bbt.beatType = timeInfo->bbt.beatType;

            fTimeInfo.bbt.ticksPerBeat = timeInfo->bbt.ticksPerBeat;
            fTimeInfo.bbt.beatsPerMinute = timeInfo->bbt.beatsPerMinute;
        }

        // ---------------------------------------------------------------
        // initialize input events

        carla_zeroStruct<EngineEvent>(kData->rack.in, RACK_EVENT_COUNT);
        {
            uint32_t engineEventIndex = 0;

            for (uint32_t i=0; i < midiEventCount && engineEventIndex < RACK_EVENT_COUNT; ++i)
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
                        EngineEvent& engineEvent(kData->rack.in[engineEventIndex++]);
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

                EngineEvent& engineEvent(kData->rack.in[engineEventIndex++]);
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

        float* inBuf[2]  = { inBuffer[0],  inBuffer[1] };
        float* outBuf[2] = { outBuffer[0], outBuffer[1] };

        // ---------------------------------------------------------------
        // process

        CarlaEngine::processRack(inBuf, outBuf, frames);
        CarlaEngine::proccessPendingEvents();
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        return;

        // TODO

        // unused
        (void)show;
    }

    void uiIdle() override
    {
        CarlaEngine::idle();
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        if (index >= getParameterCount())
            return;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return;

        plugin->uiParameterChange(index, value);
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        return;

        // TODO

        // unused
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

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() override
    {
        QString string;
        QTextStream out(&string);

        out << "<?xml version='1.0' encoding='UTF-8'?>\n";
        out << "<!DOCTYPE CARLA-PROJECT>\n";
        out << "<CARLA-PROJECT VERSION='1.0'>\n";

        bool firstPlugin = true;
        char strBuf[STR_MAX+1];

        for (unsigned int i=0; i < kData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin = kData->plugins[i].plugin;

            if (plugin != nullptr && plugin->enabled())
            {
                if (! firstPlugin)
                    out << "\n";

                plugin->getRealName(strBuf);

                if (*strBuf != 0)
                    out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

                out << " <Plugin>\n";
                out << getXMLFromSaveState(plugin->getSaveState());
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
                const SaveState& saveState(getSaveStateDictFromXML(node));
                CARLA_ASSERT(saveState.type != nullptr);

                if (saveState.type == nullptr)
                    continue;

                const void* extraStuff = nullptr;

                // FIXME
                //if (std::strcmp(saveState.type, "DSSI") == 0)
                //    extraStuff = findDSSIGUI(saveState.binary, saveState.label);

                // TODO - proper find&load plugins
                if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
                {
                    if (CarlaPlugin* plugin = getPlugin(kData->curPluginCount-1))
                        plugin->loadSaveState(saveState);
                }
            }

            node = node.nextSibling();
        }
    }

    // -------------------------------------------------------------------

#ifdef WANT_LV2
    Lv2HostDescriptor* const fLv2Host;

    // -------------------------------------------------------------------

public:
    #define handlePtr ((Lv2HostDescriptor*)handle)

    static uint32_t lv2host_get_buffer_size(HostHandle handle)
    {
        return handlePtr->bufferSize;
    }

    static double lv2host_get_sample_rate(HostHandle handle)
    {
        return handlePtr->sampleRate;
    }

    static const TimeInfo* lv2host_get_time_info(HostHandle handle)
    {
        return &handlePtr->timeInfo;
    }

    static bool lv2host_write_midi_event(HostHandle, const MidiEvent*)
    {
        // MIDI Out not supported yet
        return false;
    }

    static void lv2host_ui_parameter_changed(HostHandle, uint32_t, float) {}
    static void lv2host_ui_midi_program_changed(HostHandle, uint8_t, uint32_t, uint32_t) {}
    static void lv2host_ui_custom_data_changed(HostHandle, const char*, const char*) {}
    static void lv2host_ui_closed(HostHandle) {}
    static const char* lv2host_ui_open_file(HostHandle, bool, const char*, const char*) { return nullptr; }
    static const char* lv2host_ui_save_file(HostHandle, bool, const char*, const char*) { return nullptr; }
    static intptr_t lv2host_dispatcher(HostHandle, HostDispatcherOpcode, int32_t, intptr_t, void*) { return 0; }

    #undef handlePtr

    // -------------------------------------------------------------------

    #define handlePtr ((CarlaEngineNative*)handle)

    static LV2_Handle lv2_instantiate(const LV2_Descriptor*, double sampleRate, const char*, const LV2_Feature* const* /*features*/)
    {
        Lv2HostDescriptor* const lv2Host(new Lv2HostDescriptor());
        lv2Host->bufferSize = 1024; // TODO
        lv2Host->sampleRate = sampleRate;
        lv2Host->timeInfo.frame = 0;
        lv2Host->timeInfo.usecs = 0;
        lv2Host->timeInfo.playing = false;
        lv2Host->timeInfo.bbt.valid = false;

        HostDescriptor* const host(new HostDescriptor);
        host->handle  = lv2Host;
        host->ui_name = nullptr;

        host->get_buffer_size        = lv2host_get_buffer_size;
        host->get_sample_rate        = lv2host_get_sample_rate;
        host->get_time_info          = lv2host_get_time_info;
        host->write_midi_event       = lv2host_write_midi_event;
        host->ui_parameter_changed   = lv2host_ui_parameter_changed;
        host->ui_custom_data_changed = lv2host_ui_custom_data_changed;
        host->ui_closed              = lv2host_ui_closed;
        host->ui_open_file           = lv2host_ui_open_file;
        host->ui_save_file           = lv2host_ui_save_file;
        host->dispatcher             = lv2host_dispatcher;

        return new CarlaEngineNative(host, lv2Host);
    }

    static void lv2_connect_port(LV2_Handle handle, uint32_t port, void* dataLocation)
    {
        if (port < 4)
            handlePtr->fLv2Host->audioPorts[port] = (float*)dataLocation;
        else if (port == 4)
            handlePtr->fLv2Host->atomPort = (LV2_Atom_Sequence*)dataLocation;
    }

    static void lv2_activate(LV2_Handle handle)
    {
        handlePtr->activate();
    }

    static void lv2_run(LV2_Handle handle, uint32_t sampleCount)
    {
        float* inBuffer[2]  = { handlePtr->fLv2Host->audioPorts[0], handlePtr->fLv2Host->audioPorts[1] };
        float* outBuffer[2] = { handlePtr->fLv2Host->audioPorts[2], handlePtr->fLv2Host->audioPorts[3] };

        // TODO - get midiEvents and timePos from atomPort

        handlePtr->process(inBuffer, outBuffer, sampleCount, 0, nullptr);
    }

    static void lv2_deactivate(LV2_Handle handle)
    {
        handlePtr->deactivate();
    }

    static void lv2_cleanup(LV2_Handle handle)
    {
        delete handlePtr;
    }

    static const void* lv2_extension_data(const char* /*uri*/)
    {
        return nullptr;
    }

    #undef handlePtr
#endif

private:
    PluginDescriptorClassEND(CarlaEngineNative)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNative)
};

// -----------------------------------------------------------------------

static const PluginDescriptor carlaDesc = {
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast< ::PluginHints>(::PLUGIN_IS_SYNTH|::PLUGIN_USES_SINGLE_THREAD|::PLUGIN_USES_STATE),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Plugin",
    /* label     */ "carla",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(CarlaEngineNative)
};

#ifdef WANT_LV2
static const LV2_Descriptor carlaLv2Desc = {
    /* URI            */ "http://kxstudio.sf.net/carla",
    /* instantiate    */ CarlaEngineNative::lv2_instantiate,
    /* connect_port   */ CarlaEngineNative::lv2_connect_port,
    /* activate       */ CarlaEngineNative::lv2_activate,
    /* run            */ CarlaEngineNative::lv2_run,
    /* deactivate     */ CarlaEngineNative::lv2_deactivate,
    /* cleanup        */ CarlaEngineNative::lv2_cleanup,
    /* extension_data */ CarlaEngineNative::lv2_extension_data
};
#endif

void CarlaEngine::registerNativePlugin()
{
    carla_register_native_plugin(&carlaDesc);
}

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

#ifdef WANT_LV2
CARLA_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    CARLA_BACKEND_USE_NAMESPACE;
    return (index == 0) ? &carlaLv2Desc : nullptr;
}
#endif

// -----------------------------------------------------------------------

#endif // ! BUILD_BRIDGE
