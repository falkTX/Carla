// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#if defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC)

#include "CarlaLibJackHints.h"
#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaShmUtils.hpp"
#include "CarlaThread.hpp"

#ifdef HAVE_LIBLO
# include "CarlaOscUtils.hpp"
#else
# warning No liblo support, NSM (session state) will not be available
#endif

#include "extra/ScopedPointer.hpp"
#include "water/files/File.h"
#include "water/misc/Time.h"
#include "water/text/StringArray.h"
#include "water/threads/ChildProcess.h"

#include "jackbridge/JackBridge.hpp"

#include <ctime>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------

using water::ChildProcess;
using water::File;

CARLA_BACKEND_START_NAMESPACE

static size_t safe_rand(const size_t limit)
{
    const int r = std::rand();
    CARLA_SAFE_ASSERT_RETURN(r >= 0, 0);

    return static_cast<uint>(r) % limit;
}

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// -------------------------------------------------------------------------------------------------------------------

struct Announcer {
    virtual ~Announcer() {}
    virtual void nsmAnnounced(bool hasGui) = 0;
};

class CarlaPluginJackThread : public CarlaThread
{
public:
    CarlaPluginJackThread(Announcer* const ann, CarlaEngine* const engine, CarlaPlugin* const plugin) noexcept
        : CarlaThread("CarlaPluginJackThread"),
          kAnnouncer(ann),
          kEngine(engine),
          kPlugin(plugin),
          fShmIds(),
          fSetupLabel(),
#ifdef HAVE_LIBLO
          fOscClientAddress(nullptr),
          fOscServer(nullptr),
          fHasOptionalGui(false),
          fProject(),
#endif
          fProcess() {}

    void setData(const char* const shmIds, const char* const setupLabel) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && shmIds[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(setupLabel != nullptr && setupLabel[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fShmIds     = shmIds;
        fSetupLabel = setupLabel;
    }

    uintptr_t getProcessID() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

        return (uintptr_t)fProcess->getPID();
    }

#ifdef HAVE_LIBLO
    void nsmSave(const char* const setupLabel)
    {
        if (fOscClientAddress == nullptr)
            return;

        if (fSetupLabel != setupLabel)
            fSetupLabel = setupLabel;

        maybeOpenFirstTime(false);

        lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/nsm/client/save", "");
    }

    bool nsmShowGui(const bool yesNo)
    {
        if (fOscClientAddress == nullptr || ! fHasOptionalGui)
            return false;

        lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE,
                     yesNo ? "/nsm/client/show_optional_gui"
                           : "/nsm/client/hide_optional_gui", "");

        return true;
    }
#endif

    char* getEnvVarsToExport()
    {
        const EngineOptions& options(kEngine->getOptions());
        String binaryDir(options.binaryDir);
       #ifdef HAVE_LIBLO
        const int sessionManager = fSetupLabel[4U] - '0';
       #endif

        String ret;
       #ifdef CARLA_OS_MAC
        ret += "export DYLD_LIBRARY_PATH=" + binaryDir + "/jack\n";
        ret += "export DYLD_INSERT_LIBRARIES=" + binaryDir + "/libcarla_interposer-jack-x11.dylib\n";
        ret += "export DYLD_FORCE_FLAT_NAMESPACE=1\n";
       #else
        ret += "export LD_LIBRARY_PATH=" + binaryDir + "/jack\n";
        ret += "export LD_PRELOAD=" + binaryDir + "/libcarla_interposer-jack-x11.so\n";
       #endif
       #ifdef HAVE_LIBLO
        if (sessionManager == LIBJACK_SESSION_MANAGER_NSM)
        {
            for (int i=50; fOscServer == nullptr && --i>=0;)
                d_msleep(100);

            ret += "export NSM_URL=";
            ret += lo_server_get_url(fOscServer);
            ret += "\n";
        }
       #endif

        if (kPlugin->getHints() & PLUGIN_HAS_CUSTOM_UI)
            ret += "export CARLA_FRONTEND_WIN_ID=" + String(options.frontendWinId) + "\n";

        ret += "export CARLA_LIBJACK_SETUP=" + fSetupLabel + "\n";
        ret += "export CARLA_SHM_IDS=" + fShmIds + "\n";

        return ret.getAndReleaseBuffer();
    }

protected:
#ifdef HAVE_LIBLO
    static void _osc_error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr2("CarlaPluginJackThread::_osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int _broadcast_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, 0);
        carla_stdout("CarlaPluginJackThread::_broadcast_handler(%s, %s, %p, %i)", path, types, argv, argc);

        return ((CarlaPluginJackThread*)data)->handleBroadcast(path, types, argv, msg);
    }

    void maybeOpenFirstTime(const bool announced)
    {
        if (fSetupLabel.length() <= 6)
            return;

        if ((announced || fProject.path.isEmpty()) && fProject.init(kPlugin->getName(),
                                                                    kEngine->getCurrentProjectFolder(),
                                                                    &fSetupLabel[6U]))
        {
            carla_stdout("Sending open signal %s %s %s",
                         fProject.path.buffer(), fProject.display.buffer(), fProject.clientName.buffer());

            lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/nsm/client/open", "sss",
                         fProject.path.buffer(), fProject.display.buffer(), fProject.clientName.buffer());
        }
    }

    int handleBroadcast(const char* path, const char* types, lo_arg** argv, lo_message msg)
    {
        if (std::strcmp(path, "/nsm/server/announce") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "sssiii") == 0, 0);

            const lo_address msgAddress(lo_message_get_source(msg));
            CARLA_SAFE_ASSERT_RETURN(msgAddress != nullptr, 0);

            char* const msgURL(lo_address_get_url(msgAddress));
            CARLA_SAFE_ASSERT_RETURN(msgURL != nullptr, 0);

            if (fOscClientAddress != nullptr)
                lo_address_free(fOscClientAddress);

            fOscClientAddress = lo_address_new_from_url(msgURL);
            CARLA_SAFE_ASSERT_RETURN(fOscClientAddress != nullptr, 0);

            fProject.appName = &argv[0]->s;
            fHasOptionalGui  = std::strstr(&argv[1]->s, ":optional-gui:") != nullptr;

            kAnnouncer->nsmAnnounced(fHasOptionalGui);

            static const char* const featuresG = ":server-control:optional-gui:";
            static const char* const featuresN = ":server-control:";

            static const char* const method  = "/nsm/server/announce";
            static const char* const message = "Howdy, what took you so long?";
            static const char* const smName  = "Carla";

            const char* const features = ((fSetupLabel[5U] - '0') & LIBJACK_FLAG_CONTROL_WINDOW)
                                       ? featuresG : featuresN;

            lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/reply", "ssss",
                         method, message, smName, features);

            maybeOpenFirstTime(true);
            return 0;
        }

        CARLA_SAFE_ASSERT_RETURN(fOscClientAddress != nullptr, 0);

        if (std::strcmp(path, "/reply") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "ss") == 0, 0);

            const char* const method  = &argv[0]->s;
            const char* const message = &argv[1]->s;

            carla_stdout("Got reply of '%s' as '%s'", method, message);

            if (std::strcmp(method, "/nsm/client/open") == 0)
            {
                carla_stdout("Sending 'Session is loaded' to %s", fProject.appName.buffer());
                lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/nsm/client/session_is_loaded", "");
            }
        }

        else if (std::strcmp(path, "/nsm/client/gui_is_shown") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "") == 0, 0);

            kEngine->callback(true, true,
                              ENGINE_CALLBACK_UI_STATE_CHANGED,
                              kPlugin->getId(),
                              1,
                              0, 0, 0.0f, nullptr);
        }

        else if (std::strcmp(path, "/nsm/client/gui_is_hidden") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "") == 0, 0);

            kEngine->callback(true, true,
                              ENGINE_CALLBACK_UI_STATE_CHANGED,
                              kPlugin->getId(),
                              0,
                              0, 0, 0.0f, nullptr);
        }

        // special messages
        else if (std::strcmp(path, "/nsm/gui/client/save") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "s") == 0, 0);

            lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/nsm/client/save", "");
        }

        else if (std::strcmp(path, "/nsm/server/stop") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "s") == 0, 0);

            lo_send_from(fOscClientAddress, fOscServer, LO_TT_IMMEDIATE, "/nsm/client/hide_optional_gui", "");

            kEngine->callback(true, true,
                              ENGINE_CALLBACK_UI_STATE_CHANGED,
                              kPlugin->getId(),
                              0,
                              0, 0, 0.0f, nullptr);
        }

        return 0;
    }
#endif

    void run()
    {
#ifdef HAVE_LIBLO
        if (fOscClientAddress != nullptr)
        {
            lo_address_free(fOscClientAddress);
            fOscClientAddress = nullptr;
        }

        const int sessionManager = fSetupLabel[4U] - '0';

        if (sessionManager == LIBJACK_SESSION_MANAGER_NSM)
        {
            // NSM support
            fOscServer = lo_server_new_with_proto(nullptr, LO_UDP, _osc_error_handler);
            CARLA_SAFE_ASSERT_RETURN(fOscServer != nullptr,);

            lo_server_add_method(fOscServer, nullptr, nullptr, _broadcast_handler, this);
        }
#endif

        const bool externalProcess = ((fSetupLabel[5U] - '0') & LIBJACK_FLAG_EXTERNAL_START)
                                   && ! kEngine->isLoadingProject();

        if (! externalProcess)
        {
            if (fProcess == nullptr)
            {
                fProcess = new ChildProcess();
            }
            else if (fProcess->isRunning())
            {
                carla_stderr("CarlaPluginJackThread::run() - already running");
            }

            water::String name(kPlugin->getName());
            water::String filename(kPlugin->getFilename());

            if (name.isEmpty())
                name = "(none)";

            CARLA_SAFE_ASSERT_RETURN(filename.isNotEmpty(),);

            water::StringArray arguments;

            // binary
            arguments.addTokens(filename, true);

            const EngineOptions& options(kEngine->getOptions());

            char winIdStr[STR_MAX+1];
            std::snprintf(winIdStr, STR_MAX, P_UINTPTR, options.frontendWinId);
            winIdStr[STR_MAX] = '\0';

            const String libjackdir(String(options.binaryDir) + "/jack");
           #ifdef CARLA_OS_MAC
            const String ldpreload(String(options.binaryDir) + "/libcarla_interposer-jack-x11.dylib");
           #else
            const String ldpreload(String(options.binaryDir) + "/libcarla_interposer-jack-x11.so");
           #endif

            const ScopedEngineEnvironmentLocker _seel(kEngine);

           #ifdef CARLA_OS_MAC
            const CarlaScopedEnvVar sev2("DYLD_LIBRARY_PATH", libjackdir.buffer());
            const CarlaScopedEnvVar sev1("DYLD_INSERT_LIBRARIES", ldpreload.isNotEmpty() ? ldpreload.buffer() : nullptr);
            const CarlaScopedEnvVar sev0("DYLD_FORCE_FLAT_NAMESPACE", "1");
           #else
            const CarlaScopedEnvVar sev2("LD_LIBRARY_PATH", libjackdir.buffer());
            const CarlaScopedEnvVar sev1("LD_PRELOAD", ldpreload.isNotEmpty() ? ldpreload.buffer() : nullptr);
           #endif
           #ifdef HAVE_LIBLO
            const CarlaScopedEnvVar sev3("NSM_URL", lo_server_get_url(fOscServer));
           #endif

            if (kPlugin->getHints() & PLUGIN_HAS_CUSTOM_UI)
                carla_setenv("CARLA_FRONTEND_WIN_ID", winIdStr);
            else
                carla_unsetenv("CARLA_FRONTEND_WIN_ID");

            carla_setenv("CARLA_LIBJACK_SETUP", fSetupLabel.buffer());
            carla_setenv("CARLA_SHM_IDS", fShmIds.buffer());

            if (! fProcess->start(arguments))
            {
                carla_stdout("failed!");
                fProcess = nullptr;
                return;
            }
        }

        for (; (externalProcess || fProcess->isRunning()) && ! shouldThreadExit();)
        {
#ifdef HAVE_LIBLO
            if (sessionManager == LIBJACK_SESSION_MANAGER_NSM)
            {
                lo_server_recv_noblock(fOscServer, 50);
            }
            else
#endif
            {
                d_msleep(50);
            }
        }

#ifdef HAVE_LIBLO
        if (sessionManager == LIBJACK_SESSION_MANAGER_NSM)
        {
            lo_server_free(fOscServer);
            fOscServer = nullptr;

            if (fOscClientAddress != nullptr)
            {
                lo_address_free(fOscClientAddress);
                fOscClientAddress = nullptr;
            }
        }
#endif

        if (! externalProcess)
        {
            // we only get here if bridge crashed or thread asked to exit
            if (fProcess->isRunning() && shouldThreadExit())
            {
                fProcess->waitForProcessToFinish(2000);

                if (fProcess->isRunning())
                {
                    carla_stdout("CarlaPluginJackThread::run() - application refused to close, force kill now");
                    fProcess->kill();
                }
            }
            else
            {
                // forced quit, may have crashed
                if (fProcess->getExitCodeAndClearPID() != 0)
                {
                    carla_stderr("CarlaPluginJackThread::run() - application crashed");

                    String errorString("Plugin '" + String(kPlugin->getName()) + "' has crashed!\n"
                                            "Saving now will lose its current settings.\n"
                                            "Please remove this plugin, and not rely on it from this point.");
                    kEngine->callback(true, true,
                                    ENGINE_CALLBACK_ERROR,
                                    kPlugin->getId(),
                                    0, 0, 0, 0.0f,
                                    errorString);
                }
            }
        }

        fProcess = nullptr;
    }

private:
    Announcer* const kAnnouncer;
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    String fShmIds;
    String fSetupLabel;

#ifdef HAVE_LIBLO
    lo_address fOscClientAddress;
    lo_server  fOscServer;
    bool fHasOptionalGui;

    struct ProjectData {
        String appName;
        String path;
        String display;
        String clientName;

        ProjectData()
            : appName(),
              path(),
              display(),
              clientName() {}

        bool init(const char* const pluginName,
                  const char* const engineProjectFolder,
                  const char* const uniqueCodeID)
        {
            CARLA_SAFE_ASSERT_RETURN(engineProjectFolder != nullptr && engineProjectFolder[0] != '\0', false);
            CARLA_SAFE_ASSERT_RETURN(uniqueCodeID != nullptr && uniqueCodeID[0] != '\0', false);
            CARLA_SAFE_ASSERT_RETURN(appName.isNotEmpty(), false);

            String child(pluginName);
            child += ".";
            child += uniqueCodeID;

            const File file(File(engineProjectFolder).getChildFile(child));

            clientName = appName + "." + uniqueCodeID;
            path = file.getFullPathName().toRawUTF8();
            display = file.getFileNameWithoutExtension().toRawUTF8();

            return true;
        }

        CARLA_DECLARE_NON_COPYABLE(ProjectData)
    } fProject;
#endif

    ScopedPointer<ChildProcess> fProcess;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginJackThread)
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginJack : public CarlaPlugin,
                        public Announcer
{
public:
    CarlaPluginJack(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInitiated(false),
          fInitError(false),
          fTimedOut(false),
          fTimedError(false),
          fProcCanceled(false),
          fBufferSize(engine->getBufferSize()),
          fProcWaitTime(0),
          fSetupHints(0x0),
          fBridgeThread(this, engine, this),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fInfo()
    {
        carla_debug("CarlaPluginJack::CarlaPluginJack(%p, %i)", engine, id);

        pData->hints |= PLUGIN_IS_BRIDGE;
    }

    ~CarlaPluginJack() override
    {
        carla_debug("CarlaPluginJack::~CarlaPluginJack()");

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->transientTryCounter = 0;
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

        if (fBridgeThread.isThreadRunning())
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientQuit);
            fShmRtClientControl.commitWrite();

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientQuit);
            fShmNonRtClientControl.commitWrite();

            if (! fTimedOut)
                waitForClient("stopping", 3000);
        }

        fBridgeThread.stopThread(3000);

        fShmNonRtServerControl.clear();
        fShmNonRtClientControl.clear();
        fShmRtClientControl.clear();
        fShmAudioPool.clear();

        clearBuffers();

        fInfo.chunk.clear();
    }

    // -------------------------------------------------------------------

    void nsmAnnounced(bool hasGui) override
    {
        if (hasGui || (pData->hints & PLUGIN_HAS_CUSTOM_UI) == 0x0)
            return;

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientShowUI);
            fShmNonRtClientControl.commitWrite();
        }

        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                1,
                                0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_JACK;
    }

    PluginCategory getCategory() const noexcept override
    {
        return PLUGIN_CATEGORY_NONE;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return fInfo.mIns;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return fInfo.mOuts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        if (fInfo.mIns > 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
            options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        return options;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.setupLabel, STR_MAX);
        return true;
    }

    bool getMaker(char* const) const noexcept override
    {
        return false;
    }

    bool getCopyright(char* const) const noexcept override
    {
        return false;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        // FIXME
        std::strncpy(strBuf, "Carla's libjack", STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave(bool) noexcept override
    {
#ifdef HAVE_LIBLO
        if (fInfo.setupLabel.length() == 6)
            setupUniqueProjectID();
#endif

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPrepareForSave);
            fShmNonRtClientControl.commitWrite();
        }

#ifdef HAVE_LIBLO
        fBridgeThread.nsmSave(fInfo.setupLabel);
#endif
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setOption(const uint option, const bool yesNo, const bool sendCallback) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetOption);
            fShmNonRtClientControl.writeUInt(option);
            fShmNonRtClientControl.writeBool(yesNo);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setOption(option, yesNo, sendCallback);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetCtrlChannel);
            fShmNonRtClientControl.writeShort(channel);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0 && std::strcmp(key, "__CarlaPingOnOff__") == 0)
        {
#if 0
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPingOnOff);
            fShmNonRtClientControl.writeBool(std::strcmp(value, "true") == 0);
            fShmNonRtClientControl.commitWrite();
#endif
            return;
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo && ! fBridgeThread.isThreadRunning()) {
            CARLA_SAFE_ASSERT_RETURN(restartBridgeThread(),);
        }

#ifdef HAVE_LIBLO
        if (! fBridgeThread.nsmShowGui(yesNo))
#endif
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(yesNo ? kPluginBridgeNonRtClientShowUI : kPluginBridgeNonRtClientHideUI);
            fShmNonRtClientControl.commitWrite();
        }
    }

    void idle() override
    {
        if (fBridgeThread.isThreadRunning())
        {
            if (fInitiated && fTimedOut && pData->active)
                setActive(false, true, true);

            {
                const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

                fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPing);
                fShmNonRtClientControl.commitWrite();
            }

            try {
                handleNonRtData();
            } CARLA_SAFE_EXCEPTION("handleNonRtData");
        }
        else if (fInitiated)
        {
            fTimedOut   = true;
            fTimedError = true;
            fInitiated  = false;
            handleProcessStopped();
        }
        else if (fProcCanceled)
        {
            handleProcessStopped();
            fProcCanceled = false;
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_debug("CarlaPluginJack::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        // cleanup of previous data
        pData->audioIn.clear();
        pData->audioOut.clear();
        pData->event.clear();

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (fInfo.aIns > 0)
        {
            pData->audioIn.createNew(fInfo.aIns);
        }

        if (fInfo.aOuts > 0)
        {
            pData->audioOut.createNew(fInfo.aOuts);
            needsCtrlIn = true;
        }

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        String portName;

        // Audio Ins
        for (uint8_t j=0; j < fInfo.aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aIns > 1)
            {
                portName += "audio_in_";
                portName += String(j+1);
            }
            else
            {
                portName += "audio_in";
            }

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint8_t j=0; j < fInfo.aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aOuts > 1)
            {
                portName += "audio_out_";
                portName += String(j+1);
            }
            else
            {
                portName += "audio_out";
            }

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "event-in";
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

            portName += "event-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // extra plugin hints
        pData->extraHints = 0x0;

        if (fInfo.mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (fInfo.mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("CarlaPluginJack::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        if (! fBridgeThread.isThreadRunning())
        {
            CARLA_SAFE_ASSERT_RETURN(restartBridgeThread(),);
        }

        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientActivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("activate", 2000);
        } CARLA_SAFE_EXCEPTION("activate - waitForClient");
    }

    void deactivate() noexcept override
    {
        if (! fBridgeThread.isThreadRunning())
            return;

        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientDeactivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("deactivate", 2000);
        } CARLA_SAFE_EXCEPTION("deactivate - waitForClient");
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const*, float**, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fProcCanceled || fTimedOut || fTimedError || ! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            // TODO

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    uint8_t data1, data2, data3;
                    data1 = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    data2 = note.note;
                    data3 = note.velo;

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(0); // time
                    fShmRtClientControl.writeByte(0); // port
                    fShmRtClientControl.writeByte(3); // size
                    fShmRtClientControl.writeByte(data1);
                    fShmRtClientControl.writeByte(data2);
                    fShmRtClientControl.writeByte(data3);
                    fShmRtClientControl.commitWrite();
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif
            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
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

                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(0); // port
                            fShmRtClientControl.writeByte(3); // size
                            fShmRtClientControl.writeByte(uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT)));
                            fShmRtClientControl.writeByte(uint8_t(ctrlEvent.param));
                            fShmRtClientControl.writeByte(uint8_t(ctrlEvent.normalizedValue*127.0f + 0.5f));
                        }
                        break;

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiBank);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiProgram);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllSoundOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
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

                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllNotesOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size == 0 || midiEvent.size >= MAX_MIDI_VALUE)
                        continue;

                    const uint8_t* const midiData(midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data);

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiData[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(event.time);
                    fShmRtClientControl.writeByte(midiEvent.port);
                    fShmRtClientControl.writeByte(midiEvent.size);

                    fShmRtClientControl.writeByte(uint8_t(midiData[0] | (event.channel & MIDI_CHANNEL_BIT)));

                    for (uint8_t j=1; j < midiEvent.size; ++j)
                        fShmRtClientControl.writeByte(midiData[j]);

                    fShmRtClientControl.commitWrite();

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiData[1], midiData[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiData[1]);
                    }
                } break;
                }
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input

        if (! processSingle(audioIn, audioOut, frames))
            return;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            uint32_t time;
            uint8_t port, size;
            const uint8_t* midiData(fShmRtClientControl.data->midiOut);

            for (std::size_t read=0; read<kBridgeRtClientDataMidiOutSize-kBridgeBaseMidiOutHeaderSize;)
            {
                // get time
                time = *(const uint32_t*)midiData;
                midiData += 4;

                // get port and size
                port = *midiData++;
                size = *midiData++;

                if (size == 0)
                    break;

                // store midi data advancing as needed
                uint8_t data[size];

                for (uint8_t j=0; j<size; ++j)
                    data[j] = *midiData++;

                pData->event.portOut->writeMidiEvent(time, size, data);

                read += kBridgeBaseMidiOutHeaderSize + size;
            }

            // TODO
            (void)port;

        } // End of Control and MIDI Output
    }

    bool processSingle(const float* const* const audioIn, float** const audioOut, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedError, false);
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);
        CARLA_SAFE_ASSERT_RETURN(frames <= fBufferSize, false);

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
                carla_zeroFloats(audioOut[i], frames);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            carla_copyFloats(fShmAudioPool.data + (i * fBufferSize), audioIn[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

        bridgeTimeInfo.playing    = timeInfo.playing;
        bridgeTimeInfo.frame      = timeInfo.frame;
        bridgeTimeInfo.usecs      = timeInfo.usecs;
        bridgeTimeInfo.validFlags = timeInfo.bbt.valid ? kPluginBridgeTimeInfoValidBBT : 0x0;

        if (timeInfo.bbt.valid)
        {
            bridgeTimeInfo.bar  = timeInfo.bbt.bar;
            bridgeTimeInfo.beat = timeInfo.bbt.beat;
            bridgeTimeInfo.tick = timeInfo.bbt.tick;

            bridgeTimeInfo.beatsPerBar = timeInfo.bbt.beatsPerBar;
            bridgeTimeInfo.beatType    = timeInfo.bbt.beatType;

            bridgeTimeInfo.ticksPerBeat   = timeInfo.bbt.ticksPerBeat;
            bridgeTimeInfo.beatsPerMinute = timeInfo.bbt.beatsPerMinute;
            bridgeTimeInfo.barStartTick   = timeInfo.bbt.barStartTick;
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientProcess);
            fShmRtClientControl.writeUInt(frames);
            fShmRtClientControl.commitWrite();
        }

        waitForClient("process", fProcWaitTime);

        if (fTimedOut)
        {
            pData->singleMutex.unlock();
            return false;
        }

        if (fShmRtClientControl.data->procFlags)
        {
            fInitiated    = false;
            fProcCanceled = true;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            carla_copyFloats(audioOut[i], fShmAudioPool.data + ((i + fInfo.aIns) * fBufferSize), frames);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
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
                        bufValue = audioIn[c][k];
                        audioOut[i][k] = (audioOut[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, audioOut[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            audioOut[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            audioOut[i][k] += audioOut[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            audioOut[i][k]  = audioOut[i][k] * balRangeR;
                            audioOut[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing
#endif
        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        fBufferSize = newBufferSize;
        resizeAudioPool(newBufferSize);

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetBufferSize);
            fShmRtClientControl.writeUInt(newBufferSize);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = newBufferSize*1000/pData->engine->getSampleRate();
        fProcWaitTime = 1000;

        waitForClient("buffersize", 1000);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetSampleRate);
            fShmRtClientControl.writeDouble(newSampleRate);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = pData->engine->getBufferSize()*1000/newSampleRate;
        fProcWaitTime = 1000;

        waitForClient("samplerate", 1000);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetOnline);
            fShmRtClientControl.writeBool(isOffline);
            fShmRtClientControl.commitWrite();
        }

        waitForClient("offline", 1000);
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // -------------------------------------------------------------------

    void handleNonRtData()
    {
        for (; fShmNonRtServerControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtServerOpcode opcode(fShmNonRtServerControl.readOpcode());
//#ifdef DEBUG
            if (opcode != kPluginBridgeNonRtServerPong)
            {
                carla_debug("CarlaPluginJack::handleNonRtData() - got opcode: %s", PluginBridgeNonRtServerOpcode2str(opcode));
            }
//#endif
            switch (opcode)
            {
            case kPluginBridgeNonRtServerNull:
            case kPluginBridgeNonRtServerPong:
            case kPluginBridgeNonRtServerPluginInfo1:
            case kPluginBridgeNonRtServerPluginInfo2:
            case kPluginBridgeNonRtServerAudioCount:
            case kPluginBridgeNonRtServerMidiCount:
            case kPluginBridgeNonRtServerCvCount:
            case kPluginBridgeNonRtServerParameterCount:
            case kPluginBridgeNonRtServerProgramCount:
            case kPluginBridgeNonRtServerMidiProgramCount:
            case kPluginBridgeNonRtServerPortName:
            case kPluginBridgeNonRtServerParameterData1:
            case kPluginBridgeNonRtServerParameterData2:
            case kPluginBridgeNonRtServerParameterRanges:
            case kPluginBridgeNonRtServerParameterValue:
            case kPluginBridgeNonRtServerParameterValue2:
            case kPluginBridgeNonRtServerParameterTouch:
            case kPluginBridgeNonRtServerDefaultValue:
            case kPluginBridgeNonRtServerCurrentProgram:
            case kPluginBridgeNonRtServerCurrentMidiProgram:
            case kPluginBridgeNonRtServerProgramName:
            case kPluginBridgeNonRtServerMidiProgramData:
            case kPluginBridgeNonRtServerSetCustomData:
            case kPluginBridgeNonRtServerVersion:
            case kPluginBridgeNonRtServerRespEmbedUI:
            case kPluginBridgeNonRtServerResizeEmbedUI:
                break;

            case kPluginBridgeNonRtServerSetChunkDataFile:
                // uint/size, str[] (filename)
                if (const uint32_t chunkFilePathSize = fShmNonRtServerControl.readUInt())
                {
                    char chunkFilePath[chunkFilePathSize];
                    fShmNonRtServerControl.readCustomData(chunkFilePath, chunkFilePathSize);
                }
                break;

            case kPluginBridgeNonRtServerSetLatency:
            case kPluginBridgeNonRtServerSetParameterText:
                break;

            case kPluginBridgeNonRtServerReady:
                fInitiated = true;
                break;

            case kPluginBridgeNonRtServerSaved:
                break;

            case kPluginBridgeNonRtServerUiClosed:
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_UI_STATE_CHANGED,
                                        pData->id,
                                        0,
                                        0, 0, 0.0f, nullptr);
                break;

            case kPluginBridgeNonRtServerError: {
                // error
                const uint32_t errorSize(fShmNonRtServerControl.readUInt());
                char error[errorSize+1];
                carla_zeroChars(error, errorSize+1);
                fShmNonRtServerControl.readCustomData(error, errorSize);

                if (fInitiated)
                {
                    pData->engine->callback(true, true, ENGINE_CALLBACK_ERROR, pData->id, 0, 0, 0, 0.0f, error);

                    // just in case
                    pData->engine->setLastError(error);
                    fInitError = true;
                }
                else
                {
                    pData->engine->setLastError(error);
                    fInitError = true;
                    fInitiated = true;
                }
            }   break;
            }
        }
    }

    // -------------------------------------------------------------------

    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fBridgeThread.getProcessID();
    }

    // -------------------------------------------------------------------

    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const label, const uint options)
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
        // check setup

        if (std::strlen(label) < 6)
        {
            pData->engine->setLastError("invalid application setup received");
            return false;
        }

        for (int i=4; --i >= 0;) {
            CARLA_SAFE_ASSERT_INT2_RETURN(label[i] >= '0' && label[i] <= '0'+64, i, label[i], false);
        }
        CARLA_SAFE_ASSERT_INT2_RETURN(label[4] >= '0' && label[4] < '0'+0x4f, 4, label[4], false);
        CARLA_SAFE_ASSERT_UINT2_RETURN(static_cast<uchar>(label[5]) >= '0' &&
                                       static_cast<uchar>(label[5]) <= '0'+0x73,
                                       static_cast<uchar>(label[5]),
                                       static_cast<uchar>('0'+0x73),
                                       false);

        fInfo.aIns   = static_cast<uint8_t>(label[0] - '0');
        fInfo.aOuts  = static_cast<uint8_t>(label[1] - '0');
        fInfo.mIns   = static_cast<uint8_t>(carla_minPositive(label[2] - '0', 1));
        fInfo.mOuts  = static_cast<uint8_t>(carla_minPositive(label[3] - '0', 1));

        fInfo.setupLabel = label;

        // ---------------------------------------------------------------
        // set project unique id

        if (label[6] == '\0')
            setupUniqueProjectID();

        // ---------------------------------------------------------------
        // set icon

        pData->iconName = carla_strdup_safe("application");

        // ---------------------------------------------------------------
        // set info

        pData->filename = carla_strdup(filename);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName("Jack Application");

        std::srand(static_cast<uint>(std::time(nullptr)));

        // ---------------------------------------------------------------
        // init sem/shm

        if (! fShmAudioPool.initializeServer())
        {
            carla_stderr("Failed to initialize shared memory audio pool");
            return false;
        }

        if (! fShmRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize RT client control");
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT client control");
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtServerControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT server control");
            fShmNonRtClientControl.clear();
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

        // ---------------------------------------------------------------
        // setup hints and options

        fSetupHints = static_cast<uint>(static_cast<uchar>(label[5]) - '0');

        // FIXME dryWet broken
        pData->hints  = PLUGIN_IS_BRIDGE;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        pData->hints |= /*PLUGIN_CAN_DRYWET |*/ PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE;
#endif

        if (fSetupHints & LIBJACK_FLAG_CONTROL_WINDOW)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

        // ---------------------------------------------------------------
        // init bridge thread

        {
            char shmIdsStr[6*4+1];
            carla_zeroChars(shmIdsStr, 6*4+1);

            std::strncpy(shmIdsStr+6*0, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*1, &fShmRtClientControl.filename[fShmRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*2, &fShmNonRtClientControl.filename[fShmNonRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*3, &fShmNonRtServerControl.filename[fShmNonRtServerControl.filename.length()-6], 6);

            fBridgeThread.setData(shmIdsStr, fInfo.setupLabel);
        }

        if (! restartBridgeThread())
            return false;

        // ---------------------------------------------------------------
        // register client

        if (pData->name == nullptr)
            pData->name = pData->engine->getUniquePluginName("unknown");

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // remove unprintable characters if needed
        if (fSetupHints & LIBJACK_FLAG_EXTERNAL_START)
            fInfo.setupLabel[5U] = static_cast<char>('0' + (fSetupHints ^ LIBJACK_FLAG_EXTERNAL_START));

        // ---------------------------------------------------------------
        // set options

        pData->options = PLUGIN_OPTION_FIXED_BUFFERS;

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

        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
            pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;

        if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
            pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

        return true;
    }

private:
    bool fInitiated;
    bool fInitError;
    bool fTimedOut;
    bool fTimedError;
    bool fProcCanceled;
    uint fBufferSize;
    uint fProcWaitTime;
    uint fSetupHints;

    CarlaPluginJackThread fBridgeThread;

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    struct Info {
        uint8_t aIns, aOuts;
        uint8_t mIns, mOuts;
        String setupLabel;
        std::vector<uint8_t> chunk;

        Info()
            : aIns(0),
              aOuts(0),
              mIns(0),
              mOuts(0),
              setupLabel(),
              chunk() {}

        CARLA_DECLARE_NON_COPYABLE(Info)
    } fInfo;

    void handleProcessStopped() noexcept
    {
        const bool wasActive = pData->active;
        pData->active = false;

        if (wasActive)
        {
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                                    pData->id,
                                    PARAMETER_ACTIVE,
                                    0, 0,
                                    0.0f,
                                    nullptr);
        }

        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED,
                                    pData->id,
                                    0,
                                    0, 0, 0.0f, nullptr);
    }

    void resizeAudioPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, static_cast<uint32_t>(fInfo.aIns+fInfo.aOuts), 0);

        fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
        fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
        fShmRtClientControl.commitWrite();

        waitForClient("resize-pool", 5000);
    }

    void setupUniqueProjectID()
    {
        const char* const engineProjectFolder = pData->engine->getCurrentProjectFolder();
        carla_stdout("setupUniqueProjectID %s", engineProjectFolder);

        if (engineProjectFolder == nullptr || engineProjectFolder[0] == '\0')
            return;

        const File file(engineProjectFolder);
        CARLA_SAFE_ASSERT_RETURN(file.exists(),);

        char code[6];
        code[5] = '\0';

        String child;

        for (;;)
        {
            static const char* const kValidChars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789";

            static const size_t kValidCharsLen(std::strlen(kValidChars)-1U);

            code[0] = kValidChars[safe_rand(kValidCharsLen)];
            code[1] = kValidChars[safe_rand(kValidCharsLen)];
            code[2] = kValidChars[safe_rand(kValidCharsLen)];
            code[3] = kValidChars[safe_rand(kValidCharsLen)];
            code[4] = kValidChars[safe_rand(kValidCharsLen)];

            child  = pData->name;
            child += ".";
            child += code;
            const File newFile(file.getChildFile(child));

            if (newFile.existsAsFile())
                continue;

            fInfo.setupLabel += code;
            carla_stdout("new label %s", fInfo.setupLabel.buffer());
            break;
        }
    }

    bool restartBridgeThread()
    {
        fInitiated  = false;
        fInitError  = false;
        fTimedError = false;

        // reset memory
        fProcCanceled = false;
        fShmRtClientControl.data->procFlags = 0;
        carla_zeroStruct(fShmRtClientControl.data->timeInfo);
        carla_zeroBytes(fShmRtClientControl.data->midiOut, kBridgeRtClientDataMidiOutSize);

        fShmRtClientControl.clearData();
        fShmNonRtClientControl.clearData();
        fShmNonRtServerControl.clearData();

        // initial values
        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientVersion);
        fShmNonRtClientControl.writeUInt(CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT);

        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientInitialSetup);
        fShmNonRtClientControl.writeUInt(pData->engine->getBufferSize());
        fShmNonRtClientControl.writeDouble(pData->engine->getSampleRate());

        fShmNonRtClientControl.commitWrite();

        if (fShmAudioPool.dataSize != 0)
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
            fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
            fShmRtClientControl.commitWrite();
        }
        else
        {
            // testing dummy message
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientNull);
            fShmRtClientControl.commitWrite();
        }

        fBridgeThread.startThread();

        const bool needsCancelableAction = ! pData->engine->isLoadingProject();
        const bool needsEngineIdle = pData->engine->getType() != kEngineTypePlugin;

        String actionName;

        if (needsCancelableAction)
        {
            if (fSetupHints & LIBJACK_FLAG_EXTERNAL_START)
            {
                const EngineOptions& options(pData->engine->getOptions());
                String binaryDir(options.binaryDir);

                char* const hwVars = fBridgeThread.getEnvVarsToExport();

                actionName  = "Waiting for external JACK application start, please use the following environment variables:\n";
                actionName += hwVars;

                std::free(hwVars);
            }
            else
            {
                actionName = "Loading JACK application";
            }

            pData->engine->setActionCanceled(false);
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_CANCELABLE_ACTION,
                                    pData->id,
                                    1,
                                    0, 0, 0.0f,
                                    actionName.buffer());
        }

        for (;fBridgeThread.isThreadRunning();)
        {
            pData->engine->callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);

            if (needsEngineIdle)
                pData->engine->idle();

            idle();

            if (fInitiated)
                break;
            if (pData->engine->isAboutToClose() || pData->engine->wasActionCanceled())
                break;

            d_msleep(5);
        }

        if (needsCancelableAction)
        {
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_CANCELABLE_ACTION,
                                    pData->id,
                                    0,
                                    0, 0, 0.0f,
                                    actionName.buffer());
        }

        if (fInitError || ! fInitiated)
        {
            fBridgeThread.stopThread(6000);

            if (! fInitError)
                pData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n"
                                            "(or the plugin crashed on initialization?)");

            return false;
        }

        return true;
    }

    void waitForClient(const char* const action, const uint msecs)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedOut,);
        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        if (fShmRtClientControl.waitForClient(msecs))
            return;

        fTimedOut = true;
        carla_stderr2("waitForClient(%s) timed out", action);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginJack)
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OS_LINUX

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPluginPtr CarlaPlugin::newJackApp(const Initializer& init)
{
    carla_debug("CarlaPlugin::newJackApp({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

#if defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC)
    std::shared_ptr<CarlaPluginJack> plugin(new CarlaPluginJack(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
#else
    init.engine->setLastError("JACK Application support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
