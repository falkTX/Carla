/*
 * Carla Plugin, DSSI implementation
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaEngineUtils.hpp"

#include "CarlaDssiUtils.hpp"
#include "CarlaMathUtils.hpp"

#ifdef HAVE_LIBLO
# include "CarlaOscUtils.hpp"
# include "CarlaPipeUtils.hpp"
# include "CarlaThread.hpp"
#endif

using juce::ChildProcess;
using juce::ScopedPointer;
using juce::String;
using juce::StringArray;

#define CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                 \
    /* check argument count */                                                                                                  \
    if (argc != argcToCompare)                                                                                                  \
    {                                                                                                                           \
        carla_stderr("CarlaPluginDSSI::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return;                                                                                                                 \
    }                                                                                                                           \
    if (argc > 0)                                                                                                               \
    {                                                                                                                           \
        /* check for nullness */                                                                                                \
        if (types == nullptr || typesToCompare == nullptr)                                                                      \
        {                                                                                                                       \
            carla_stderr("CarlaPluginDSSI::%s() - argument types are null", __FUNCTION__);                                      \
            return;                                                                                                             \
        }                                                                                                                       \
        /* check argument types */                                                                                              \
        if (std::strcmp(types, typesToCompare) != 0)                                                                            \
        {                                                                                                                       \
            carla_stderr("CarlaPluginDSSI::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return;                                                                                                             \
        }                                                                                                                       \
    }

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// Fallback data

static const CustomData kCustomDataFallback = { nullptr, nullptr, nullptr };

#ifdef HAVE_LIBLO
// -------------------------------------------------------------------

class CarlaThreadDSSIUI : public CarlaThread
{
public:
    CarlaThreadDSSIUI(CarlaEngine* const engine, CarlaPlugin* const plugin, const CarlaOscData& oscData) noexcept
        : CarlaThread("CarlaThreadDSSIUI"),
          kEngine(engine),
          kPlugin(plugin),
          fBinary(),
          fLabel(),
          fOscData(oscData),
          fProcess() {}

    void setData(const char* const binary, const char* const label) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(binary != nullptr && binary[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(label != nullptr /*&& label[0] != '\0'*/,);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fBinary = binary;
        fLabel  = label;

        if (fLabel.isEmpty())
            fLabel = "\"\"";
    }

    uintptr_t getProcessPID() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

        return (uintptr_t)fProcess->getPID();
    }

    void run()
    {
        if (fProcess == nullptr)
        {
            fProcess = new ChildProcess();
        }
        else if (fProcess->isRunning())
        {
            carla_stderr("CarlaThreadDSSI::run() - already running, giving up...");

            kEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, kPlugin->getId(), 0, 0, 0.0f, nullptr);
            fProcess->kill();
            fProcess = nullptr;
            return;
        }

        String name(kPlugin->getName());
        String filename(kPlugin->getFilename());

        if (name.isEmpty())
            name = "(none)";

        if (filename.isEmpty())
            filename = "\"\"";

        StringArray arguments;

        // binary
        arguments.add(fBinary.buffer());

        // osc-url
        arguments.add(String(kEngine->getOscServerPathUDP()) + String("/") + String(kPlugin->getId()));

        // filename
        arguments.add(filename);

        // label
        arguments.add(fLabel.buffer());

        // ui-title
        arguments.add(name + String(" (GUI)"));

        bool started;

        {
#ifdef CARLA_OS_LINUX
            /*
             * If the frontend uses winId parent, set LD_PRELOAD to auto-map the DSSI UI.
             * If not, unset LD_PRELOAD.
             */
            const uintptr_t winId(kEngine->getOptions().frontendWinId);

            // for CARLA_ENGINE_OPTION_FRONTEND_WIN_ID
            char winIdStr[STR_MAX+1];
            winIdStr[STR_MAX] = '\0';

            // for LD_PRELOAD
            CarlaString ldPreloadValue;

            if (winId != 0)
            {
                std::snprintf(winIdStr, STR_MAX, P_UINTPTR, kEngine->getOptions().frontendWinId);
                ldPreloadValue = (CarlaString(kEngine->getOptions().binaryDir) + CARLA_OS_SEP_STR "libcarla_interposer-x11.so");
            }

            const ScopedEngineEnvironmentLocker _seel(kEngine);
            const ScopedEnvVar _sev1("CARLA_ENGINE_OPTION_FRONTEND_WIN_ID", winIdStr[0] != '\0' ? winIdStr : nullptr);
            const ScopedEnvVar _sev2("LD_PRELOAD", ldPreloadValue.isNotEmpty() ? ldPreloadValue.buffer() : nullptr);
#endif // CARLA_OS_LINUX

            // start the DSSI UI application
            carla_stdout("starting DSSI UI...");
            started = fProcess->start(arguments);
        }

        if (! started)
        {
            carla_stdout("failed!");
            fProcess = nullptr;
            return;
        }

        if (waitForOscGuiShow())
        {
            for (; fProcess->isRunning() && ! shouldThreadExit();)
                carla_sleep(1);

            // we only get here if UI was closed or thread asked to exit
            if (fProcess->isRunning() && shouldThreadExit())
            {
                fProcess->waitForProcessToFinish(static_cast<int>(kEngine->getOptions().uiBridgesTimeout));

                if (fProcess->isRunning())
                {
                    carla_stdout("CarlaThreadDSSIUI::run() - UI refused to close, force kill now");
                    fProcess->kill();
                }
                else
                {
                    carla_stdout("CarlaThreadDSSIUI::run() - UI auto-closed successfully");
                }
            }
            else if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
                carla_stderr("CarlaThreadDSSIUI::run() - UI crashed while running");
            else
                carla_stdout("CarlaThreadDSSIUI::run() - UI closed cleanly");

            kEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, kPlugin->getId(), 0, 0, 0.0f, nullptr);
        }
        else
        {
            fProcess->kill();

            carla_stdout("CarlaThreadDSSIUI::run() - GUI timeout");
            kEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, kPlugin->getId(), 0, 0, 0.0f, nullptr);
        }

        carla_stdout("DSSI UI finished");
        fProcess = nullptr;
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    CarlaString fBinary;
    CarlaString fLabel;

    const CarlaOscData&         fOscData;
    ScopedPointer<ChildProcess> fProcess;

    bool waitForOscGuiShow()
    {
        carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow()");
        uint i=0, oscUiTimeout = kEngine->getOptions().uiBridgesTimeout;

        // wait for UI 'update' call
        for (; i < oscUiTimeout/100; ++i)
        {
            if (fOscData.target != nullptr)
            {
                carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow() - got response, asking UI to show itself now");
                osc_send_show(fOscData);
                return true;
            }

            if (fProcess != nullptr && fProcess->isRunning())
                carla_msleep(100);
            else
                return false;
        }

        carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", oscUiTimeout);
        return false;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaThreadDSSIUI)
};
#endif

// -----------------------------------------------------

class CarlaPluginDSSI : public CarlaPlugin
{
public:
    CarlaPluginDSSI(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id),
          fHandles(),
          fDescriptor(nullptr),
          fDssiDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fExtraStereoBuffer(),
          fParamBuffers(nullptr),
          fLatencyIndex(-1),
          fForcedStereoIn(false),
          fForcedStereoOut(false),
          fIsDssiVst(false),
          fUsesCustomData(false)
#ifdef HAVE_LIBLO
        , fOscData(),
          fThreadUI(engine, this, fOscData),
          fUiFilename(nullptr)
#endif
    {
        carla_debug("CarlaPluginDSSI::CarlaPluginDSSI(%p, %i)", engine, id);

        carla_zeroPointers(fExtraStereoBuffer, 2);
    }

    ~CarlaPluginDSSI() noexcept override
    {
        carla_debug("CarlaPluginDSSI::~CarlaPluginDSSI()");

#ifdef HAVE_LIBLO
        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            showCustomUI(false);

            fThreadUI.stopThread(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
        }
#endif

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->cleanup != nullptr)
            {
                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->cleanup(handle);
                    } CARLA_SAFE_EXCEPTION("LADSPA cleanup");
                }
            }

            fHandles.clear();
            fDescriptor = nullptr;
            fDssiDescriptor = nullptr;
        }

#ifdef HAVE_LIBLO
        if (fUiFilename != nullptr)
        {
            delete[] fUiFilename;
            fUiFilename = nullptr;
        }
#endif

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_DSSI;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        if (pData->audioIn.count == 0 && pData->audioOut.count > 0 && fDssiDescriptor->run_synth != nullptr)
            return PLUGIN_CATEGORY_SYNTH;

        return CarlaPlugin::getCategory();
    }

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return static_cast<int64_t>(fDescriptor->UniqueID);
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData, 0);
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->get_custom_data != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandles.count() > 0, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        *dataPtr = nullptr;

        int ret = 0;
        ulong dataSize = 0;

        try {
            ret = fDssiDescriptor->get_custom_data(fHandles.getFirst(nullptr), dataPtr, &dataSize);
        } CARLA_SAFE_EXCEPTION_RETURN("CarlaPluginDSSI::getChunkData", 0);

        return (ret != 0) ? dataSize : 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, 0x0);

        uint options = 0x0;

        if (! fIsDssiVst)
        {
            // can't disable fixed buffers if using latency
            if (fLatencyIndex == -1)
                options |= PLUGIN_OPTION_FIXED_BUFFERS;

            // can't disable forced stereo if in rack mode
            if (pData->engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
                pass();
            // if inputs or outputs are just 1, then yes we can force stereo
            else if (pData->audioIn.count == 1 || pData->audioOut.count == 1 || fForcedStereoIn || fForcedStereoOut)
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fDssiDescriptor->run_synth != nullptr)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        if (fUsesCustomData)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // bad plugins might have set output values out of bounds
        if (pData->param.data[parameterId].type == PARAMETER_OUTPUT)
            return pData->param.ranges[parameterId].getFixedValue(fParamBuffers[parameterId]);

        // not output, should be fine
        return fParamBuffers[parameterId];
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor        != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Label != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor        != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Maker != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor            != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Copyright != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor       != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Name != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,           nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr,             nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, true))
            return;

        std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, false))
            return;

        nullStrBuf(strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setId(const uint newId) noexcept
    {
        CarlaPlugin::setId(newId);

        // UI osc-url uses Id, so we need to close it when it changes
        // FIXME - must be RT safe
        showCustomUI(false);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginDSSI::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0)
            return carla_stderr2("CarlaPluginDSSI::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (fDssiDescriptor->configure != nullptr && fHandles.count() > 0)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDssiDescriptor->configure(handle, key, value);
                } catch(...) {}
            }
        }

#ifdef HAVE_LIBLO
        if (sendGui && fOscData.target != nullptr)
            osc_send_configure(fOscData, key, value);
#endif

        if (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0)
        {
            const ScopedSingleProcessLocker spl(this, true);
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData,);
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->set_custom_data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        if (fHandles.count() > 0)
        {
            const ScopedSingleProcessLocker spl(this, true);

            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDssiDescriptor->set_custom_data(handle, const_cast<void*>(data), static_cast<ulong>(dataSize));
                } CARLA_SAFE_EXCEPTION("CarlaPluginDSSI::setChunkData");
            }
        }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        const bool sendOsc(pData->engine->isOscControlRegistered());
#else
        const bool sendOsc(false);
#endif
        pData->updateParameterValues(this, sendOsc, true, false);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->select_program != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        if (index >= 0 && fHandles.count() > 0)
        {
            const uint32_t bank(pData->midiprog.data[index].bank);
            const uint32_t program(pData->midiprog.data[index].program);

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDssiDescriptor->select_program(handle, bank, program);
                } catch(...) {}
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

#ifdef HAVE_LIBLO
    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo)
        {
            fOscData.clear();
            fThreadUI.startThread();
        }
        else
        {
            pData->transientTryCounter = 0;

            if (fOscData.target != nullptr)
            {
                osc_send_hide(fOscData);
                osc_send_quit(fOscData);
                fOscData.clear();
            }

            fThreadUI.stopThread(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
        }
    }
#endif

#if 0 // TODO
    void idle() override
    {
        if (fLatencyChanged && fLatencyIndex != -1)
        {
            fLatencyChanged = false;

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency changed to %i", latency);

                    const ScopedSingleProcessLocker sspl(this, true);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);
        }

        CarlaPlugin::idle();
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandles.count() > 0,);
        carla_debug("CarlaPluginDSSI::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(getSafePortCount());

        uint32_t aIns, aOuts, mIns, params;
        aIns = aOuts = mIns = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (uint32_t i=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType(fDescriptor->PortDescriptors[i]);

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                if (LADSPA_IS_PORT_INPUT(portType))
                    aIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                    aOuts += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
                params += 1;
        }

        if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
        {
            if ((aIns == 1 || aOuts == 1) && fHandles.count() == 1 && addInstance())
            {
                if (aIns == 1)
                {
                    aIns = 2;
                    forcedStereoIn = true;
                }
                if (aOuts == 1)
                {
                    aOuts = 2;
                    forcedStereoOut = true;
                }
            }
        }

        if (fDssiDescriptor->run_synth != nullptr)
        {
            mIns = 1;
            needsCtrlIn = true;
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            pData->param.createNew(params, true);

            fParamBuffers = new float[params];
            FloatVectorOperations::clear(fParamBuffers, static_cast<int>(params));
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                if (fDescriptor->PortNames[i] != nullptr && fDescriptor->PortNames[i][0] != '\0')
                {
                    portName += fDescriptor->PortNames[i];
                }
                else
                {
                    if (LADSPA_IS_PORT_INPUT(portType))
                    {
                        if (aIns > 1)
                        {
                            portName += "audio-in_";
                            portName += CarlaString(iAudioIn+1);
                        }
                        else
                            portName += "audio-in";
                    }
                    else
                    {
                        if (aOuts > 1)
                        {
                            portName += "audio-out_";
                            portName += CarlaString(iAudioOut+1);
                        }
                        else
                            portName += "audio-out";
                    }
                }

                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    const uint32_t j = iAudioIn++;
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, 1);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    const uint32_t j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
                        pData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                const uint32_t j = iCtrl++;
                pData->param.data[j].index  = static_cast<int32_t>(j);
                pData->param.data[j].rindex = static_cast<int32_t>(i);

                const char* const paramName(fDescriptor->PortNames[i] != nullptr ? fDescriptor->PortNames[i] : "unknown");

                float min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHints.HintDescriptor))
                    min = portRangeHints.LowerBound;
                else
                    min = 0.0f;

                // max value
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHints.HintDescriptor))
                    max = portRangeHints.UpperBound;
                else
                    max = 1.0f;

                if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (min >= max)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': min >= max", paramName);
                    max = min + 0.1f;
                }

                // default value
                def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LADSPA_IS_HINT_TOGGLED(portRangeHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portRangeHints.HintDescriptor))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    const float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    pData->param.data[j].type   = PARAMETER_INPUT;
                    pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                    pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needsCtrlIn = true;

                    // MIDI CC value
                    if (fDssiDescriptor->get_midi_controller_for_port != nullptr)
                    {
                        int controller = fDssiDescriptor->get_midi_controller_for_port(fHandles.getFirst(nullptr), i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                pData->param.data[j].midiCC = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    pData->param.data[j].type = PARAMETER_OUTPUT;

                    if (std::strcmp(paramName, "latency") == 0 || std::strcmp(paramName, "_latency") == 0)
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;
                        pData->param.special[j] = PARAMETER_SPECIAL_LATENCY;
                        CARLA_SAFE_ASSERT(fLatencyIndex == -1);
                        fLatencyIndex = static_cast<int32_t>(j);
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portRangeHints.HintDescriptor))
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                fParamBuffers[j] = def;

                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->connect_port(handle, i, &fParamBuffers[j]);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port (parameter)");
                }
            }
            else
            {
                // Not Audio or Control
                carla_stderr2("ERROR - Got a broken Port (neither Audio or Control)");

                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->connect_port(handle, i, nullptr);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port (null)");
                }
            }
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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (LADSPA_IS_HARD_RT_CAPABLE(fDescriptor->Properties))
            pData->hints |= PLUGIN_IS_RTSAFE;

#ifdef HAVE_LIBLO
        if (fUiFilename != nullptr)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
#endif

#ifndef BUILD_BRIDGE
        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;
#endif

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

#if 0 // TODO
        // check latency
        if (fLatencyIndex >= 0)
        {
            // we need to pre-run the plugin so it can update its latency control-port

            float tmpIn [(aIns > 0)  ? aIns  : 1][2];
            float tmpOut[(aOuts > 0) ? aOuts : 1][2];

            for (uint32_t j=0; j < aIns; ++j)
            {
                tmpIn[j][0] = 0.0f;
                tmpIn[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[j].rindex, tmpIn[j]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port latency input");
            }

            for (uint32_t j=0; j < aOuts; ++j)
            {
                tmpOut[j][0] = 0.0f;
                tmpOut[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[j].rindex, tmpOut[j]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port latency output");
            }

            if (fDescriptor->activate != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle);
                } CARLA_SAFE_EXCEPTION("DSSI latency activate");
            }

            try {
                fDescriptor->run(fHandle, 2);
            } CARLA_SAFE_EXCEPTION("DSSI latency run");

            if (fDescriptor->deactivate != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle);
                } CARLA_SAFE_EXCEPTION("DSSI latency deactivate");
            }

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency = %i", latency);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);

            fLatencyChanged = false;
        }
#endif

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginDSSI::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginDSSI::reloadPrograms(%s)", bool2str(doInit));

        const LADSPA_Handle handle(fHandles.getFirst(nullptr));
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        const uint32_t oldCount = pData->midiprog.count;
        const int32_t  current  = pData->midiprog.current;

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t newCount = 0;
        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            for (; fDssiDescriptor->get_program(handle, newCount) != nullptr;)
                ++newCount;
        }

        if (newCount > 0)
        {
            pData->midiprog.createNew(newCount);

            // Update data
            for (uint32_t i=0; i < newCount; ++i)
            {
                const DSSI_Program_Descriptor* const pdesc(fDssiDescriptor->get_program(handle, i));
                CARLA_SAFE_ASSERT_CONTINUE(pdesc != nullptr);
                CARLA_SAFE_ASSERT(pdesc->Name != nullptr);

                pData->midiprog.data[i].bank    = static_cast<uint32_t>(pdesc->Bank);
                pData->midiprog.data[i].program = static_cast<uint32_t>(pdesc->Program);
                pData->midiprog.data[i].name    = carla_strdup(pdesc->Name);
            }
        }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        // Update OSC Names
        if (pData->engine->isOscControlRegistered() && pData->id < pData->engine->getCurrentPluginCount())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, newCount);

            for (uint32_t i=0; i < newCount; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (doInit)
        {
            if (newCount > 0)
                setMidiProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (newCount == oldCount+1)
            {
                // one midi program added, probably created by user
                pData->midiprog.current = static_cast<int32_t>(oldCount);
                programChanged = true;
            }
            else if (current < 0 && newCount > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && newCount == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(newCount))
            {
                // current midi program > count
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                pData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(pData->midiprog.current, true, true, true);

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->activate(handle);
                } CARLA_SAFE_EXCEPTION("DSSI activate");
            }
        }
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->deactivate(handle);
                } CARLA_SAFE_EXCEPTION("DSSI deactivate");
            }
        }
    }

    void process(const float** const audioIn, float** const audioOut, const float** const, float** const, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            return;
        }

        ulong midiEventCount = 0;
        carla_zeroStructs(fMidiEvents, kPluginMaxMidiEvents);

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                midiEventCount = MAX_MIDI_CHANNELS*2;

                for (uchar i=0, k=MAX_MIDI_CHANNELS; i < MAX_MIDI_CHANNELS; ++i)
                {
                    fMidiEvents[i].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[i].data.control.channel = i;
                    fMidiEvents[i].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[k+i].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[k+i].data.control.channel = i;
                    fMidiEvents[k+i].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                midiEventCount = MAX_MIDI_NOTE;

                for (uchar i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    fMidiEvents[i].type = SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[i].data.note.channel = static_cast<uchar>(pData->ctrlChannel);
                    fMidiEvents[i].data.note.note    = i;
                }
            }

#ifndef BUILD_BRIDGE
#if 0 // TODO
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latencyBuffers[i], static_cast<int>(pData->latency));
            }
#endif
#endif

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };

                for (; midiEventCount < kPluginMaxMidiEvents && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);

                    seqEvent.type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    seqEvent.data.note.channel  = static_cast<uchar>(note.channel);
                    seqEvent.data.note.note     = note.note;
                    seqEvent.data.note.velocity = note.velo;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
            bool       allNotesOffSent  = false;
#endif
            const bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, event.time - timeOffset, timeOffset, midiEventCount))
                    {
                        startTime  = 0;
                        timeOffset = event.time;
                        midiEventCount = 0;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;
                    }
                    else
                        startTime += timeOffset;
                }

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                break;
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_CONTROL)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);

                            seqEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = ctrlEvent.param;
                            seqEvent.data.control.value   = int8_t(ctrlEvent.value*127.0f);
                        }
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (uint32_t k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    const int32_t index(static_cast<int32_t>(k));
                                    setMidiProgram(index, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);

                            seqEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);

                            seqEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    if (midiEventCount >= kPluginMaxMidiEvents)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > EngineMidiEvent::kDataSize)
                        continue;

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    // Fix bad note-off (per DSSI spec)
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);

                    seqEvent.time.tick = isSampleAccurate ? startTime : event.time;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF: {
                        const uint8_t note = midiEvent.data[1];

                        seqEvent.type = SND_SEQ_EVENT_NOTEOFF;
                        seqEvent.data.note.channel = event.channel;
                        seqEvent.data.note.note    = note;

                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, note, 0.0f);
                        break;
                    }

                    case MIDI_STATUS_NOTE_ON: {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        seqEvent.type = SND_SEQ_EVENT_NOTEON;
                        seqEvent.data.note.channel  = event.channel;
                        seqEvent.data.note.note     = note;
                        seqEvent.data.note.velocity = velo;

                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, note, velo);
                        break;
                    }

                    case MIDI_STATUS_POLYPHONIC_AFTERTOUCH:
                        if (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
                        {
                            const uint8_t note     = midiEvent.data[1];
                            const uint8_t pressure = midiEvent.data[2];

                            seqEvent.type = SND_SEQ_EVENT_KEYPRESS;
                            seqEvent.data.note.channel  = event.channel;
                            seqEvent.data.note.note     = note;
                            seqEvent.data.note.velocity = pressure;
                        }
                        break;

                    case MIDI_STATUS_CONTROL_CHANGE:
                        if (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
                        {
                            const uint8_t control = midiEvent.data[1];
                            const uint8_t value   = midiEvent.data[2];

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = control;
                            seqEvent.data.control.value   = value;
                        }
                        break;

                    case MIDI_STATUS_CHANNEL_PRESSURE:
                        if (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
                        {
                            const uint8_t pressure = midiEvent.data[1];

                            seqEvent.type = SND_SEQ_EVENT_CHANPRESS;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.value   = pressure;
                        }
                        break;

                    case MIDI_STATUS_PITCH_WHEEL_CONTROL:
                        if (pData->options & PLUGIN_OPTION_SEND_PITCHBEND)
                        {
                            const uint8_t lsb = midiEvent.data[1];
                            const uint8_t msb = midiEvent.data[2];

                            seqEvent.type = SND_SEQ_EVENT_PITCHBEND;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.value   = ((msb << 7) | lsb) - 8192;
                        }
                        break;

                    default:
                        --midiEventCount;
                        break;
                    } // switch (status)
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, frames - timeOffset, timeOffset, midiEventCount);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, frames, 0, midiEventCount);

        } // End of Plugin processing (no events)

#ifndef BUILD_BRIDGE
#if 0 // TODO
        // --------------------------------------------------------------------------------------------------------
        // Latency, save values for next callback

        if (fLatencyIndex != -1)
        {
            if (pData->latency != static_cast<uint32_t>(fParamBuffers[fLatencyIndex]))
            {
                fLatencyChanged = true;
            }
            else if (pData->latency > 0)
            {
                if (pData->latency <= frames)
                {
                    for (uint32_t i=0; i < pData->audioIn.count; ++i)
                        FloatVectorOperations::copy(pData->latencyBuffers[i], audioIn[i]+(frames-pData->latency), static_cast<int>(pData->latency));
                }
                else
                {
                    for (uint32_t i=0, j, k; i < pData->audioIn.count; ++i)
                    {
                        for (k=0; k < pData->latency-frames; ++k)
                            pData->latencyBuffers[i][k] = pData->latencyBuffers[i][k+frames];
                        for (j=0; k < pData->latency; ++j, ++k)
                            pData->latencyBuffers[i][k] = audioIn[i][j];
                    }
                }
            }
        }
#endif

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                pData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (pData->param.data[k].midiCC > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].midiCC);
                    value   = pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }
        } // End of Control Output
#endif
    }

    bool processSingle(const float** const audioIn, float** const audioOut, const uint32_t frames,
                       const uint32_t timeOffset, const ulong midiEventCount)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
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
                    audioOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        const int iframes(static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Handle needsReset

        if (pData->needsReset)
        {
            if (pData->latency.frames > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latency.buffers[i], static_cast<int>(pData->latency.frames));
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        const bool customMonoOut   = pData->audioOut.count == 2 && fForcedStereoOut && ! fForcedStereoIn;
        const bool customStereoOut = pData->audioOut.count == 2 && fForcedStereoIn  && ! fForcedStereoOut;

        if (! customMonoOut)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(fAudioOutBuffers[i], iframes);
        }

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            FloatVectorOperations::copy(fAudioInBuffers[i], audioIn[i]+timeOffset, iframes);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        uint instn = 0;
        for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next(), ++instn)
        {
            LADSPA_Handle const handle(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

            // ----------------------------------------------------------------------------------------------------
            // Mixdown for forced stereo

            if (customMonoOut)
                FloatVectorOperations::clear(fAudioOutBuffers[instn], iframes);

            // ----------------------------------------------------------------------------------------------------
            // Run it

            if (fDssiDescriptor->run_synth != nullptr)
            {
                try {
                    fDssiDescriptor->run_synth(handle, frames, fMidiEvents, midiEventCount);
                } CARLA_SAFE_EXCEPTION("DSSI run_synth");
            }
            else
            {
                try {
                    fDescriptor->run(handle, frames);
                } CARLA_SAFE_EXCEPTION("DSSI run");
            }

            // ----------------------------------------------------------------------------------------------------
            // Mixdown for forced stereo

            if (customMonoOut)
                FloatVectorOperations::multiply(fAudioOutBuffers[instn], 0.5f, iframes);
            else if (customStereoOut)
                FloatVectorOperations::copy(fExtraStereoBuffer[instn], fAudioOutBuffers[instn], iframes);
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
#if 0 // TODO
                        if (k < pData->latency)
                            bufValue = pData->latencyBuffers[isMono ? 0 : i][k];
                        else if (pData->latency < frames)
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k-pData->latency];
                        else
#endif
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k];

                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], static_cast<int>(frames));
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioOutBuffers[i][k]  = oldBufLeft[k]            * (1.0f - balRangeL);
                            fAudioOutBuffers[i][k] += fAudioOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioOutBuffers[i][k]  = fAudioOutBuffers[i][k] * balRangeR;
                            fAudioOutBuffers[i][k] += oldBufLeft[k]          * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

        } // End of Post-processing

#else // BUILD_BRIDGE
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

#if 0
        for (uint32_t i=0; i < pData->cvOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                cvOut[i][k+timeOffset] = fCvOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginDSSI::bufferSizeChanged(%i) - start", newBufferSize);

        const int iBufferSize(static_cast<int>(newBufferSize));

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];

            fAudioInBuffers[i] = new float[newBufferSize];
            FloatVectorOperations::clear(fAudioInBuffers[i], iBufferSize);
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];

            fAudioOutBuffers[i] = new float[newBufferSize];
            FloatVectorOperations::clear(fAudioOutBuffers[i], iBufferSize);
        }

        if (fExtraStereoBuffer[0] != nullptr)
        {
            delete[] fExtraStereoBuffer[0];
            fExtraStereoBuffer[0] = nullptr;
        }

        if (fExtraStereoBuffer[1] != nullptr)
        {
            delete[] fExtraStereoBuffer[1];
            fExtraStereoBuffer[1] = nullptr;
        }

        if (fForcedStereoIn && pData->audioOut.count == 2)
        {
            fExtraStereoBuffer[0] = new float[newBufferSize];
            fExtraStereoBuffer[1] = new float[newBufferSize];
            FloatVectorOperations::clear(fExtraStereoBuffer[0], iBufferSize);
            FloatVectorOperations::clear(fExtraStereoBuffer[1], iBufferSize);
        }

        reconnectAudioPorts();

        carla_debug("CarlaPluginDSSI::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginDSSI::sampleRateChanged(%g) - start", newSampleRate);

        // TODO - handle UI stuff

        if (pData->active)
            deactivate();

        const std::size_t instanceCount(fHandles.count());

        if (fDescriptor->cleanup == nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->cleanup(handle);
                } CARLA_SAFE_EXCEPTION("LADSPA cleanup");
            }
        }

        fHandles.clear();

        for (std::size_t i=0; i<instanceCount; ++i)
            addInstance();

        reconnectAudioPorts();

        if (pData->active)
            activate();

        carla_debug("CarlaPluginDSSI::sampleRateChanged(%g) - end", newSampleRate);
    }

    void reconnectAudioPorts() const noexcept
    {
        if (fForcedStereoIn)
        {
            if (LADSPA_Handle const handle = fHandles.getAt(0, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port (forced stereo input)");
            }

            if (LADSPA_Handle const handle = fHandles.getAt(1, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port (forced stereo input)");
            }
        }
        else
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                {
                    try {
                        fDescriptor->connect_port(handle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port (audio input)");
                }
            }
        }

        if (fForcedStereoOut)
        {
            if (LADSPA_Handle const handle = fHandles.getAt(0, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port (forced stereo output)");
            }

            if (LADSPA_Handle const handle = fHandles.getAt(1, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port (forced stereo output)");
            }
        }
        else
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                for (uint32_t i=0; i < pData->audioOut.count; ++i)
                {
                    try {
                        fDescriptor->connect_port(handle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port (audio output)");
                }
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginDSSI::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
            {
                if (fAudioInBuffers[i] != nullptr)
                {
                    delete[] fAudioInBuffers[i];
                    fAudioInBuffers[i] = nullptr;
                }
            }

            delete[] fAudioInBuffers;
            fAudioInBuffers = nullptr;
        }

        if (fAudioOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                if (fAudioOutBuffers[i] != nullptr)
                {
                    delete[] fAudioOutBuffers[i];
                    fAudioOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioOutBuffers;
            fAudioOutBuffers = nullptr;
        }

        if (fExtraStereoBuffer[0] != nullptr)
        {
            delete[] fExtraStereoBuffer[0];
            fExtraStereoBuffer[0] = nullptr;
        }

        if (fExtraStereoBuffer[1] != nullptr)
        {
            delete[] fExtraStereoBuffer[1];
            fExtraStereoBuffer[1] = nullptr;
        }

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginDSSI::clearBuffers() - end");
    }

#ifdef HAVE_LIBLO
    // -------------------------------------------------------------------
    // OSC stuff

    void handleOscMessage(const char* const method, const int argc, const void* const argvx, const char* const types, const lo_message msg) override
    {
        const lo_address source(lo_message_get_source(msg));
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

        // protocol for DSSI UIs *must* be UDP
        CARLA_SAFE_ASSERT_RETURN(lo_address_get_protocol(source) == LO_UDP,);

        if (fOscData.source == nullptr)
        {
            // if no UI is registered yet only "update" message is valid
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(method, "update") == 0,)
        }
        else
        {
            // make sure message source is the DSSI UI
            const char* const msghost = lo_address_get_hostname(source);
            const char* const msgport = lo_address_get_port(source);

            const char* const ourhost = lo_address_get_hostname(fOscData.source);
            const char* const ourport = lo_address_get_port(fOscData.source);

            CARLA_SAFE_ASSERT_RETURN(std::strcmp(msghost, ourhost) == 0,);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(msgport, ourport) == 0,);
        }

        const lo_arg* const* const argv(static_cast<const lo_arg* const* const>(argvx));

        if (std::strcmp(method, "configure") == 0)
            return handleOscMessageConfigure(argc, argv, types);
        if (std::strcmp(method, "control") == 0)
            return handleOscMessageControl(argc, argv, types);
        if (std::strcmp(method, "program") == 0)
            return handleOscMessageProgram(argc, argv, types);
        if (std::strcmp(method, "midi") == 0)
            return handleOscMessageMIDI(argc, argv, types);
        if (std::strcmp(method, "update") == 0)
            return handleOscMessageUpdate(argc, argv, types, lo_message_get_source(msg));
        if (std::strcmp(method, "exiting") == 0)
            return handleOscMessageExiting();

        carla_stdout("CarlaPluginDSSI::handleOscMessage() - unknown method '%s'", method);
    }

    void handleOscMessageConfigure(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginDSSI::handleMsgConfigure()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "ss");

        const char* const key   = (const char*)&argv[0]->s;
        const char* const value = (const char*)&argv[1]->s;

        setCustomData(CUSTOM_DATA_TYPE_STRING, key, value, false);
    }

    void handleOscMessageControl(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginDSSI::handleMsgControl()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "if");

        const int32_t rindex = argv[0]->i;
        const float   value  = argv[1]->f;

        setParameterValueByRealIndex(rindex, value, false, true, true);
    }

    void handleOscMessageProgram(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginDSSI::handleMsgProgram()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "ii");

        const int32_t bank    = argv[0]->i;
        const int32_t program = argv[1]->i;

        CARLA_SAFE_ASSERT_RETURN(bank >= 0,);
        CARLA_SAFE_ASSERT_RETURN(program >= 0,);

        setMidiProgramById(static_cast<uint32_t>(bank), static_cast<uint32_t>(program), false, true, true);
    }

    void handleOscMessageMIDI(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginDSSI::handleMsgMidi()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(1, "m");

        if (getMidiInCount() == 0)
        {
            carla_stderr("CarlaPluginDSSI::handleMsgMidi() - received midi when plugin has no midi inputs");
            return;
        }

        const uint8_t* const data = argv[0]->m;
        uint8_t status  = data[1];
        uint8_t channel = status & 0x0F;

        // Fix bad note-off
        if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
            status = MIDI_STATUS_NOTE_OFF;

        if (MIDI_IS_STATUS_NOTE_OFF(status))
        {
            const uint8_t note = data[2];

            CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

            sendMidiSingleNote(channel, note, 0, false, true, true);
        }
        else if (MIDI_IS_STATUS_NOTE_ON(status))
        {
            const uint8_t note = data[2];
            const uint8_t velo = data[3];

            CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
            CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE,);

            sendMidiSingleNote(channel, note, velo, false, true, true);
        }
    }

    void handleOscMessageUpdate(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source)
    {
        carla_debug("CarlaPluginDSSI::handleMsgUpdate()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(1, "s");

        const char* const url = (const char*)&argv[0]->s;

        // FIXME - remove debug prints later
        carla_stdout("CarlaPluginDSSI::updateOscData(%p, \"%s\")", source, url);

        fOscData.clear();

        const int proto = lo_address_get_protocol(source);

        {
            const char* host = lo_address_get_hostname(source);
            const char* port = lo_address_get_port(source);
            fOscData.source = lo_address_new_with_proto(proto, host, port);

            carla_stdout("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);
        }

        {
            char* host = lo_url_get_hostname(url);
            char* port = lo_url_get_port(url);
            fOscData.path   = carla_strdup_free(lo_url_get_path(url));
            fOscData.target = lo_address_new_with_proto(proto, host, port);
            carla_stdout("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, fOscData.path);

            std::free(host);
            std::free(port);
        }

        osc_send_sample_rate(fOscData, static_cast<float>(pData->engine->getSampleRate()));

        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            const CustomData& customData(it.getValue(kCustomDataFallback));
            CARLA_SAFE_ASSERT_CONTINUE(customData.isValid());

            if (std::strcmp(customData.type, CUSTOM_DATA_TYPE_STRING) == 0)
                osc_send_configure(fOscData, customData.key, customData.value);
        }

        if (pData->prog.current >= 0)
            osc_send_program(fOscData, static_cast<uint32_t>(pData->prog.current));

        if (pData->midiprog.current >= 0)
        {
            const MidiProgramData& curMidiProg(pData->midiprog.getCurrent());

            if (getType() == PLUGIN_DSSI)
                osc_send_program(fOscData, curMidiProg.bank, curMidiProg.program);
            else
                osc_send_midi_program(fOscData, curMidiProg.bank, curMidiProg.program);
        }

        for (uint32_t i=0; i < pData->param.count; ++i)
            osc_send_control(fOscData, pData->param.data[i].rindex, getParameterValue(i));

        if ((pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0 && pData->engine->getOptions().frontendWinId != 0)
            pData->transientTryCounter = 1;

        carla_stdout("CarlaPluginDSSI::updateOscData() - done");
    }

    void handleOscMessageExiting()
    {
        carla_debug("CarlaPluginDSSI::handleMsgExiting()");

        // hide UI
        showCustomUI(false);

        // tell frontend
        pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (fOscData.target == nullptr)
            return;

        osc_send_control(fOscData, pData->param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        if (fOscData.target == nullptr)
            return;

        osc_send_program(fOscData, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        if (fOscData.target == nullptr)
            return;

#if 0
        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT));
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(fOscData, midiData);
#endif
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        if (fOscData.target == nullptr)
            return;

#if 0
        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT));
        midiData[2] = note;
        midiData[3] = 0;

        osc_send_midi(fOscData, midiData);
#endif
    }
#endif // HAVE_LIBLO

    // -------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fDssiDescriptor;
    }

#ifdef HAVE_LIBLO
    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fThreadUI.getProcessPID();
    }

    const void* getExtraStuff() const noexcept override
    {
        return fUiFilename;
    }
#endif

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const uint options)
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

        // ---------------------------------------------------------------
        // open DLL

        if (! pData->libOpen(filename))
        {
            pData->engine->setLastError(pData->libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const DSSI_Descriptor_Function descFn = pData->libSymbol<DSSI_Descriptor_Function>("dssi_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the DSSI Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (ulong d=0;; ++d)
        {
            try {
                fDssiDescriptor = descFn(d);
            }
            catch(...) {
                carla_stderr2("Caught exception when trying to get DSSI descriptor");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }

            if (fDssiDescriptor == nullptr)
                break;

            fDescriptor = fDssiDescriptor->LADSPA_Plugin;

            if (fDescriptor == nullptr)
            {
                carla_stderr2("WARNING - Missing LADSPA interface, will not use this plugin");
                fDssiDescriptor = nullptr;
                break;
            }
            if (fDescriptor->Label == nullptr || fDescriptor->Label[0] == '\0')
            {
                carla_stderr2("WARNING - Got an invalid label, will not use this plugin");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }
            if (fDescriptor->run == nullptr)
            {
                carla_stderr2("WARNING - Plugin has no run, cannot use it");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }

            if (std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr || fDssiDescriptor == nullptr)
        {
            pData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check if uses global instance

        if (fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
        {
            pData->engine->setLastError("This plugin requires run_multiple_synths which is not supported");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fDescriptor->Name != nullptr && fDescriptor->Name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fDescriptor->Name);
        else
            pData->name = pData->engine->getUniquePluginName(fDescriptor->Label);

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
        // initialize plugin

        if (! addInstance())
            return false;

        // ---------------------------------------------------------------
        // find latency port index

        for (uint32_t i=0, iCtrl=0, count=getSafePortCount(); i<count; ++i)
        {
            const int portType(fDescriptor->PortDescriptors[i]);

            if (! LADSPA_IS_PORT_CONTROL(portType))
                continue;

            const uint32_t index(iCtrl++);

            if (! LADSPA_IS_PORT_OUTPUT(portType))
                continue;

            const char* const portName(fDescriptor->PortNames[i]);
            CARLA_SAFE_ASSERT_BREAK(portName != nullptr);

            if (std::strcmp(portName, "latency")  == 0 ||
                std::strcmp(portName, "_latency") == 0)
            {
                fLatencyIndex = static_cast<int32_t>(index);
                break;
            }
        }

        // ---------------------------------------------------------------
        // check for custom data extension

        if (fDssiDescriptor->configure != nullptr)
        {
            if (char* const error = fDssiDescriptor->configure(fHandles.getFirst(nullptr), DSSI_CUSTOMDATA_EXTENSION_KEY, ""))
            {
                if (std::strcmp(error, "true") == 0 && fDssiDescriptor->get_custom_data != nullptr
                                                    && fDssiDescriptor->set_custom_data != nullptr)
                    fUsesCustomData = true;

                std::free(error);
            }
        }

        // ---------------------------------------------------------------
        // check if this is dssi-vst

        fIsDssiVst = CarlaString(filename).contains("dssi-vst", true);

#ifdef HAVE_LIBLO
        // ---------------------------------------------------------------
        // check for gui

        if (const char* const guiFilename = find_dssi_ui(filename, fDescriptor->Label))
        {
            fUiFilename = guiFilename;
            fThreadUI.setData(guiFilename, fDescriptor->Label);
        }
#endif

        // ---------------------------------------------------------------
        // set default options

        pData->options = 0x0;

        /**/ if (fLatencyIndex >= 0 || fIsDssiVst)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
         else if (options & PLUGIN_OPTION_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        /**/ if (pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
         else if (options & PLUGIN_OPTION_FORCE_STEREO)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fUsesCustomData)
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (fDssiDescriptor->run_synth != nullptr)
        {
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

            if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
                pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        }

        return true;
    }

    // -------------------------------------------------------------------

private:
    LinkedList<LADSPA_Handle> fHandles;
    const LADSPA_Descriptor*  fDescriptor;
    const DSSI_Descriptor*    fDssiDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fExtraStereoBuffer[2]; // used only if forcedStereoIn and audioOut == 2
    float*  fParamBuffers;

    snd_seq_event_t fMidiEvents[kPluginMaxMidiEvents];

    int32_t fLatencyIndex; // -1 if invalid
    bool    fForcedStereoIn;
    bool    fForcedStereoOut;
    bool    fIsDssiVst;
    bool    fUsesCustomData;

#ifdef HAVE_LIBLO
    CarlaOscData      fOscData;
    CarlaThreadDSSIUI fThreadUI;
    const char*       fUiFilename;
#endif

    // -------------------------------------------------------------------

    bool addInstance()
    {
        LADSPA_Handle handle;

        try {
            handle = fDescriptor->instantiate(fDescriptor, static_cast<ulong>(pData->engine->getSampleRate()));
        } CARLA_SAFE_EXCEPTION_RETURN_ERR("LADSPA instantiate", "Plugin failed to initialize");

        for (uint32_t i=0, count=pData->param.count; i<count; ++i)
        {
            const int32_t rindex(pData->param.data[i].rindex);
            CARLA_SAFE_ASSERT_CONTINUE(rindex >= 0);

            try {
                fDescriptor->connect_port(handle, static_cast<ulong>(rindex), &fParamBuffers[i]);
            } CARLA_SAFE_EXCEPTION("LADSPA connect_port");
        }

        if (fHandles.append(handle))
            return true;

        try {
            fDescriptor->cleanup(handle);
        } CARLA_SAFE_EXCEPTION("LADSPA cleanup");

        pData->engine->setLastError("Out of memory");
        return false;
    }

    uint32_t getSafePortCount() const noexcept
    {
        if (fDescriptor->PortCount == 0)
            return 0;

        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortDescriptors != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortRangeHints != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames != nullptr, 0);

        return static_cast<uint32_t>(fDescriptor->PortCount);
    }

    bool getSeparatedParameterNameOrUnit(const char* const paramName, char* const strBuf, const bool wantName) const noexcept
    {
        if (_getSeparatedParameterNameOrUnitImpl(paramName, strBuf, wantName, true))
            return true;
        if (_getSeparatedParameterNameOrUnitImpl(paramName, strBuf, wantName, false))
            return true;
        return false;
    }

    static bool _getSeparatedParameterNameOrUnitImpl(const char* const paramName, char* const strBuf,
                                                     const bool wantName, const bool useBracket) noexcept
    {
        const char* const sepBracketStart(std::strstr(paramName, useBracket ? " [" : " ("));

        if (sepBracketStart == nullptr)
            return false;

        const char* const sepBracketEnd(std::strstr(sepBracketStart, useBracket ? "]" : ")"));

        if (sepBracketEnd == nullptr)
            return false;

        const std::size_t unitSize(static_cast<std::size_t>(sepBracketEnd-sepBracketStart-2));

        if (unitSize > 7) // very unlikely to have such big unit
            return false;

        const std::size_t sepIndex(std::strlen(paramName)-unitSize-3);

        // just in case
        if (sepIndex+2 >= STR_MAX)
            return false;

        if (wantName)
        {
            std::strncpy(strBuf, paramName, sepIndex);
            strBuf[sepIndex] = '\0';
        }
        else
        {
            std::strncpy(strBuf, paramName+(sepIndex+2), unitSize);
            strBuf[unitSize] = '\0';
        }

        return true;
    }

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginDSSI)
};

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newDSSI(const Initializer& init)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\", " P_INT64 ", %x})",
                init.engine, init.filename, init.name, init.label, init.uniqueId, init.options);

    CarlaPluginDSSI* const plugin(new CarlaPluginDSSI(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label, init.options))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
