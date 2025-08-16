// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngineUtils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaLadspaUtils.hpp"
#include "CarlaDssiUtils.hpp"
#include "CarlaMathUtils.hpp"

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE) && !defined(CARLA_OS_WASM)
# define CARLA_ENABLE_DSSI_PLUGIN_GUI
# include "CarlaOscUtils.hpp"
# include "CarlaScopeUtils.hpp"
# include "CarlaThread.hpp"
# include "extra/ScopedPointer.hpp"
# include "water/threads/ChildProcess.h"
using water::ChildProcess;
#endif

#define CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare) \
    /* check argument count */                                                                  \
    if (argc != argcToCompare)                                                                  \
    {                                                                                           \
        carla_stderr("CarlaPluginLADSPADSSI::%s() - argument count mismatch: %i != %i",         \
                     __FUNCTION__, argc, argcToCompare);                                        \
        return;                                                                                 \
    }                                                                                           \
    if (argc > 0)                                                                               \
    {                                                                                           \
        /* check for nullness */                                                                \
        if (types == nullptr || typesToCompare == nullptr)                                      \
        {                                                                                       \
            carla_stderr("CarlaPluginLADSPADSSI::%s() - argument types are null",               \
                         __FUNCTION__);                                                         \
            return;                                                                             \
        }                                                                                       \
        /* check argument types */                                                              \
        if (std::strcmp(types, typesToCompare) != 0)                                            \
        {                                                                                       \
            carla_stderr("CarlaPluginLADSPADSSI::%s() - argument types mismatch: '%s' != '%s'", \
                         __FUNCTION__, types, typesToCompare);                                  \
            return;                                                                             \
        }                                                                                       \
    }

CARLA_BACKEND_START_NAMESPACE

#ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
// -------------------------------------------------------------------
// Fallback data

static const CustomData kCustomDataFallback = { nullptr, nullptr, nullptr };

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
          fUiTitle(),
          fOscData(oscData),
          fProcess() {}

    void setData(const char* const binary, const char* const label, const char* const uiTitle) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(binary != nullptr && binary[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(label != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(uiTitle != nullptr && uiTitle[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fBinary  = binary;
        fLabel   = label;
        fUiTitle = uiTitle;

        if (fLabel.isEmpty())
            fLabel = "\"\"";
    }

    void setUITitle(const char* const uiTitle) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(uiTitle != nullptr && uiTitle[0] != '\0',);

        fUiTitle = uiTitle;
    }

    uintptr_t getProcessId() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

        return (uintptr_t)fProcess->getPID();
    }

    void run()
    {
        carla_stdout("LADSPA/DSSI UI thread started");

        if (fProcess == nullptr)
        {
            fProcess = new ChildProcess();
        }
        else if (fProcess->isRunning())
        {
            carla_stderr("CarlaThreadDSSI::run() - already running, giving up...");

            fProcess->kill();
            fProcess = nullptr;
            kEngine->callback(true, true,
                              ENGINE_CALLBACK_UI_STATE_CHANGED,
                              kPlugin->getId(),
                              0,
                              0, 0, 0.0f, nullptr);
            return;
        }

        water::String name(kPlugin->getName());
        water::String filename(kPlugin->getFilename());

        if (name.isEmpty())
            name = "(none)";

        if (filename.isEmpty())
            filename = "\"\"";

        water::StringArray arguments;

        // binary
        arguments.add(fBinary.buffer());

        // osc-url
        arguments.add(String(kEngine->getOscServerPathUDP()) + water::String("/") + water::String(kPlugin->getId()));

        // filename
        arguments.add(filename);

        // label
        arguments.add(fLabel.buffer());

        // ui-title
        arguments.add(fUiTitle.buffer());

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
            String ldPreloadValue;

            if (winId != 0)
            {
                std::snprintf(winIdStr, STR_MAX, P_UINTPTR, winId);
                ldPreloadValue = (String(kEngine->getOptions().binaryDir)
                               + "/libcarla_interposer-x11.so");
            }
            else
            {
                winIdStr[0] = '\0';
            }

            const ScopedEngineEnvironmentLocker _seel(kEngine);
            const CarlaScopedEnvVar _sev1("CARLA_ENGINE_OPTION_FRONTEND_WIN_ID", winIdStr[0] != '\0' ? winIdStr : nullptr);
            const CarlaScopedEnvVar _sev2("LD_PRELOAD", ldPreloadValue.isNotEmpty() ? ldPreloadValue.buffer() : nullptr);
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
                d_msleep(100);

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
            else if (fProcess->getExitCodeAndClearPID() != 0)
                carla_stderr("CarlaThreadDSSIUI::run() - UI crashed while running");
            else
                carla_stdout("CarlaThreadDSSIUI::run() - UI closed cleanly");
        }
        else
        {
            fProcess->kill();
            carla_stdout("CarlaThreadDSSIUI::run() - GUI timeout");
        }

        fProcess = nullptr;
        kEngine->callback(true, true,
                          ENGINE_CALLBACK_UI_STATE_CHANGED,
                          kPlugin->getId(),
                          0,
                          0, 0, 0.0f, nullptr);

        carla_stdout("LADSPA/DSSI UI thread finished");
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    String fBinary;
    String fLabel;
    String fUiTitle;

    const CarlaOscData& fOscData;
    ScopedPointer<ChildProcess> fProcess;

    bool waitForOscGuiShow()
    {
        carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow()");
        const uint uiBridgesTimeout = kEngine->getOptions().uiBridgesTimeout;

        // wait for UI 'update' call
        for (uint i=0; i < uiBridgesTimeout/100; ++i)
        {
            if (fOscData.target != nullptr)
            {
                carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow() - got response, asking UI to show itself now");
                osc_send_show(fOscData);
                return true;
            }

            if (fProcess != nullptr && fProcess->isRunning() && ! shouldThreadExit())
                d_msleep(100);
            else
                return false;
        }

        carla_stdout("CarlaThreadDSSIUI::waitForOscGuiShow() - Timeout while waiting for UI to respond"
                     "(waited %u msecs)", uiBridgesTimeout);
        return false;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaThreadDSSIUI)
};
#endif

// -----------------------------------------------------

class CarlaPluginLADSPADSSI : public CarlaPlugin
{
public:
    CarlaPluginLADSPADSSI(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id),
          fHandles(),
          fDescriptor(nullptr),
          fDssiDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fExtraStereoBuffer(),
          fParamBuffers(nullptr),
          fLatencyIndex(-1),
          fForcedStereoIn(false),
          fForcedStereoOut(false),
          fNeedsFixedBuffers(false),
          fUsesCustomData(false)
       #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
        , fOscData(),
          fThreadUI(engine, this, fOscData),
          fUiFilename(nullptr)
       #endif
    {
        carla_debug("CarlaPluginLADSPADSSI::CarlaPluginLADSPADSSI(%p, %i)", engine, id);

        carla_zeroPointers(fExtraStereoBuffer, 2);
    }

    ~CarlaPluginLADSPADSSI() noexcept override
    {
        carla_debug("CarlaPluginLADSPADSSI::~CarlaPluginLADSPADSSI()");

       #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
        // close UI
        if (fUiFilename != nullptr)
        {
            showCustomUI(false);

            delete[] fUiFilename;
            fUiFilename = nullptr;
        }
       #endif

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

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
                    } CARLA_SAFE_EXCEPTION("LADSPA/DSSI cleanup");
                }
            }

            fHandles.clear();
            fDescriptor = nullptr;
            fDssiDescriptor = nullptr;
        }

        if (fRdfDescriptor != nullptr)
        {
            delete fRdfDescriptor;
            fRdfDescriptor = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return fDssiDescriptor != nullptr ? PLUGIN_DSSI : PLUGIN_LADSPA;
    }

    PluginCategory getCategory() const noexcept override
    {
        if (fRdfDescriptor != nullptr)
        {
            const LADSPA_RDF_PluginType category = fRdfDescriptor->Type;

            // Specific Types
            if (category & (LADSPA_RDF_PLUGIN_DELAY|LADSPA_RDF_PLUGIN_REVERB))
                return PLUGIN_CATEGORY_DELAY;
            if (category & (LADSPA_RDF_PLUGIN_PHASER|LADSPA_RDF_PLUGIN_FLANGER|LADSPA_RDF_PLUGIN_CHORUS))
                return PLUGIN_CATEGORY_MODULATOR;
            if (category & (LADSPA_RDF_PLUGIN_AMPLIFIER))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (category & (LADSPA_RDF_PLUGIN_UTILITY|LADSPA_RDF_PLUGIN_SPECTRAL|LADSPA_RDF_PLUGIN_FREQUENCY_METER))
                return PLUGIN_CATEGORY_UTILITY;

            // Pre-set LADSPA Types
            if (LADSPA_RDF_IS_PLUGIN_DYNAMICS(category))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (LADSPA_RDF_IS_PLUGIN_AMPLITUDE(category))
                return PLUGIN_CATEGORY_MODULATOR;
            if (LADSPA_RDF_IS_PLUGIN_EQ(category))
                return PLUGIN_CATEGORY_EQ;
            if (LADSPA_RDF_IS_PLUGIN_FILTER(category))
                return PLUGIN_CATEGORY_FILTER;
            if (LADSPA_RDF_IS_PLUGIN_FREQUENCY(category))
                return PLUGIN_CATEGORY_UTILITY;
            if (LADSPA_RDF_IS_PLUGIN_SIMULATOR(category))
                return PLUGIN_CATEGORY_OTHER;
            if (LADSPA_RDF_IS_PLUGIN_TIME(category))
                return PLUGIN_CATEGORY_DELAY;
            if (LADSPA_RDF_IS_PLUGIN_GENERATOR(category))
                return PLUGIN_CATEGORY_SYNTH;
        }

        if (fDssiDescriptor != nullptr && fDssiDescriptor->run_synth != nullptr)
            if (pData->audioIn.count == 0 && pData->audioOut.count > 0)
                return PLUGIN_CATEGORY_SYNTH;

        return CarlaPlugin::getCategory();
    }

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return static_cast<int64_t>(fDescriptor->UniqueID);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        if (fLatencyIndex < 0 || fParamBuffers == nullptr)
            return 0;

        const float latency(fParamBuffers[fLatencyIndex]);
        CARLA_SAFE_ASSERT_RETURN(latency >= 0.0f, 0);

        return static_cast<uint32_t>(latency);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        if (fRdfDescriptor == nullptr)
            return 0;

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, 0);

        if (rindex >= static_cast<int32_t>(fRdfDescriptor->PortCount))
            return 0;

        const LADSPA_RDF_Port& port(fRdfDescriptor->Ports[rindex]);
        return static_cast<uint32_t>(port.ScalePointCount);
    }

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
        } CARLA_SAFE_EXCEPTION_RETURN("CarlaPluginLADSPADSSI::getChunkData", 0);

        return (ret != 0) ? dataSize : 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        // can't disable fixed buffers if using latency
        if (fLatencyIndex == -1 && ! fNeedsFixedBuffers)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        // can't disable forced stereo if enabled in the engine
        if (pData->engine->getOptions().forceStereo)
            pass();
        // if inputs or outputs are just 1, then yes we can force stereo
        else if (pData->audioIn.count == 1 || pData->audioOut.count == 1 || fForcedStereoIn || fForcedStereoOut)
            options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fDssiDescriptor != nullptr)
        {
            if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
                options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (fUsesCustomData)
                options |= PLUGIN_OPTION_USE_CHUNKS;

            if (fDssiDescriptor->run_synth != nullptr)
            {
                options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
                options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                options |= PLUGIN_OPTION_SEND_PITCHBEND;
                options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
                options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
            }
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,         0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // bad plugins might have set output values out of bounds
        if (pData->param.data[parameterId].type == PARAMETER_OUTPUT)
            return pData->param.ranges[parameterId].getFixedValue(fParamBuffers[parameterId]);

        // not output, should be fine
        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,        0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                                              0.0f);
        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fRdfDescriptor->PortCount), 0.0f);

        const LADSPA_RDF_Port& port(fRdfDescriptor->Ports[rindex]);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < port.ScalePointCount, 0.0f);

        const LADSPA_RDF_ScalePoint& scalePoint(port.ScalePoints[scalePointId]);
        return pData->param.ranges[parameterId].getFixedValue(scalePoint.Value);
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Label != nullptr, false);

        std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Maker != nullptr, false);

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Creator != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Creator, STR_MAX);
        else
            std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);

        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Copyright != nullptr, false);

        std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        return true;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Name != nullptr, false);

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Title, STR_MAX);
        else
            std::strncpy(strBuf, fDescriptor->Name, STR_MAX);

        return true;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr, false);

        if (! getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, true))
            std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);

        return true;
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (LADSPA_RDF_PORT_HAS_UNIT(port.Hints))
            {
                switch (port.Unit)
                {
                case LADSPA_RDF_UNIT_DB:
                    std::strncpy(strBuf, "dB", STR_MAX);
                    return true;
                case LADSPA_RDF_UNIT_COEF:
                    std::strncpy(strBuf, "(coef)", STR_MAX);
                    return true;
                case LADSPA_RDF_UNIT_HZ:
                    std::strncpy(strBuf, "Hz", STR_MAX);
                    return true;
                case LADSPA_RDF_UNIT_S:
                    std::strncpy(strBuf, "s", STR_MAX);
                    return true;
                case LADSPA_RDF_UNIT_MS:
                    std::strncpy(strBuf, "ms", STR_MAX);
                    return true;
                case LADSPA_RDF_UNIT_MIN:
                    std::strncpy(strBuf, "min", STR_MAX);
                    return true;
                }
            }
        }

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr, false);

        return getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, false);
    }

    bool getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        if (fRdfDescriptor == nullptr)
            return false;

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex >= static_cast<int32_t>(fRdfDescriptor->PortCount))
            return false;

        const LADSPA_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

        if (! LADSPA_RDF_PORT_HAS_LABEL(port.Hints))
            return false;

        CARLA_SAFE_ASSERT_RETURN(port.Label != nullptr, false);

        std::strncpy(strBuf, port.Label, STR_MAX);
        return true;
    }

    bool getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fRdfDescriptor->PortCount), false);

        const LADSPA_RDF_Port& port(fRdfDescriptor->Ports[rindex]);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < port.ScalePointCount, false);

        const LADSPA_RDF_ScalePoint& scalePoint(port.ScalePoints[scalePointId]);
        CARLA_SAFE_ASSERT_RETURN(scalePoint.Label != nullptr, false);

        std::strncpy(strBuf, scalePoint.Label, STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setId(const uint newId) noexcept override
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

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginLADSPADSSI::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0)
            return carla_stderr2("CarlaPluginLADSPADSSI::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string",
                                 type, key, value, bool2str(sendGui));

        if (fDssiDescriptor->configure != nullptr && fHandles.count() > 0)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDssiDescriptor->configure(handle, key, value);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI setCustomData");
            }
        }

       #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
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
                } CARLA_SAFE_EXCEPTION("CarlaPluginLADSPADSSI::setChunkData");
            }
        }

        pData->updateParameterValues(this, true, true, false);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->select_program != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        if (index >= 0 && fHandles.count() > 0)
        {
            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            setMidiProgramInDSSI(static_cast<uint32_t>(index));
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setMidiProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->select_program != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(uindex < pData->midiprog.count,);

        setMidiProgramInDSSI(uindex);

        CarlaPlugin::setMidiProgramRT(uindex, sendCallbackLater);
    }

    void setMidiProgramInDSSI(const uint32_t uindex) noexcept
    {
        const uint32_t bank(pData->midiprog.data[uindex].bank);
        const uint32_t program(pData->midiprog.data[uindex].program);

        for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
        {
            LADSPA_Handle const handle(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

            try {
                fDssiDescriptor->select_program(handle, bank, program);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI setMidiProgram")
        }
    }

   #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
    // -------------------------------------------------------------------
    // Set ui stuff

    void setCustomUITitle(const char* title) noexcept override
    {
        fThreadUI.setUITitle(title);
        CarlaPlugin::setCustomUITitle(title);
    }

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo)
        {
            fOscData.clear();
            fThreadUI.startThread();
        }
        else
        {
           #ifndef BUILD_BRIDGE
            pData->transientTryCounter = 0;
           #endif

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

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandles.count() > 0,);
        carla_debug("CarlaPluginLADSPADSSI::reload() - start");

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

        if (fDssiDescriptor != nullptr && fDssiDescriptor->run_synth != nullptr)
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
            carla_zeroFloats(fParamBuffers, params);
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        String portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];
            const bool hasPortRDF = (fRdfDescriptor != nullptr && i < fRdfDescriptor->PortCount);

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
                            portName += String(iAudioIn+1);
                        }
                        else
                            portName += "audio-in";
                    }
                    else
                    {
                        if (aOuts > 1)
                        {
                            portName += "audio-out_";
                            portName += String(iAudioOut+1);
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
                if (hasPortRDF && LADSPA_RDF_PORT_HAS_DEFAULT(fRdfDescriptor->Ports[i].Hints))
                    def = fRdfDescriptor->Ports[i].Default;
                else
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
                    pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                    pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
                    needsCtrlIn = true;

                    // MIDI CC value
                    if (fDssiDescriptor != nullptr && fDssiDescriptor->get_midi_controller_for_port != nullptr)
                    {
                        const int ctrl = fDssiDescriptor->get_midi_controller_for_port(fHandles.getFirst(nullptr), i);
                        if (DSSI_CONTROLLER_IS_SET(ctrl) && DSSI_IS_CC(ctrl))
                        {
                            const int16_t cc = DSSI_CC_NUMBER(ctrl);
                            if (cc < MAX_MIDI_CONTROL && ! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                pData->param.data[j].mappedControlIndex = cc;
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
                        CARLA_SAFE_ASSERT_INT2(fLatencyIndex == static_cast<int32_t>(j), fLatencyIndex, j);
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
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

                // check for scalepoints, require at least 2 to make it useful
                if (hasPortRDF && fRdfDescriptor->Ports[i].ScalePointCount >= 2)
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

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
                    } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (parameter)");
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
                    } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (null)");
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
           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
           #endif
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

       #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
        if (fUiFilename != nullptr)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
       #endif

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
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

        // check initial latency
        findInitialLatencyValue(aIns, aOuts);

        fForcedStereoIn  = forcedStereoIn;
        fForcedStereoOut = forcedStereoOut;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginLADSPADSSI::reload() - end");
    }

    void findInitialLatencyValue(const uint32_t aIns, const uint32_t aOuts) const
    {
        if (fLatencyIndex < 0 || fHandles.count() == 0)
            return;

        // we need to pre-run the plugin so it can update its latency control-port
        const LADSPA_Handle handle(fHandles.getFirst(nullptr));
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        float tmpIn[64][2];
        float tmpOut[64][2];

        for (uint32_t j=0; j < aIns; ++j)
        {
            tmpIn[j][0] = 0.0f;
            tmpIn[j][1] = 0.0f;

            try {
                fDescriptor->connect_port(handle, pData->audioIn.ports[j].rindex, tmpIn[j]);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (latency input)");
        }

        for (uint32_t j=0; j < aOuts; ++j)
        {
            tmpOut[j][0] = 0.0f;
            tmpOut[j][1] = 0.0f;

            try {
                fDescriptor->connect_port(handle, pData->audioOut.ports[j].rindex, tmpOut[j]);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (latency output)");
        }

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(handle);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI latency activate");
        }

        try {
            fDescriptor->run(handle, 2);
        } CARLA_SAFE_EXCEPTION("LADSPA/DSSI latency run");

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(handle);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI latency deactivate");
        }

        // done, let's get the value
        if (const uint32_t latency = getLatencyInFrames())
        {
            pData->client->setLatency(latency);
#ifndef BUILD_BRIDGE
            pData->latency.recreateBuffers(std::max(aIns, aOuts), latency);
#endif
        }
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginLADSPADSSI::reloadPrograms(%s)", bool2str(doInit));

        const LADSPA_Handle handle(fHandles.getFirst(nullptr));
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        const uint32_t oldCount = pData->midiprog.count;
        const int32_t  current  = pData->midiprog.current;

        // Delete old programs
        pData->midiprog.clear();

        // nothing to do for simple LADSPA plugins (do we want to bother with lrdf presets?)
        if (fDssiDescriptor == nullptr)
            return;

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

        if (doInit)
        {
            if (newCount > 0)
                setMidiProgram(0, false, false, false, true);
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
                setMidiProgram(pData->midiprog.current, true, true, true, false);

            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0, 0.0f, nullptr);
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
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI activate");
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
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI deactivate");
            }
        }
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const* const cvIn, float**,
                 const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
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
                    carla_zeroStruct(seqEvent);

                    seqEvent.type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    seqEvent.data.note.channel  = static_cast<uchar>(note.channel);
                    seqEvent.data.note.note     = note.note;
                    seqEvent.data.note.velocity = note.velo;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif
            const bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn, frames, isSampleAccurate, pData->event.portIn);
#endif

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                EngineEvent& event(pData->event.portIn->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }

                if (isSampleAccurate && eventTime > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, eventTime - timeOffset, timeOffset, midiEventCount))
                    {
                        startTime  = 0;
                        timeOffset = eventTime;
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
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

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

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct(seqEvent);

                            seqEvent.time.tick = isSampleAccurate ? startTime : eventTime;

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = ctrlEvent.param;
                            seqEvent.data.control.value   = int8_t(ctrlEvent.normalizedValue*127.0f + 0.5f);
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
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
                                    setMidiProgramRT(k, true);
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
                            carla_zeroStruct(seqEvent);

                            seqEvent.time.tick = isSampleAccurate ? startTime : eventTime;

                            seqEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            seqEvent.data.control.channel = event.channel;
                            seqEvent.data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& seqEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct(seqEvent);

                            seqEvent.time.tick = isSampleAccurate ? startTime : eventTime;

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
                    carla_zeroStruct(seqEvent);

                    seqEvent.time.tick = isSampleAccurate ? startTime : eventTime;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF:
                        if ((pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES) == 0x0)
                        {
                            const uint8_t note = midiEvent.data[1];

                            seqEvent.type = SND_SEQ_EVENT_NOTEOFF;
                            seqEvent.data.note.channel = event.channel;
                            seqEvent.data.note.note    = note;

                            pData->postponeNoteOffRtEvent(true, event.channel, note);
                        }
                        break;

                    case MIDI_STATUS_NOTE_ON:
                        if ((pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES) == 0x0)
                        {
                            const uint8_t note = midiEvent.data[1];
                            const uint8_t velo = midiEvent.data[2];

                            seqEvent.type = SND_SEQ_EVENT_NOTEON;
                            seqEvent.data.note.channel  = event.channel;
                            seqEvent.data.note.note     = note;
                            seqEvent.data.note.velocity = velo;

                            pData->postponeNoteOnRtEvent(true, event.channel, note, velo);
                        }
                        break;

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

                if (pData->param.data[k].mappedControlIndex > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].mappedControlIndex);
                    value   = pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter,
                                                            param, -1, value);
                }
            }
        } // End of Control Output

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return;

        // unused
        (void)cvIn;
#endif
    }

    bool processSingle(const float* const* const audioIn, float** const audioOut, const uint32_t frames,
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

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    audioOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        const bool customMonoOut   = pData->audioOut.count == 2 && fForcedStereoOut && ! fForcedStereoIn;
        const bool customStereoOut = pData->audioOut.count == 2 && fForcedStereoIn  && ! fForcedStereoOut;

        if (! customMonoOut)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(fAudioOutBuffers[i], frames);
        }

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            carla_copyFloats(fAudioInBuffers[i], audioIn[i]+timeOffset, frames);

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
                carla_zeroFloats(fAudioOutBuffers[instn], frames);

            // ----------------------------------------------------------------------------------------------------
            // Run it

            if (fDssiDescriptor != nullptr && fDssiDescriptor->run_synth != nullptr)
            {
                try {
                    fDssiDescriptor->run_synth(handle, frames, fMidiEvents, midiEventCount);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI run_synth");
            }
            else
            {
                try {
                    fDescriptor->run(handle, frames);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI run");
            }

            // ----------------------------------------------------------------------------------------------------
            // Mixdown for forced stereo

            if (customMonoOut)
                carla_multiply(fAudioOutBuffers[instn], 0.5f, frames);
            else if (customStereoOut)
                carla_copyFloats(fExtraStereoBuffer[instn], fAudioOutBuffers[instn], frames);
        }

        if (customStereoOut)
        {
            carla_copyFloats(fAudioOutBuffers[0], fExtraStereoBuffer[0], frames);
            carla_copyFloats(fAudioOutBuffers[1], fExtraStereoBuffer[1], frames);
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue;
            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
# ifndef BUILD_BRIDGE
                        if (k < pData->latency.frames && pData->latency.buffers != nullptr)
                            bufValue = pData->latency.buffers[c][k];
                        else if (pData->latency.frames < frames)
                            bufValue = fAudioInBuffers[c][k-pData->latency.frames];
                        else
# endif
                            bufValue = fAudioInBuffers[c][k];

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
                        carla_copyFloats(oldBufLeft, fAudioOutBuffers[i], frames);
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

# ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Save latency values for next callback

        if (pData->latency.frames != 0 && pData->latency.buffers != nullptr)
        {
            CARLA_SAFE_ASSERT(timeOffset == 0);
            const uint32_t latframes = pData->latency.frames;

            if (latframes <= frames)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    carla_copyFloats(pData->latency.buffers[i], audioIn[i]+(frames-latframes), latframes);
            }
            else
            {
                const uint32_t diff = latframes - frames;

                for (uint32_t i=0, k; i<pData->audioIn.count; ++i)
                {
                    // push back buffer by 'frames'
                    for (k=0; k < diff; ++k)
                        pData->latency.buffers[i][k] = pData->latency.buffers[i][k+frames];

                    // put current input at the end
                    for (uint32_t j=0; k < latframes; ++j, ++k)
                        pData->latency.buffers[i][k] = audioIn[i][j];
                }
            }
        }
# endif
#else // BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginLADSPADSSI::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];

            fAudioInBuffers[i] = new float[newBufferSize];
            carla_zeroFloats(fAudioInBuffers[i], newBufferSize);
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];

            fAudioOutBuffers[i] = new float[newBufferSize];
            carla_zeroFloats(fAudioOutBuffers[i], newBufferSize);
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
            carla_zeroFloats(fExtraStereoBuffer[0], newBufferSize);
            carla_zeroFloats(fExtraStereoBuffer[1], newBufferSize);
        }

        reconnectAudioPorts();

        carla_debug("CarlaPluginLADSPADSSI::bufferSizeChanged(%i) - end", newBufferSize);

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginLADSPADSSI::sampleRateChanged(%g) - start", newSampleRate);

        // TODO - handle UI stuff

        if (pData->active)
            deactivate();

        const std::size_t instanceCount(fHandles.count());

        if (fDescriptor->cleanup != nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->cleanup(handle);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI cleanup");
            }
        }

        fHandles.clear();

        for (std::size_t i=0; i<instanceCount; ++i)
            addInstance();

        reconnectAudioPorts();

        if (pData->active)
            activate();

        carla_debug("CarlaPluginLADSPADSSI::sampleRateChanged(%g) - end", newSampleRate);
    }

    void reconnectAudioPorts() const noexcept
    {
        if (fForcedStereoIn)
        {
            if (LADSPA_Handle const handle = fHandles.getFirst(nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (forced stereo input, first)");
            }

            if (LADSPA_Handle const handle = fHandles.getLast(nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (forced stereo input, last)");
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
                    } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (audio input)");
                }
            }
        }

        if (fForcedStereoOut)
        {
            if (LADSPA_Handle const handle = fHandles.getFirst(nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (forced stereo output, first)");
            }

            if (LADSPA_Handle const handle = fHandles.getLast(nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
                } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (forced stereo output, last)");
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
                    } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port (audio output)");
                }
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginLADSPADSSI::clearBuffers() - start");

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

        carla_debug("CarlaPluginLADSPADSSI::clearBuffers() - end");
    }

   #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
    // -------------------------------------------------------------------
    // OSC stuff

    void handleOscMessage(const char* const method, const int argc, const void* const argvx, const char* const types, void* const msg) override
    {
        const lo_address source = lo_message_get_source(static_cast<lo_message>(msg));
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

        // protocol for DSSI UIs *must* be UDP
        CARLA_SAFE_ASSERT_RETURN(lo_address_get_protocol(source) == LO_UDP,);

        if (fOscData.source == nullptr)
        {
            // if no UI is registered yet only "configure" and "update" messages are valid
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(method, "configure") == 0 || std::strcmp(method, "update") == 0,)
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

        const lo_arg* const* const argv(static_cast<const lo_arg* const*>(argvx));

        if (std::strcmp(method, "configure") == 0)
            return handleOscMessageConfigure(argc, argv, types);
        if (std::strcmp(method, "control") == 0)
            return handleOscMessageControl(argc, argv, types);
        if (std::strcmp(method, "program") == 0)
            return handleOscMessageProgram(argc, argv, types);
        if (std::strcmp(method, "midi") == 0)
            return handleOscMessageMIDI(argc, argv, types);
        if (std::strcmp(method, "update") == 0)
            return handleOscMessageUpdate(argc, argv, types, source);
        if (std::strcmp(method, "exiting") == 0)
            return handleOscMessageExiting();

        carla_stdout("CarlaPluginLADSPADSSI::handleOscMessage() - unknown method '%s'", method);
    }

    void handleOscMessageConfigure(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginLADSPADSSI::handleMsgConfigure()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "ss");

        const char* const key   = (const char*)&argv[0]->s;
        const char* const value = (const char*)&argv[1]->s;

        setCustomData(CUSTOM_DATA_TYPE_STRING, key, value, false);
    }

    void handleOscMessageControl(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginLADSPADSSI::handleMsgControl()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "if");

        const int32_t rindex = argv[0]->i;
        const float   value  = argv[1]->f;

        setParameterValueByRealIndex(rindex, value, false, true, true);
    }

    void handleOscMessageProgram(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginLADSPADSSI::handleMsgProgram()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(2, "ii");

        const int32_t bank    = argv[0]->i;
        const int32_t program = argv[1]->i;

        CARLA_SAFE_ASSERT_RETURN(bank >= 0,);
        CARLA_SAFE_ASSERT_RETURN(program >= 0,);

        setMidiProgramById(static_cast<uint32_t>(bank), static_cast<uint32_t>(program), false, true, true);
    }

    void handleOscMessageMIDI(const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("CarlaPluginLADSPADSSI::handleMsgMidi()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(1, "m");

        if (getMidiInCount() == 0)
        {
            carla_stderr("CarlaPluginLADSPADSSI::handleMsgMidi() - received midi when plugin has no midi inputs");
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
        carla_debug("CarlaPluginLADSPADSSI::handleMsgUpdate()");
        CARLA_PLUGIN_DSSI_OSC_CHECK_OSC_TYPES(1, "s");

        const char* const url = (const char*)&argv[0]->s;

        // FIXME - remove debug prints later
        carla_stdout("CarlaPluginLADSPADSSI::updateOscData(%p, \"%s\")", source, url);

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
            osc_send_program(fOscData, curMidiProg.bank, curMidiProg.program);
        }

        for (uint32_t i=0; i < pData->param.count; ++i)
            osc_send_control(fOscData, pData->param.data[i].rindex, getParameterValue(i));

       #ifndef BUILD_BRIDGE
        if (pData->engine->getOptions().frontendWinId != 0)
            pData->transientTryCounter = 1;
       #endif

        carla_stdout("CarlaPluginLADSPADSSI::updateOscData() - done");
    }

    void handleOscMessageExiting()
    {
        carla_debug("CarlaPluginLADSPADSSI::handleMsgExiting()");

        // hide UI
        showCustomUI(false);

        // tell frontend
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
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
   #endif // CARLA_ENABLE_DSSI_PLUGIN_GUI

    // -------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fDssiDescriptor != nullptr
               ? (const void*)fDssiDescriptor
               : (const void*)fDescriptor;
    }

    const void* getExtraStuff() const noexcept override
    {
        if (fDssiDescriptor != nullptr)
        {
           #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
            return fUiFilename;
           #else
            return nullptr;
           #endif
        }
        return fRdfDescriptor;
    }

   #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fThreadUI.getProcessId();
    }
   #endif

    // -------------------------------------------------------------------

    bool initLADSPA(const CarlaPluginPtr plugin,
                    const char* const filename, const char* name, const char* const label, const uint options,
                    const LADSPA_RDF_Descriptor* const rdfDescriptor)
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

        // ---------------------------------------------------------------
        // open DLL

        if (! pData->libOpen(filename))
        {
            pData->engine->setLastError(pData->libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const LADSPA_Descriptor_Function descFn = pData->libSymbol<LADSPA_Descriptor_Function>("ladspa_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        // if label is null, get first valid plugin
        const bool nullLabel = (label == nullptr || label[0] == '\0');

        for (ulong d=0;; ++d)
        {
            try {
                fDescriptor = descFn(d);
            }
            catch(...) {
                carla_stderr2("Caught exception when trying to get LADSPA descriptor");
                fDescriptor = nullptr;
                break;
            }

            if (fDescriptor == nullptr)
                break;

            if (fDescriptor->Label == nullptr || fDescriptor->Label[0] == '\0')
            {
                carla_stderr2("WARNING - Got an invalid label, will not use this plugin");
                fDescriptor = nullptr;
                break;
            }
            if (fDescriptor->run == nullptr)
            {
                carla_stderr2("WARNING - Plugin has no run, cannot use it");
                fDescriptor = nullptr;
                break;
            }

            if (nullLabel || std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr)
        {
            pData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        return init2(plugin, filename, name, options, rdfDescriptor);
    }

    bool initDSSI(const CarlaPluginPtr plugin,
                  const char* const filename, const char* name, const char* const label, const uint options)
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

        // if label is null, get first valid plugin
        const bool nullLabel = (label == nullptr || label[0] == '\0');

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

            if (nullLabel || std::strcmp(fDescriptor->Label, label) == 0)
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

        return init2(plugin, filename, name, options, nullptr);
    }

    bool init2(const CarlaPluginPtr plugin,
               const char* const filename, const char* name, const uint options,
               const LADSPA_RDF_Descriptor* const rdfDescriptor)
    {
        // ---------------------------------------------------------------
        // check for fixed buffer size requirement

        fNeedsFixedBuffers = String(filename).contains("dssi-vst", true);

        if (fNeedsFixedBuffers && ! pData->engine->usesConstantBufferSize())
        {
            pData->engine->setLastError("Cannot use this plugin under the current engine.\n"
                                        "The plugin requires a fixed block size which is not possible right now.");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (is_ladspa_rdf_descriptor_valid(rdfDescriptor, fDescriptor))
            fRdfDescriptor = ladspa_rdf_dup(rdfDescriptor);

        if (name == nullptr || name[0] == '\0')
        {
            /**/ if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr && fRdfDescriptor->Title[0] != '\0')
                name = fRdfDescriptor->Title;
            else if (fDescriptor->Name != nullptr && fDescriptor->Name[0] != '\0')
                name = fDescriptor->Name;
            else
                name = fDescriptor->Label;
        }

        pData->name = pData->engine->getUniquePluginName(name);
        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

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

        if (fDssiDescriptor != nullptr && fDssiDescriptor->configure != nullptr)
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
        // get engine options

        const EngineOptions& opts(pData->engine->getOptions());

       #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
        // ---------------------------------------------------------------
        // check for gui

        if (opts.oscEnabled && opts.oscPortUDP >= 0)
        {
            if (const char* const guiFilename = find_dssi_ui(filename, fDescriptor->Label))
            {
                fUiFilename = guiFilename;

                String uiTitle;

                if (pData->uiTitle.isNotEmpty())
                {
                    uiTitle = pData->uiTitle;
                }
                else
                {
                    uiTitle  = pData->name;
                    uiTitle += " (GUI)";
                }

                fThreadUI.setData(guiFilename, fDescriptor->Label, uiTitle);
            }
        }
       #endif

        // ---------------------------------------------------------------
        // set options

        pData->options = 0x0;

        /**/ if (fLatencyIndex >= 0 || fNeedsFixedBuffers)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        else if (options & PLUGIN_OPTION_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        /**/ if (opts.forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else if (options & PLUGIN_OPTION_FORCE_STEREO)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fDssiDescriptor != nullptr)
        {
            if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
                    pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (fUsesCustomData)
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
                    pData->options |= PLUGIN_OPTION_USE_CHUNKS;

            if (fDssiDescriptor->run_synth != nullptr)
            {
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
                    pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
                    pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
                    pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
                    pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
                    pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
                if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                    pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
            }
        }

        return true;
    }

    // -------------------------------------------------------------------

private:
    LinkedList<LADSPA_Handle>    fHandles;
    const LADSPA_Descriptor*     fDescriptor;
    const DSSI_Descriptor*       fDssiDescriptor;
    const LADSPA_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fExtraStereoBuffer[2]; // used only if forcedStereoIn and audioOut == 2
    float*  fParamBuffers;

    snd_seq_event_t fMidiEvents[kPluginMaxMidiEvents];

    int32_t fLatencyIndex; // -1 if invalid
    bool    fForcedStereoIn;
    bool    fForcedStereoOut;
    bool    fNeedsFixedBuffers;
    bool    fUsesCustomData;

   #ifdef CARLA_ENABLE_DSSI_PLUGIN_GUI
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
        } CARLA_SAFE_EXCEPTION_RETURN_ERR("LADSPA/DSSI instantiate", "Plugin failed to initialize");

        for (uint32_t i=0, count=pData->param.count; i<count; ++i)
        {
            const int32_t rindex(pData->param.data[i].rindex);
            CARLA_SAFE_ASSERT_CONTINUE(rindex >= 0);

            try {
                fDescriptor->connect_port(handle, static_cast<ulong>(rindex), &fParamBuffers[i]);
            } CARLA_SAFE_EXCEPTION("LADSPA/DSSI connect_port");
        }

        if (fHandles.append(handle))
            return true;

        try {
            fDescriptor->cleanup(handle);
        } CARLA_SAFE_EXCEPTION("LADSPA/DSSI cleanup");

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
        const char* const sepBracketStart = std::strstr(paramName, useBracket ? " [" : " (");

        if (sepBracketStart == nullptr)
            return false;

        const char* const sepBracketEnd = std::strstr(sepBracketStart, useBracket ? "]" : ")");

        if (sepBracketEnd == nullptr)
            return false;

        const std::size_t unitSize = static_cast<std::size_t>(sepBracketEnd-sepBracketStart-2);

        if (unitSize > 7) // very unlikely to have such big unit
            return false;

        const std::size_t sepIndex = std::strlen(paramName)-unitSize-3;

        // just in case
        if (sepIndex > STR_MAX-3)
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

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginLADSPADSSI)
};

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* const rdfDescriptor)
{
    carla_debug("CarlaPlugin::newLADSPA({%p, \"%s\", \"%s\", \"%s\", " P_INT64 ", %x}, %p)",
                init.engine, init.filename, init.name, init.label, init.uniqueId, init.options, rdfDescriptor);

    std::shared_ptr<CarlaPluginLADSPADSSI> plugin(new CarlaPluginLADSPADSSI(init.engine, init.id));

    if (! plugin->initLADSPA(plugin, init.filename, init.name, init.label, init.options, rdfDescriptor))
        return nullptr;

    return plugin;
}

CarlaPluginPtr CarlaPlugin::newDSSI(const Initializer& init)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\", " P_INT64 ", %x})",
                init.engine, init.filename, init.name, init.label, init.uniqueId, init.options);

    std::shared_ptr<CarlaPluginLADSPADSSI> plugin(new CarlaPluginLADSPADSSI(init.engine, init.id));

    if (! plugin->initDSSI(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
