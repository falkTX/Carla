/*
 * Carla Juce Plugin
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef HAVE_JUCE

#include "CarlaBackendUtils.hpp"
#include "JucePluginWindow.hpp"

#include "juce_audio_processors.h"

using namespace juce;

CARLA_BACKEND_START_NAMESPACE

class JucePlugin : public CarlaPlugin
{
public:
    JucePlugin(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInstance(nullptr),
          fAudioBuffer(1, 0)
    {
        carla_debug("JucePlugin::JucePlugin(%p, %i)", engine, id);

        fMidiBuffer.ensureSize(2048);
        fMidiBuffer.clear();
    }

    ~JucePlugin() override
    {
        carla_debug("JucePlugin::~JucePlugin()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            showCustomUI(false);

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fInstance != nullptr)
        {
            delete fInstance;
            fInstance = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        PluginType type = PLUGIN_NONE;

        try {
            type = getPluginTypeFromString(fDesc.pluginFormatName.toRawUTF8());
        } catch(...) {}

        return type;
    }

    PluginCategory getCategory() const noexcept override
    {
        PluginCategory category = PLUGIN_CATEGORY_NONE;

        try {
            category = getPluginCategoryFromName(fDesc.category.toRawUTF8());
        } catch(...) {}

        return category;
    }

    long getUniqueId() const noexcept override
    {
        return fDesc.uid;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr, 0x0);

        unsigned int options = 0x0;

        //options |= PLUGIN_OPTION_FIXED_BUFFERS;
        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        //options |= PLUGIN_OPTION_USE_CHUNKS;

        if (fInstance->acceptsMidi())
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr, 0.0f);

        return fInstance->getParameter(static_cast<int>(parameterId));
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fDesc.name.toRawUTF8(), STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fDesc.manufacturerName.toRawUTF8(), STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getName().toRawUTF8(), STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getParameterName(static_cast<int>(parameterId)).toRawUTF8(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getParameterLabel(static_cast<int>(parameterId)).toRawUTF8(), STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fWindow != nullptr)
        {
            String uiName(pData->name);
            uiName += " (GUI)";
            fWindow->setName(uiName);
        }
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fInstance->setParameter(static_cast<int>(parameterId), value);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

#ifdef CARLA_OS_LINUX
        const MessageManagerLock mmLock;
#endif

        if (yesNo)
        {
            if (fWindow == nullptr)
            {
                String uiName(pData->name);
                uiName += " (GUI)";

                fWindow = new JucePluginWindow();
                fWindow->setName(uiName);
            }

            if (AudioProcessorEditor* const editor = fInstance->createEditorIfNeeded())
                fWindow->show(editor);
        }
        else
        {
            if (fWindow != nullptr)
                fWindow->hide();

            if (AudioProcessorEditor* const editor = fInstance->getActiveEditor())
                delete editor;

            fWindow = nullptr;
        }
    }

    void idle() override
    {
        if (fWindow != nullptr)
        {
            if (fWindow->wasClosedByUser())
            {
                showCustomUI(false);
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
            }
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);
        carla_debug("VstPlugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        fInstance->refreshParameterList();

        uint32_t aIns, aOuts, mIns, mOuts, params;
        aIns = aOuts = mIns = mOuts = params = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = (fInstance->getNumInputChannels() > 0)  ? static_cast<uint32_t>(fInstance->getNumInputChannels())  : 0;
        aOuts  = (fInstance->getNumOutputChannels() > 0) ? static_cast<uint32_t>(fInstance->getNumOutputChannels()) : 0;
        params = (fInstance->getNumParameters() > 0)     ? static_cast<uint32_t>(fInstance->getNumParameters())     : 0;

        if (fInstance->acceptsMidi())
        {
            mIns = 1;
            needsCtrlIn = true;
        }

        if (fInstance->producesMidi())
        {
            mOuts = 1;
            needsCtrlOut = true;
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = 0x0;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(j);
            pData->param.data[j].midiCC = -1;
            pData->param.data[j].midiChannel = 0;

            float min, max, def, step, stepSmall, stepLarge;

            // TODO
            //const int numSteps(fInstance->getParameterNumSteps(static_cast<int>(j)));
            {
                min = 0.0f;
                max = 1.0f;
                step = 0.001f;
                stepSmall = 0.0001f;
                stepLarge = 0.1f;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
#ifndef BUILD_BRIDGE
            //pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
#endif

            if (fInstance->isParameterAutomatable(static_cast<int>(j)))
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            // FIXME?
            def = fInstance->getParameterDefaultValue(static_cast<int>(j));

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin hints
        pData->hints = 0x0;

        if (fDesc.isInstrument)
           pData->hints |= PLUGIN_IS_SYNTH;

        if (fInstance->hasEditor())
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
            pData->hints |= PLUGIN_NEEDS_SINGLE_THREAD;
        }

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        fInstance->setPlayConfigDetails(static_cast<int>(aIns), static_cast<int>(aOuts), pData->engine->getSampleRate(), static_cast<int>(pData->engine->getBufferSize()));

        bufferSizeChanged(pData->engine->getBufferSize());
        //reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("VstPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        try {
            fInstance->prepareToPlay(pData->engine->getSampleRate(), static_cast<int>(pData->engine->getBufferSize()));
        } catch(...) {}
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        try {
            fInstance->releaseResources();
        } catch(...) {}
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            fInstance->reset();
            pData->needsReset = false;
        }

        uint32_t l=0;
        for (; l < pData->audioIn.count; ++l)
            fAudioBuffer.clear(static_cast<int>(l), 0, static_cast<int>(frames));
        for (; l < pData->audioOut.count; ++l)
            fAudioBuffer.clear(static_cast<int>(l), 0, static_cast<int>(frames));

        processSingle(inBuffer, outBuffer, frames);
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k/*+timeOffset*/] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio in buffers

        uint32_t l;
        for (l=0; l < pData->audioIn.count; ++l)
            fAudioBuffer.copyFrom(static_cast<int>(l), 0, inBuffer[l], static_cast<int>(frames));
        //for (l=0; l < pData->audioOut.count; ++l)
        //    fAudioBuffer.clear(static_cast<int>(l), 0, static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fInstance->processBlock(fAudioBuffer, fMidiBuffer);

        // --------------------------------------------------------------------------------------------------------
        // Set audio out buffers

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::copy(outBuffer[i], fAudioBuffer.getSampleData(static_cast<int>(i)), static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("VstPlugin::bufferSizeChanged(%i)", newBufferSize);

        fAudioBuffer.setSize(static_cast<int>(std::max<uint32_t>(pData->audioIn.count, pData->audioOut.count)), static_cast<int>(newBufferSize));

        if (pData->active)
        {
            deactivate();
            activate();
        }
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("VstPlugin::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
        {
            deactivate();
            activate();
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    // TODO

    // -------------------------------------------------------------------

public:
    bool init(const char* const filename, const char* const name, const char* const label, const char* const format)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

#ifdef CARLA_OS_LINUX
        const MessageManagerLock mmLock;
#endif

        // ---------------------------------------------------------------
        // fix path for wine usage

        String jfilename(filename);

#ifdef CARLA_OS_WIN
        if (jfilename.startsWith("/"))
        {
            jfilename.replace("/", "\\");
            jfilename = "Z:" + jfilename;
        }
#endif

        //fDesc.name = fDesc.descriptiveName = label;
        fDesc.uid = 0; // TODO - set uid for shell plugins
        fDesc.fileOrIdentifier = jfilename;
        fDesc.pluginFormatName = format;

        fFormatManager.addDefaultFormats();

        String error;
        fInstance = fFormatManager.createPluginInstance(fDesc, 44100, 512, error);

        if (fInstance == nullptr)
        {
            pData->engine->setLastError(error.toRawUTF8());
            return false;
        }

        fInstance->fillInPluginDescription(fDesc);

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(fInstance->getName().toRawUTF8());

        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            //pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
            //pData->options |= PLUGIN_OPTION_USE_CHUNKS;

            if (fInstance->acceptsMidi())
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // set identifier string
            String juceId(fDesc.createIdentifierString());

            CarlaString identifier("Juce/");
            identifier += juceId.toRawUTF8();
            pData->identifier = identifier.dup();

            // load settings
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());
        }

        return true;
    }

private:
    PluginDescription    fDesc;
    AudioPluginInstance* fInstance;
    AudioPluginFormatManager fFormatManager;

    AudioSampleBuffer fAudioBuffer;
    MidiBuffer        fMidiBuffer;

    ScopedPointer<JucePluginWindow> fWindow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_JUCE

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newJuce(const Initializer& init, const char* const format)
{
    carla_debug("CarlaPlugin::newJuce({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, format);

#ifdef HAVE_JUCE
    JucePlugin* const plugin(new JucePlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label, format))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo VST3 plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("Juce support not available");
    return nullptr;

    // unused
    (void)format;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
