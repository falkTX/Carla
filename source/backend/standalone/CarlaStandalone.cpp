/*
 * Carla Standalone
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

// TODO:
// Check carla_stderr2("Engine is not running"); <= prepend func name and args

#include "CarlaHost.h"
#include "CarlaMIDI.h"
#include "CarlaNative.h"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaOscUtils.hpp"

#include <QtCore/QByteArray>

#ifdef BUILD_BRIDGE
# undef HAVE_JUCE
#endif

#ifdef HAVE_JUCE
# include "juce_gui_basics.h"
using juce::initialiseJuce_GUI;
using juce::shutdownJuce_GUI;
using juce::MessageManager;
#endif

namespace CB = CarlaBackend;
using CB::EngineOptions;

// -------------------------------------------------------------------------------------------------------------------
// Juce Message Thread

#if defined(HAVE_JUCE) && defined(CARLA_OS_LINUX)

class JuceMessageThread : public juce::Thread
{
public:
    JuceMessageThread()
      : juce::Thread("JuceMessageThread"),
        fInitialised(false)
    {
    }

    ~JuceMessageThread() override
    {
        stop();
    }

    void start()
    {
        CARLA_SAFE_ASSERT_RETURN(! fInitialised,);

        fInitialised = false;

        startThread();

        while (! fInitialised)
            Thread::sleep(1);
    }

    void stop()
    {
        if (! fInitialised)
            return;

        stopThread(-1);
    }

protected:
    void run() override
    {
        fInitialised = true;

        if (MessageManager* const msgMgr = MessageManager::getInstance())
        {
            msgMgr->setCurrentThreadAsMessageThread();

            for (; (! threadShouldExit()) && msgMgr->runDispatchLoopUntil(250);) {}
        }

        fInitialised = false;
    }

private:
    volatile bool fInitialised;

    CARLA_DECLARE_NON_COPY_CLASS(JuceMessageThread)
};

#endif // defined(HAVE_JUCE) && defined(CARLA_OS_LINUX)

// -------------------------------------------------------------------------------------------------------------------
// Single, standalone engine

struct CarlaBackendStandalone {
    CarlaEngine*       engine;
    EngineCallbackFunc engineCallback;
    void*              engineCallbackPtr;
    EngineOptions      engineOptions;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

    CarlaString lastError;

#if defined(HAVE_JUCE) && defined(CARLA_OS_LINUX)
    JuceMessageThread juceMsgThread;
#endif

    CarlaBackendStandalone()
        : engine(nullptr),
          engineCallback(nullptr),
          engineCallbackPtr(nullptr),
          fileCallback(nullptr),
          fileCallbackPtr(nullptr)
    {
#ifdef BUILD_BRIDGE
        engineOptions.processMode         = CB::ENGINE_PROCESS_MODE_BRIDGE;
        engineOptions.transportMode       = CB::ENGINE_TRANSPORT_MODE_BRIDGE;
        engineOptions.forceStereo         = false;
        engineOptions.preferPluginBridges = false;
        engineOptions.preferUiBridges     = false;
#endif
    }

    ~CarlaBackendStandalone()
    {
        CARLA_SAFE_ASSERT(engine == nullptr);
#ifdef HAVE_JUCE
        CARLA_SAFE_ASSERT(MessageManager::getInstanceWithoutCreating() == nullptr);
#endif
    }

#ifdef HAVE_JUCE
    void init()
    {
        JUCE_AUTORELEASEPOOL
        {
            initialiseJuce_GUI();
# ifdef CARLA_OS_LINUX
            juceMsgThread.start();
# else
            if (MessageManager* const msgMgr = MessageManager::getInstance())
                msgMgr->setCurrentThreadAsMessageThread();
# endif
        }
    }

    void close()
    {
        JUCE_AUTORELEASEPOOL
        {
# ifdef CARLA_OS_LINUX
            juceMsgThread.stop();
# else
            MessageManager::deleteInstance();
# endif
            shutdownJuce_GUI();
        }
    }

# ifndef CARLA_OS_LINUX
    void idle()
    {
        JUCE_AUTORELEASEPOOL
        {
            if (MessageManager* const msgMgr = MessageManager::getInstanceWithoutCreating())
                msgMgr->runDispatchLoopUntil(5);
        }
    }
# endif
#endif

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaBackendStandalone)
};

static CarlaBackendStandalone gStandalone;

// -------------------------------------------------------------------------------------------------------------------
// NSM support

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 2

//#define NSM_CLIENT_FEATURES ":switch:optional-gui:"
#define NSM_CLIENT_FEATURES ":switch:"

class CarlaNSM
{
public:
    CarlaNSM() noexcept
        : fOscServer(nullptr),
          fHasBroadcast(false),
          fHasShowHideUI(false) {}

    ~CarlaNSM()
    {
        if (fOscServer != nullptr)
        {
            lo_server_del_method(fOscServer, "/reply", "ssss");
            lo_server_del_method(fOscServer, "/nsm/client/open", "sss");
            lo_server_del_method(fOscServer, "/nsm/client/save", "");
            //lo_server_del_method(fOscServer, "/nsm/client/show_optional_gui", "");
            //lo_server_del_method(fOscServer, "/nsm/client/hide_optional_gui", "");
            lo_server_free(fOscServer);
            fOscServer = nullptr;
        }
    }

    void announce(const int pid, const char* const initName)
    {
        const char* const NSM_URL(std::getenv("NSM_URL"));

        if (NSM_URL == nullptr)
            return;

        const lo_address addr = lo_address_new_from_url(NSM_URL);

        if (addr == nullptr)
            return;

        const int proto = lo_address_get_protocol(addr);

        if (fOscServer == nullptr)
        {
            // create new OSC server
            fOscServer = lo_server_new_with_proto(nullptr, proto, _error_handler);

            // register message handlers and start OSC thread
            lo_server_add_method(fOscServer, "/reply",           "ssss", _reply_handler, this);
            lo_server_add_method(fOscServer, "/nsm/client/open", "sss",  _open_handler,  this);
            lo_server_add_method(fOscServer, "/nsm/client/save", "",     _save_handler,  this);
            //lo_server_add_method(fOscServer, "/nsm/client/show_optional_gui", "", _show_gui_handler, this);
            //lo_server_add_method(fOscServer, "/nsm/client/hide_optional_gui", "", _hide_gui_handler, this);
            // /nsm/client/session_is_loaded
        }

#ifndef BUILD_ANSI_TEST
        lo_send_from(addr, fOscServer, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     "Carla", NSM_CLIENT_FEATURES, initName, NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);
#endif

        lo_address_free(addr);
    }

    void idle() noexcept
    {
        if (fOscServer == nullptr)
            return;

        for (;;)
        {
            try {
                if (lo_server_recv_noblock(fOscServer, 0) == 0)
                    break;
            }
            CARLA_SAFE_EXCEPTION_CONTINUE("NSM OSC idle")
        }
    }

protected:
    int handleReply(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::handleReply(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        const char* const method   = &argv[0]->s;
        const char* const message  = &argv[1]->s;
        const char* const smName   = &argv[2]->s;
        const char* const features = &argv[3]->s;

        CARLA_SAFE_ASSERT_RETURN(std::strcmp(method, "/nsm/server/announce") == 0, 0);

        fHasBroadcast  = std::strstr(features, ":broadcast:") != nullptr;
        fHasShowHideUI = std::strstr(features, ":optional-gui:") != nullptr;

        carla_stdout("'%s' started: %s", smName, message);

        // TODO: send callback, disable open+save etc

        return 0;

#ifndef DEBUG
        // unused
        (void)path; (void)types; (void)argc;
#endif
    }

    int handleOpen(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        CARLA_SAFE_ASSERT_RETURN(fOscServer != nullptr, 0);
        carla_debug("CarlaNSM::handleOpen(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        const char* const projectPath = &argv[0]->s;
        //const char* const displayName = &argv[1]->s;
        const char* const clientId    = &argv[2]->s;

        if (! carla_is_engine_running())
        {
            gStandalone.engineOptions.processMode   = CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
            gStandalone.engineOptions.transportMode = CB::ENGINE_TRANSPORT_MODE_JACK;

            carla_engine_init("JACK", clientId);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

            for (uint i=0, count=gStandalone.engine->getCurrentPluginCount(); i < count; ++i)
                gStandalone.engine->removePlugin(i);
        }

        fClientId = clientId;

        fProjectPath  = projectPath;
        fProjectPath += ".carxp";

        CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

        gStandalone.engine->loadProject(fProjectPath);

#ifndef BUILD_ANSI_TEST
        lo_send_from(lo_message_get_source(msg), fOscServer, LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");
#endif

#if 0
        if (fHasBroadcast)
        {
            char* const url = lo_server_get_url(fOscServer);

            lo_send(lo_message_get_source(msg), "/nsm/server/broadcast", "sssss",
                            "/non/hello", url, "Carla", CARLA_VERSION_STRING, clientId);

            //lo_send_from(lo_message_get_source(msg), fOscServer, LO_TT_IMMEDIATE, "/nsm/server/broadcast", "sssss"
            //             "/non/hello", url, "Carla", CARLA_VERSION_STRING, clientId);

            lo_send(lo_message_get_source(msg), "/signal/created", "ssfff", "/path/somewhere", true ? "in" : "out", 0.0f, 1.0f, 0.5f);

            std::free(url);

            carla_stdout("Broadcast sent!");
        }
        else
            carla_stdout("Broadcast NOT NOT NOT sent!");
#endif

        return 0;

#ifndef DEBUG
        // unused
        (void)path; (void)types; (void)argc; (void)msg;
#endif
    }

    int handleSave(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        CARLA_SAFE_ASSERT_RETURN(fOscServer != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fProjectPath.isNotEmpty(), 0);
        carla_debug("CarlaNSM::handleSave(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        gStandalone.engine->saveProject(fProjectPath);

#ifndef BUILD_ANSI_TEST
        lo_send_from(lo_message_get_source(msg), fOscServer, LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/save", "OK");
#endif

        return 0;

#ifndef DEBUG
        // unused
        (void)path; (void)types; (void)argv; (void)argc; (void)msg;
#endif
    }

#if 0
    int handleShowHideGui(const lo_message msg, const bool show)
    {
        CARLA_SAFE_ASSERT_RETURN(fOscServer != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
        //CARLA_SAFE_ASSERT_RETURN(gStandalone.frontendWinId != 0, 0);
        carla_debug("CarlaNSM::handleShowHideGui(%s)", bool2str(show));

#ifdef HAVE_X11
#endif

#ifndef BUILD_ANSI_TEST
        lo_send_from(lo_message_get_source(msg), fOscServer, LO_TT_IMMEDIATE, show ? "/nsm/client/gui_is_shown" : "/nsm/client/gui_is_hidden", "");
#endif

        return 0;
    }
#endif

private:
    lo_server fOscServer;

    CarlaString fClientId;
    CarlaString fProjectPath;

    bool fHasBroadcast;
    bool fHasShowHideUI;

    #define handlePtr ((CarlaNSM*)data)

    static void _error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr2("CarlaNSM::_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int _reply_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        return handlePtr->handleReply(path, types, argv, argc, msg);
    }

    static int _open_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        return handlePtr->handleOpen(path, types, argv, argc, msg);
    }

    static int _save_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        return handlePtr->handleSave(path, types, argv, argc, msg);
    }

#if 0
    static int _show_gui_handler(const char*, const char*, lo_arg**, int, lo_message, void* data)
    {
        return handlePtr->handleShowHideGui(true);
    }

    static int _hide_gui_handler(const char*, const char*, lo_arg**, int, lo_message, void* data)
    {
        return handlePtr->handleShowHideGui(false);
    }
#endif

    #undef handlePtr

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaNSM)
};

static CarlaNSM gNSM;

// -------------------------------------------------------------------------------------------------------------------
// Always return a valid string ptr

static const char* const gNullCharPtr = "";

static void checkStringPtr(const char*& charPtr)
{
    if (charPtr == nullptr)
        charPtr = gNullCharPtr;
}

// -------------------------------------------------------------------------------------------------------------------
// Constructors

_CarlaPluginInfo::_CarlaPluginInfo() noexcept
    : type(CB::PLUGIN_NONE),
      category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0),
      optionsAvailable(0x0),
      optionsEnabled(0x0),
      filename(gNullCharPtr),
      name(gNullCharPtr),
      label(gNullCharPtr),
      maker(gNullCharPtr),
      copyright(gNullCharPtr),
      iconName(gNullCharPtr),
      uniqueId(0) {}

_CarlaPluginInfo::~_CarlaPluginInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
    if (maker != gNullCharPtr)
        delete[] maker;
    if (copyright != gNullCharPtr)
        delete[] copyright;
}

_CarlaNativePluginInfo::_CarlaNativePluginInfo() noexcept
    : category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0),
      audioIns(0),
      audioOuts(0),
      midiIns(0),
      midiOuts(0),
      parameterIns(0),
      parameterOuts(0),
      name(gNullCharPtr),
      label(gNullCharPtr),
      maker(gNullCharPtr),
      copyright(gNullCharPtr) {}

_CarlaParameterInfo::_CarlaParameterInfo() noexcept
    : name(gNullCharPtr),
      symbol(gNullCharPtr),
      unit(gNullCharPtr),
      scalePointCount(0) {}

_CarlaParameterInfo::~_CarlaParameterInfo() noexcept
{
    if (name != gNullCharPtr)
        delete[] name;
    if (symbol != gNullCharPtr)
        delete[] symbol;
    if (unit != gNullCharPtr)
        delete[] unit;
}

_CarlaScalePointInfo::_CarlaScalePointInfo() noexcept
    : value(0.0f),
      label(gNullCharPtr) {}

_CarlaScalePointInfo::~_CarlaScalePointInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
}

_CarlaTransportInfo::_CarlaTransportInfo() noexcept
    : playing(false),
      frame(0),
      bar(0),
      beat(0),
      tick(0),
      bpm(0.0) {}

// -------------------------------------------------------------------------------------------------------------------
// API

const char* carla_get_complete_license_text()
{
    carla_debug("carla_get_complete_license_text()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        CarlaString text1, text2, text3, text4, text5;

        text1 += "<p>This current Carla build is using the following features and 3rd-party code:</p>";
        text1 += "<ul>";

        // Plugin formats
#ifdef WANT_LADSPA
        text2 += "<li>LADSPA plugin support, http://www.ladspa.org/</li>";
#endif
#ifdef WANT_DSSI
        text2 += "<li>DSSI plugin support, http://dssi.sourceforge.net/</li>";
#endif
#ifdef WANT_LV2
        text2 += "<li>LV2 plugin support, http://lv2plug.in/</li>";
#endif
#ifdef WANT_VST
# ifdef VESTIGE_HEADER
        text2 += "<li>VST plugin support, using VeSTige header by Javier Serrano Polo</li>";
# else
        text2 += "<li>VST plugin support, using official VST SDK 2.4 (trademark of Steinberg Media Technologies GmbH)</li>";
# endif
#endif
#ifdef WANT_AU
        text2 += "<li>AU plugin support</li>"; // FIXME
#endif

        // Files
#ifdef WANT_CSOUND
        text2 += "<li>CSound library for csd support</li>"; // FIXME
#endif

        // Sample kit libraries
#ifdef WANT_FLUIDSYNTH
        text2 += "<li>FluidSynth library for SF2 support, http://www.fluidsynth.org/</li>";
#endif
#ifdef WANT_LINUXSAMPLER
        text2 += "<li>LinuxSampler library for GIG and SFZ support*, http://www.linuxsampler.org/</li>";
#endif

#ifdef WANT_NATIVE
        // Internal plugins
# ifdef HAVE_OPENGL
        text3 += "<li>DISTRHO Mini-Series plugin code, based on LOSER-dev suite by Michael Gruhn</li>";
# endif
        text3 += "<li>NekoFilter plugin code, based on lv2fil by Nedko Arnaudov and Fons Adriaensen</li>";
        //text1 += "<li>SunVox library file support, http://www.warmplace.ru/soft/sunvox/</li>"; // unfinished
# ifdef WANT_AUDIOFILE
        text3 += "<li>AudioDecoder library for Audio file support, by Robin Gareus</li>";
# endif
# ifdef WANT_MIDIFILE
        text3 += "<li>LibSMF library for MIDI file support, http://libsmf.sourceforge.net/</li>";
# endif
# ifdef WANT_ZYNADDSUBFX
        text3 += "<li>ZynAddSubFX plugin code, http://zynaddsubfx.sf.net/</li>";
#  ifdef WANT_ZYNADDSUBFX_UI
        text3 += "<li>ZynAddSubFX UI using NTK, http://non.tuxfamily.org/wiki/NTK</li>";
#  endif
# endif
#endif

        // misc libs
        text4 += "<li>liblo library for OSC support, http://liblo.sourceforge.net/</li>";
#ifdef WANT_LV2
        text4 += "<li>serd, sord, sratom and lilv libraries for LV2 discovery, http://drobilla.net/software/lilv/</li>";
#endif
        text4 += "<li>RtAudio+RtMidi libraries for extra Audio and MIDI support, http://www.music.mcgill.ca/~gary/rtaudio/</li>";

        // end
        text4 += "</ul>";

        // code snippets
        text5 += "<p>Additionally, Carla uses code snippets from the following projects:</p>";
        text5 += "<ul>";
        text5 += "<li>Pointer and data leak utils from JUCE, http://www.rawmaterialsoftware.com/juce.php</li>";
        text5 += "<li>Shared memory utils from dssi-vst, http://www.breakfastquay.com/dssi-vst/</li>";
        text5 += "<li>Real-time memory pool, by Nedko Arnaudov</li>";
        text5 += "</ul>";

#ifdef WANT_LINUXSAMPLER
        // LinuxSampler GPL exception
        text5 += "<p>(*) Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors.</p>";
#endif

        retText = text1 + text2 + text3 + text4 + text5;
    }

    return retText;
}

const char* carla_get_supported_file_extensions()
{
    carla_debug("carla_get_supported_file_extensions()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        // Base types
        retText += "*.carxp;*.carxs";

        // CSound
#ifdef WANT_CSOUND
        retText += ";*.csd";
#endif

        // Sample kits
#ifdef WANT_FLUIDSYNTH
        retText += ";*.sf2";
#endif
#ifdef WANT_LINUXSAMPLER
        retText += ";*.gig;*.sfz";
#endif

        // Files provided by internal plugins
#ifdef WANT_AUDIOFILE
        retText += ";*.aiff;*.flac;*.oga;*.ogg;*.w64;*.wav";
# ifdef HAVE_FFMPEG
        retText += ";*.3g2;*.3gp;*.aac;*.ac3;*.amr;*.ape;*.mp2;*.mp3;*.mpc;*.wma";
# endif
#endif
#ifdef WANT_MIDIFILE
        retText += ";*.mid;*.midi";
#endif

        // Plugin presets
#ifdef WANT_ZYNADDSUBFX
        retText += ";*.xmz;*.xiz";
#endif
    }

    return retText;
}

// -------------------------------------------------------------------------------------------------------------------

unsigned int carla_get_engine_driver_count()
{
    carla_debug("carla_get_engine_driver_count()");

    return CarlaEngine::getDriverCount();
}

const char* carla_get_engine_driver_name(unsigned int index)
{
    carla_debug("carla_get_engine_driver_name(%i)", index);

    return CarlaEngine::getDriverName(index);
}

const char* const* carla_get_engine_driver_device_names(unsigned int index)
{
    carla_debug("carla_get_engine_driver_device_names(%i)", index);

    return CarlaEngine::getDriverDeviceNames(index);
}

const EngineDriverDeviceInfo* carla_get_engine_driver_device_info(unsigned int index, const char* name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("carla_get_engine_driver_device_info(%i, \"%s\")", index, name);

    if (const EngineDriverDeviceInfo* const ret = CarlaEngine::getDriverDeviceInfo(index, name))
    {
        static EngineDriverDeviceInfo devInfo;
        static const uint32_t nullBufferSizes[] = { 0   };
        static const double   nullSampleRates[] = { 0.0 };

        devInfo.hints       = ret->hints;
        devInfo.bufferSizes = (ret->bufferSizes != nullptr) ? ret->bufferSizes : nullBufferSizes;
        devInfo.sampleRates = (ret->sampleRates != nullptr) ? ret->sampleRates : nullSampleRates;
        return &devInfo;
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

unsigned int carla_get_internal_plugin_count()
{
    carla_debug("carla_get_internal_plugin_count()");

#ifdef WANT_NATIVE
    return static_cast<unsigned int>(CarlaPlugin::getNativePluginCount());
#else
    return 0;
#endif
}

const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int index)
{
    carla_debug("carla_get_internal_plugin_info(%i)", index);

#ifdef WANT_NATIVE
    static CarlaNativePluginInfo info;

    const NativePluginDescriptor* const nativePlugin(CarlaPlugin::getNativePluginDescriptor(index));

    // as internal plugin, this must never fail
    CARLA_SAFE_ASSERT_RETURN(nativePlugin != nullptr, nullptr);

     info.category = static_cast<CB::PluginCategory>(nativePlugin->category);
     info.hints    = 0x0;

     if (nativePlugin->hints & ::PLUGIN_IS_RTSAFE)
         info.hints |= CB::PLUGIN_IS_RTSAFE;
     if (nativePlugin->hints & ::PLUGIN_IS_SYNTH)
         info.hints |= CB::PLUGIN_IS_SYNTH;
     if (nativePlugin->hints & ::PLUGIN_HAS_UI)
         info.hints |= CB::PLUGIN_HAS_CUSTOM_UI;
     if (nativePlugin->hints & ::PLUGIN_NEEDS_FIXED_BUFFERS)
         info.hints |= CB::PLUGIN_NEEDS_FIXED_BUFFERS;
     if (nativePlugin->hints & ::PLUGIN_NEEDS_SINGLE_THREAD)
         info.hints |= CB::PLUGIN_NEEDS_SINGLE_THREAD;

     info.audioIns  = nativePlugin->audioIns;
     info.audioOuts = nativePlugin->audioOuts;
     info.midiIns   = nativePlugin->midiIns;
     info.midiOuts  = nativePlugin->midiOuts;
     info.parameterIns  = nativePlugin->paramIns;
     info.parameterOuts = nativePlugin->paramOuts;

     info.name      = nativePlugin->name;
     info.label     = nativePlugin->label;
     info.maker     = nativePlugin->maker;
     info.copyright = nativePlugin->copyright;

     checkStringPtr(info.name);
     checkStringPtr(info.label);
     checkStringPtr(info.maker);
     checkStringPtr(info.copyright);

    return &info;
#else
    return nullptr;

    // unused
    (void)index;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaEngine* carla_get_engine()
{
    carla_debug("carla_get_engine()");

    return gStandalone.engine;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_engine_init(const char* driverName, const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);

    if (gStandalone.engine != nullptr)
    {
        carla_stderr2("Engine is already running");
        gStandalone.lastError = "Engine is already running";
        return false;
    }

#ifdef CARLA_OS_WIN
    carla_setenv("WINEASIO_CLIENT_NAME", clientName);
#endif

    // TODO: make this an option, put somewhere else
    if (std::getenv("WINE_RT") == nullptr)
    {
        carla_setenv("WINE_RT", "15");
        carla_setenv("WINE_SVR_RT", "10");
    }

    gStandalone.engine = CarlaEngine::newDriverByName(driverName);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("The seleted audio driver is not available");
        gStandalone.lastError = "The seleted audio driver is not available";
        return false;
    }

    gStandalone.engine->setCallback(gStandalone.engineCallback, gStandalone.engineCallbackPtr);
    gStandalone.engine->setFileCallback(gStandalone.fileCallback, gStandalone.fileCallbackPtr);

#ifdef BUILD_BRIDGE
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,                     nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        CB::ENGINE_TRANSPORT_MODE_JACK,                               nullptr);
#else
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          static_cast<int>(gStandalone.engineOptions.processMode),      nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        static_cast<int>(gStandalone.engineOptions.transportMode),    nullptr);
#endif

    gStandalone.engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          gStandalone.engineOptions.forceStereo         ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, gStandalone.engineOptions.preferPluginBridges ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     gStandalone.engineOptions.preferUiBridges     ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     gStandalone.engineOptions.uisAlwaysOnTop      ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,        static_cast<int>(gStandalone.engineOptions.maxParameters),    nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    static_cast<int>(gStandalone.engineOptions.uiBridgesTimeout), nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_NUM_PERIODS,     static_cast<int>(gStandalone.engineOptions.audioNumPeriods),  nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE,     static_cast<int>(gStandalone.engineOptions.audioBufferSize),  nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE,     static_cast<int>(gStandalone.engineOptions.audioSampleRate),  nullptr);

    if (gStandalone.engineOptions.audioDevice != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_DEVICE,   0, gStandalone.engineOptions.audioDevice);

    if (gStandalone.engineOptions.binaryDir != nullptr && gStandalone.engineOptions.binaryDir[0] != '\0')
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,  0, gStandalone.engineOptions.binaryDir);

    if (gStandalone.engineOptions.resourceDir != nullptr && gStandalone.engineOptions.resourceDir[0] != '\0')
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES, 0, gStandalone.engineOptions.resourceDir);

    if (gStandalone.engineOptions.frontendWinId != 0)
    {
        char strBuf[STR_MAX+1];
        std::sprintf(strBuf, P_UINTPTR, gStandalone.engineOptions.frontendWinId);
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, strBuf);
    }
    else
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0");

    if (gStandalone.engine->init(clientName))
    {
        gStandalone.lastError = "No error";
#ifdef HAVE_JUCE
        gStandalone.init();
#endif
        return true;
    }
    else
    {
        gStandalone.lastError = gStandalone.engine->getLastError();
        delete gStandalone.engine;
        gStandalone.engine = nullptr;
        return false;
    }
}

#ifdef BUILD_BRIDGE
bool carla_engine_init_bridge(const char audioBaseName[6+1], const char controlBaseName[6+1], const char timeBaseName[6+1], const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(audioBaseName != nullptr && audioBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(controlBaseName != nullptr && controlBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(timeBaseName != nullptr && timeBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init_bridge(\"%s\", \"%s\", \"%s\", \"%s\")", audioBaseName, controlBaseName, timeBaseName, clientName);

    if (gStandalone.engine != nullptr)
    {
        carla_stderr2("Engine is already running");
        gStandalone.lastError = "Engine is already running";
        return false;
    }

    gStandalone.engine = CarlaEngine::newBridge(audioBaseName, controlBaseName, timeBaseName);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("The seleted audio driver is not available!");
        gStandalone.lastError = "The seleted audio driver is not available!";
        return false;
    }

    gStandalone.engine->setCallback(gStandalone.engineCallback, gStandalone.engineCallbackPtr);
    gStandalone.engine->setFileCallback(gStandalone.fileCallback, gStandalone.fileCallbackPtr);

    // forced options for bridge mode
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          CB::ENGINE_PROCESS_MODE_BRIDGE,   nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        CB::ENGINE_TRANSPORT_MODE_BRIDGE, nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          false, nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, false, nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     false, nullptr);

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP,      (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const maxParameters = std::getenv("ENGINE_OPTION_MAX_PARAMETERS"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,          std::atoi(maxParameters), nullptr);

    if (const char* const uiBridgesTimeout = std::getenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT,      std::atoi(uiBridgesTimeout), nullptr);

    if (const char* const binaryDir = std::getenv("ENGINE_OPTION_PATH_BINARIES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,        0, binaryDir);

    if (const char* const resourceDir = std::getenv("ENGINE_OPTION_PATH_RESOURCES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,       0, resourceDir);

    if (const char* const frontendWinId = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID,      0, frontendWinId);

    if (gStandalone.engine->init(clientName))
    {
        gStandalone.lastError = "No error";
#ifdef HAVE_JUCE
        gStandalone.init();
#endif
        return true;
    }
    else
    {
        gStandalone.lastError = gStandalone.engine->getLastError();
        delete gStandalone.engine;
        gStandalone.engine = nullptr;
        return false;
    }
}
#endif

bool carla_engine_close()
{
    carla_debug("carla_engine_close()");

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    gStandalone.engine->setAboutToClose();
    gStandalone.engine->removeAllPlugins();

    const bool closed(gStandalone.engine->close());

    if (! closed)
        gStandalone.lastError = gStandalone.engine->getLastError();

#ifdef HAVE_JUCE
    gStandalone.close();
#endif

    delete gStandalone.engine;
    gStandalone.engine = nullptr;

    return closed;
}

void carla_engine_idle()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    gNSM.idle();
#if defined(HAVE_JUCE) && ! defined(CARLA_OS_LINUX)
    gStandalone.idle();
#endif
    gStandalone.engine->idle();
}

bool carla_is_engine_running()
{
    return (gStandalone.engine != nullptr && gStandalone.engine->isRunning());
}

void carla_set_engine_about_to_close()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_engine_about_to_close()");

    gStandalone.engine->setAboutToClose();
}

void carla_set_engine_callback(EngineCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p, %p)", func, ptr);

    gStandalone.engineCallback    = func;
    gStandalone.engineCallbackPtr = ptr;

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setCallback(func, ptr);

//#ifdef WANT_LOGS
//    gLogThread.setCallback(func, ptr);
//#endif
}

#ifndef BUILD_BRIDGE
void carla_set_engine_option(EngineOption option, int value, const char* valueStr)
{
    carla_debug("carla_set_engine_option(%i:%s, %i, \"%s\")", option, CB::EngineOption2Str(option), value, valueStr);

    switch (option)
    {
    case CB::ENGINE_OPTION_DEBUG:
        break;

    case CB::ENGINE_OPTION_PROCESS_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_PROCESS_MODE_SINGLE_CLIENT && value < CB::ENGINE_PROCESS_MODE_BRIDGE,);
        gStandalone.engineOptions.processMode = static_cast<CB::EngineProcessMode>(value);
        break;

    case CB::ENGINE_OPTION_TRANSPORT_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_TRANSPORT_MODE_INTERNAL && value < CB::ENGINE_TRANSPORT_MODE_BRIDGE,);
        gStandalone.engineOptions.transportMode = static_cast<CB::EngineTransportMode>(value);
        break;

    case CB::ENGINE_OPTION_FORCE_STEREO:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.forceStereo = (value != 0);
        break;

    case CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.preferPluginBridges = (value != 0);
        break;

    case CB::ENGINE_OPTION_PREFER_UI_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.preferUiBridges = (value != 0);
        break;

    case CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.uisAlwaysOnTop = (value != 0);
        break;

    case CB::ENGINE_OPTION_MAX_PARAMETERS:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        gStandalone.engineOptions.maxParameters = static_cast<unsigned int>(value);
        break;

    case CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        gStandalone.engineOptions.uiBridgesTimeout = static_cast<unsigned int>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_NUM_PERIODS:
        CARLA_SAFE_ASSERT_RETURN(value >= 2 && value <= 3,);
        gStandalone.engineOptions.audioNumPeriods = static_cast<unsigned int>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        gStandalone.engineOptions.audioBufferSize = static_cast<unsigned int>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        gStandalone.engineOptions.audioSampleRate = static_cast<unsigned int>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (gStandalone.engineOptions.audioDevice != nullptr)
            delete[] gStandalone.engineOptions.audioDevice;

        gStandalone.engineOptions.audioDevice = carla_strdup(valueStr);
        break;

    case CB:: ENGINE_OPTION_NSM_INIT:
        CARLA_SAFE_ASSERT_RETURN(value != 0,);
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        gNSM.announce(value, valueStr);
        break;

    case CB::ENGINE_OPTION_PATH_BINARIES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.binaryDir != nullptr)
            delete[] gStandalone.engineOptions.binaryDir;

        gStandalone.engineOptions.binaryDir = carla_strdup(valueStr);
        break;

    case CB::ENGINE_OPTION_PATH_RESOURCES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.resourceDir != nullptr)
            delete[] gStandalone.engineOptions.resourceDir;

        gStandalone.engineOptions.resourceDir = carla_strdup(valueStr);
        break;

    case CB::ENGINE_OPTION_FRONTEND_WIN_ID:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        const long long winId(std::strtoll(valueStr, nullptr, 16));
        CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
        gStandalone.engineOptions.frontendWinId = static_cast<uintptr_t>(winId);
        break;
    }

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setOption(option, value, valueStr);
}
#endif

void carla_set_file_callback(FileCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_file_callback(%p, %p)", func, ptr);

    gStandalone.fileCallback    = func;
    gStandalone.fileCallbackPtr = ptr;

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setFileCallback(func, ptr);
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_file(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_file(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->loadFile(filename);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_load_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_project(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->loadProject(filename);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_save_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_save_project(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->saveProject(filename);

    carla_stderr2("Engine was never initiated");
    gStandalone.lastError = "Engine was never initiated";
    return false;
}

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(int groupIdA, int portIdA, int groupIdB, int portIdB)
{
    carla_debug("carla_patchbay_connect(%i, %i, %i, %i)", groupIdA, portIdA, groupIdB, portIdB);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayConnect(groupIdA, portIdA, groupIdB, portIdB);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_patchbay_disconnect(uint connectionId)
{
    carla_debug("carla_patchbay_disconnect(%i)", connectionId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayDisconnect(connectionId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_patchbay_refresh()
{
    carla_debug("carla_patchbay_refresh()");

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayRefresh();

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_transport_play()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_transport_play()");

    gStandalone.engine->transportPlay();
}

void carla_transport_pause()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_transport_pause()");

    gStandalone.engine->transportPause();
}

void carla_transport_relocate(uint64_t frame)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_transport_relocate(%i)", frame);

    gStandalone.engine->transportRelocate(frame);
}

uint64_t carla_get_current_transport_frame()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), 0);

    const CB::EngineTimeInfo& timeInfo(gStandalone.engine->getTimeInfo());
    return timeInfo.frame;
}

const CarlaTransportInfo* carla_get_transport_info()
{
    static CarlaTransportInfo retInfo;

    // reset
    retInfo.playing = false;
    retInfo.frame   = 0;
    retInfo.bar     = 0;
    retInfo.beat    = 0;
    retInfo.tick    = 0;
    retInfo.bpm     = 0.0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), &retInfo);

    const CB::EngineTimeInfo& timeInfo(gStandalone.engine->getTimeInfo());

    retInfo.playing = timeInfo.playing;
    retInfo.frame   = timeInfo.frame;

    if (timeInfo.valid & CB::EngineTimeInfo::kValidBBT)
    {
        retInfo.bar  = timeInfo.bbt.bar;
        retInfo.beat = timeInfo.bbt.beat;
        retInfo.tick = timeInfo.bbt.tick;
        retInfo.bpm  = timeInfo.bbt.beatsPerMinute;
    }

    return &retInfo;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* name, const char* label, int64_t uniqueId, const void* extraPtr)
{
    CARLA_SAFE_ASSERT_RETURN(label != nullptr /*&& label[0] != '\0'*/, false);
    carla_debug("carla_add_plugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p)", btype, CB::BinaryType2Str(btype), ptype, CB::PluginType2Str(ptype), filename, name, label, uniqueId, extraPtr);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->addPlugin(btype, ptype, filename, name, label, uniqueId, extraPtr);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_remove_plugin(uint pluginId)
{
    carla_debug("carla_remove_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->removePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_remove_all_plugins()
{
    carla_debug("carla_remove_all_plugins()");

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->removeAllPlugins();

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

const char* carla_rename_plugin(uint pluginId, const char* newName)
{
    CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', nullptr);
    carla_debug("carla_rename_plugin(%i, \"%s\")", pluginId, newName);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->renamePlugin(pluginId, newName);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return nullptr;
}

bool carla_clone_plugin(uint pluginId)
{
    carla_debug("carla_clone_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->clonePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_replace_plugin(uint pluginId)
{
    carla_debug("carla_replace_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->replacePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_switch_plugins(uint pluginIdA, uint pluginIdB)
{
    CARLA_SAFE_ASSERT_RETURN(pluginIdA != pluginIdB, false);
    carla_debug("carla_switch_plugins(%i, %i)", pluginIdA, pluginIdB);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->switchPlugins(pluginIdA, pluginIdB);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_plugin_state(%i, \"%s\")", pluginId, filename);

    if (gStandalone.engine == nullptr || ! gStandalone.engine->isRunning())
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->loadStateFromFile(filename);

    carla_stderr2("carla_load_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

bool carla_save_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_save_plugin_state(%i, \"%s\")", pluginId, filename);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    // allow to save even if engine isn't running

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->saveStateToFile(filename);

    carla_stderr2("carla_save_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(uint pluginId)
{
    carla_debug("carla_get_plugin_info(%i)", pluginId);

    static CarlaPluginInfo info;

    // reset
    info.type             = CB::PLUGIN_NONE;
    info.category         = CB::PLUGIN_CATEGORY_NONE;
    info.hints            = 0x0;
    info.optionsAvailable = 0x0;
    info.optionsEnabled   = 0x0;
    info.filename         = gNullCharPtr;
    info.name             = gNullCharPtr;
    info.iconName         = gNullCharPtr;
    info.uniqueId         = 0;

    // cleanup
    if (info.label != gNullCharPtr)
    {
        delete[] info.label;
        info.label = gNullCharPtr;
    }

    if (info.maker != gNullCharPtr)
    {
        delete[] info.maker;
        info.maker = gNullCharPtr;
    }

    if (info.copyright != gNullCharPtr)
    {
        delete[] info.copyright;
        info.copyright = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        char strBufLabel[STR_MAX+1];
        char strBufMaker[STR_MAX+1];
        char strBufCopyright[STR_MAX+1];

        carla_zeroChar(strBufLabel, STR_MAX+1);
        carla_zeroChar(strBufMaker, STR_MAX+1);
        carla_zeroChar(strBufCopyright, STR_MAX+1);

        info.type     = plugin->getType();
        info.category = plugin->getCategory();
        info.hints    = plugin->getHints();
        info.filename = plugin->getFilename();
        info.name     = plugin->getName();
        info.iconName = plugin->getIconName();
        info.uniqueId = plugin->getUniqueId();

        info.optionsAvailable = plugin->getOptionsAvailable();
        info.optionsEnabled   = plugin->getOptionsEnabled();

        plugin->getLabel(strBufLabel);
        info.label = carla_strdup(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = carla_strdup(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = carla_strdup(strBufCopyright);

        checkStringPtr(info.filename);
        checkStringPtr(info.name);
        checkStringPtr(info.iconName);

        return &info;
    }

    carla_stderr2("carla_get_plugin_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(uint pluginId)
{
    carla_debug("carla_get_audio_port_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->getAudioInCount();
        info.outs  = plugin->getAudioOutCount();
        return &info;
    }

    carla_stderr2("carla_get_audio_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(uint pluginId)
{
    carla_debug("carla_get_midi_port_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->getMidiInCount();
        info.outs  = plugin->getMidiOutCount();
        return &info;
    }

    carla_stderr2("carla_get_midi_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(uint pluginId)
{
    carla_debug("carla_get_parameter_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        plugin->getParameterCountInfo(info.ins, info.outs);
        return &info;
    }

    carla_stderr2("carla_get_parameter_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaParameterInfo* carla_get_parameter_info(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_info(%i, %i)", pluginId, parameterId);

    static CarlaParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
    if (info.name != gNullCharPtr)
    {
        delete[] info.name;
        info.name = gNullCharPtr;
    }

    if (info.symbol != gNullCharPtr)
    {
        delete[] info.symbol;
        info.symbol = gNullCharPtr;
    }

    if (info.unit != gNullCharPtr)
    {
        delete[] info.unit;
        info.unit = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            char strBufName[STR_MAX+1];
            char strBufSymbol[STR_MAX+1];
            char strBufUnit[STR_MAX+1];

            carla_zeroChar(strBufName, STR_MAX+1);
            carla_zeroChar(strBufSymbol, STR_MAX+1);
            carla_zeroChar(strBufUnit, STR_MAX+1);

            info.scalePointCount = plugin->getParameterScalePointCount(parameterId);

            plugin->getParameterName(parameterId, strBufName);
            info.name = carla_strdup(strBufName);

            plugin->getParameterSymbol(parameterId, strBufSymbol);
            info.symbol = carla_strdup(strBufSymbol);

            plugin->getParameterUnit(parameterId, strBufUnit);
            info.unit = carla_strdup(strBufUnit);
        }
        else
            carla_stderr2("carla_get_parameter_info(%i, %i) - parameterId out of bounds", pluginId, parameterId);

        return &info;
    }

    carla_stderr2("carla_get_parameter_info(%i, %i) - could not find plugin", pluginId, parameterId);
    return &info;
}

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(uint pluginId, uint32_t parameterId, uint32_t scalePointId)
{
    carla_debug("carla_get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameterId, scalePointId);
    CARLA_ASSERT(gStandalone.engine != nullptr);

    static CarlaScalePointInfo info;

    // reset
    info.value = 0.0f;

    // cleanup
    if (info.label != gNullCharPtr)
    {
        delete[] info.label;
        info.label = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            if (scalePointId < plugin->getParameterScalePointCount(parameterId))
            {
                char strBufLabel[STR_MAX+1];
                carla_zeroChar(strBufLabel, STR_MAX+1);

                info.value = plugin->getParameterScalePointValue(parameterId, scalePointId);

                plugin->getParameterScalePointLabel(parameterId, scalePointId, strBufLabel);
                info.label = carla_strdup(strBufLabel);
            }
            else
                carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - scalePointId out of bounds", pluginId, parameterId, scalePointId);
        }
        else
            carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, scalePointId);

        return &info;
    }

    carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", pluginId, parameterId, scalePointId);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const ParameterData* carla_get_parameter_data(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);

    static const ParameterData fallbackParameterData = { CB::PARAMETER_UNKNOWN, 0x0, CB::PARAMETER_NULL, -1, -1, 0 };

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &fallbackParameterData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return &plugin->getParameterData(parameterId);

        carla_stderr2("carla_get_parameter_data(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &fallbackParameterData;
    }

    carla_stderr2("carla_get_parameter_data(%i, %i) - could not find plugin", pluginId, parameterId);
    return &fallbackParameterData;
}

const ParameterRanges* carla_get_parameter_ranges(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);

    static const ParameterRanges fallbackParamRanges = { 0.0f, 0.0f, 1.0f, 0.01f, 0.0001f, 0.1f };

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &fallbackParamRanges);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return &plugin->getParameterRanges(parameterId);

        carla_stderr2("carla_get_parameter_ranges(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &fallbackParamRanges;
    }

    carla_stderr2("carla_get_parameter_ranges(%i, %i) - could not find plugin", pluginId, parameterId);
    return &fallbackParamRanges;
}

const MidiProgramData* carla_get_midi_program_data(uint pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);

    static MidiProgramData midiProgData;

    // reset
    midiProgData.bank    = 0;
    midiProgData.program = 0;
    midiProgData.name    = gNullCharPtr;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &midiProgData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
        {
            const MidiProgramData& ret(plugin->getMidiProgramData(midiProgramId));
            carla_copyStruct<MidiProgramData>(midiProgData, ret);
            checkStringPtr(midiProgData.name);
            return &midiProgData;
        }

        carla_stderr2("carla_get_midi_program_data(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return &midiProgData;
    }

    carla_stderr2("carla_get_midi_program_data(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return &midiProgData;
}

const CustomData* carla_get_custom_data(uint pluginId, uint32_t customDataId)
{
    carla_debug("carla_get_custom_data(%i, %i)", pluginId, customDataId);

    static CustomData customData;

    // reset
    customData.type  = gNullCharPtr;
    customData.key   = gNullCharPtr;
    customData.value = gNullCharPtr;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &customData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (customDataId < plugin->getCustomDataCount())
        {
            const CustomData& ret(plugin->getCustomData(customDataId));
            carla_copyStruct<CustomData>(customData, ret);
            checkStringPtr(customData.type);
            checkStringPtr(customData.key);
            checkStringPtr(customData.value);
            return &customData;
        }

        carla_stderr2("carla_get_custom_data(%i, %i) - customDataId out of bounds", pluginId, customDataId);
        return &customData;
    }

    carla_stderr2("carla_get_custom_data(%i, %i) - could not find plugin", pluginId, customDataId);
    return &customData;
}

const char* carla_get_chunk_data(uint pluginId)
{
    carla_debug("carla_get_chunk_data(%i)", pluginId);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);

    static CarlaString chunkData;

    // cleanup
    chunkData.clear();

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS)
        {
            void* data = nullptr;
            const int32_t dataSize(plugin->getChunkData(&data));

            if (data != nullptr && dataSize > 0)
            {
                chunkData = QByteArray((char*)data, dataSize).toBase64().constData();

                return chunkData;
            }
            else
                carla_stderr2("carla_get_chunk_data(%i) - got invalid chunk data", pluginId);
        }
        else
            carla_stderr2("carla_get_chunk_data(%i) - plugin does not use chunks", pluginId);

        return nullptr;
    }

    carla_stderr2("carla_get_chunk_data(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_parameter_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getParameterCount();

    carla_stderr2("carla_get_parameter_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_program_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getProgramCount();

    carla_stderr2("carla_get_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_midi_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_midi_program_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getMidiProgramCount();

    carla_stderr2("carla_get_midi_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_custom_data_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_custom_data_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCustomDataCount();

    carla_stderr2("carla_get_custom_data_count(%i) - could not find plugin", pluginId);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_parameter_text(%i, %i)", pluginId, parameterId);

    static char textBuf[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            carla_zeroChar(textBuf, STR_MAX+1);
            plugin->getParameterText(parameterId, textBuf);
            return textBuf;
        }

        carla_stderr2("carla_get_parameter_text(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return nullptr;
    }

    carla_stderr2("carla_get_parameter_text(%i, %i) - could not find plugin", pluginId, parameterId);
    return nullptr;
}

const char* carla_get_program_name(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_program_name(%i, %i)", pluginId, programId);

    static char programName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->getProgramCount())
        {
            carla_zeroChar(programName, STR_MAX+1);
            plugin->getProgramName(programId, programName);
            return programName;
        }

        carla_stderr2("carla_get_program_name(%i, %i) - programId out of bounds", pluginId, programId);
        return nullptr;
    }

    carla_stderr2("carla_get_program_name(%i, %i) - could not find plugin", pluginId, programId);
    return nullptr;
}

const char* carla_get_midi_program_name(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_midi_program_name(%i, %i)", pluginId, midiProgramId);

    static char midiProgramName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
        {
            carla_zeroChar(midiProgramName, STR_MAX+1);
            plugin->getMidiProgramName(midiProgramId, midiProgramName);
            return midiProgramName;
        }

        carla_stderr2("carla_get_midi_program_name(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return nullptr;
    }

    carla_stderr2("carla_get_midi_program_name(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return nullptr;
}

const char* carla_get_real_plugin_name(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_real_plugin_name(%i)", pluginId);

    static char realPluginName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        carla_zeroChar(realPluginName, STR_MAX+1);
        plugin->getRealName(realPluginName);
        return realPluginName;
    }

    carla_stderr2("carla_get_real_plugin_name(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);
    carla_debug("carla_get_current_program_index(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCurrentProgram();

    carla_stderr2("carla_get_current_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

int32_t carla_get_current_midi_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);
    carla_debug("carla_get_current_midi_program_index(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCurrentMidiProgram();

    carla_stderr2("carla_get_current_midi_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);
    carla_debug("carla_get_default_parameter_value(%i, %i)", pluginId, parameterId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->getParameterRanges(parameterId).def;

        carla_stderr2("carla_get_default_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_default_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_current_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->getParameterValue(parameterId);

        carla_stderr2("carla_get_current_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_current_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_internal_parameter_value(uint pluginId, int32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);
    CARLA_SAFE_ASSERT_RETURN(parameterId != CB::PARAMETER_NULL && parameterId > CB::PARAMETER_MAX, 0.0f);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getInternalParameterValue(parameterId);

    carla_stderr2("carla_get_internal_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_input_peak_value(uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    return gStandalone.engine->getInputPeak(pluginId, isLeft);
}

float carla_get_output_peak_value(uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    return gStandalone.engine->getOutputPeak(pluginId, isLeft);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_option(uint pluginId, uint option, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_option(%i, %i, %s)", pluginId, option, bool2str(yesNo));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setOption(option, yesNo);

    carla_stderr2("carla_set_option(%i, %i, %s) - could not find plugin", pluginId, option, bool2str(yesNo));
}

void carla_set_active(uint pluginId, bool onOff)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_active(%i, %s)", pluginId, bool2str(onOff));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setActive(onOff, true, false);

    carla_stderr2("carla_set_active(%i, %s) - could not find plugin", pluginId, bool2str(onOff));
}

#ifndef BUILD_BRIDGE
void carla_set_drywet(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_drywet(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setDryWet(value, true, false);

    carla_stderr2("carla_set_drywet(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_volume(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_volume(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setVolume(value, true, false);

    carla_stderr2("carla_set_volume(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_left(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_balance_left(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setBalanceLeft(value, true, false);

    carla_stderr2("carla_set_balance_left(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_right(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_balance_right(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setBalanceRight(value, true, false);

    carla_stderr2("carla_set_balance_right(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_panning(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_panning(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setPanning(value, true, false);

    carla_stderr2("carla_set_panning(%i, %f) - could not find plugin", pluginId, value);
}
#endif

void carla_set_ctrl_channel(uint pluginId, int8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_ctrl_channel(%i, %i)", pluginId, channel);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setCtrlChannel(channel, true, false);

    carla_stderr2("carla_set_ctrl_channel(%i, %i) - could not find plugin", pluginId, channel);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(uint pluginId, uint32_t parameterId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_parameter_value(%i, %i, %f)", pluginId, parameterId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterValue(parameterId, value, true, true, false);

        carla_stderr2("carla_set_parameter_value(%i, %i, %f) - parameterId out of bounds", pluginId, parameterId, value);
        return;
    }

    carla_stderr2("carla_set_parameter_value(%i, %i, %f) - could not find plugin", pluginId, parameterId, value);
}

#ifndef BUILD_BRIDGE
void carla_set_parameter_midi_channel(uint pluginId, uint32_t parameterId, uint8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel >= MAX_MIDI_CHANNELS,);
    carla_debug("carla_set_parameter_midi_channel(%i, %i, %i)", pluginId, parameterId, channel);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterMidiChannel(parameterId, channel, true, false);

        carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, channel);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - could not find plugin", pluginId, parameterId, channel);
}

void carla_set_parameter_midi_cc(uint pluginId, uint32_t parameterId, int16_t cc)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc <= 0x5F,);
    carla_debug("carla_set_parameter_midi_cc(%i, %i, %i)", pluginId, parameterId, cc);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterMidiCC(parameterId, cc, true, false);

        carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, cc);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - could not find plugin", pluginId, parameterId, cc);
}
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_set_program(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_program(%i, %i)", pluginId, programId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->getProgramCount())
            return plugin->setProgram(static_cast<int32_t>(programId), true, true, false);

        carla_stderr2("carla_set_program(%i, %i) - programId out of bounds", pluginId, programId);
        return;
    }

    carla_stderr2("carla_set_program(%i, %i) - could not find plugin", pluginId, programId);
}

void carla_set_midi_program(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_midi_program(%i, %i)", pluginId, midiProgramId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
            return plugin->setMidiProgram(static_cast<int32_t>(midiProgramId), true, true, false);

        carla_stderr2("carla_set_midi_program(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return;
    }

    carla_stderr2("carla_set_midi_program(%i, %i) - could not find plugin", pluginId, midiProgramId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(uint pluginId, const char* type, const char* key, const char* value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setCustomData(type, key, value, true);

    carla_stderr2("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\") - could not find plugin", pluginId, type, key, value);
}

void carla_set_chunk_data(uint pluginId, const char* chunkData)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(chunkData != nullptr && chunkData[0] != '\0',);
    carla_debug("carla_set_chunk_data(%i, \"%s\")", pluginId, chunkData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS)
            return plugin->setChunkData(chunkData);

        carla_stderr2("carla_set_chunk_data(%i, \"%s\") - plugin does not use chunks", pluginId, chunkData);
        return;
    }

    carla_stderr2("carla_set_chunk_data(%i, \"%s\") - could not find plugin", pluginId, chunkData);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_prepare_for_save(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->prepareForSave();

    carla_stderr2("carla_prepare_for_save(%i) - could not find plugin", pluginId);
}

void carla_reset_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_reset_parameters(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->resetParameters();

    carla_stderr2("carla_reset_parameters(%i) - could not find plugin", pluginId);
}

void carla_randomize_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_randomize_parameters(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->randomizeParameters();

    carla_stderr2("carla_randomize_parameters(%i) - could not find plugin", pluginId);
}

#ifndef BUILD_BRIDGE
void carla_send_midi_note(uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    carla_stderr2("carla_send_midi_note(%i, %i, %i, %i) - could not find plugin", pluginId, channel, note, velocity);
}
#endif

void carla_show_custom_ui(uint pluginId, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_show_custom_ui(%i, %s)", pluginId, bool2str(yesNo));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->showCustomUI(yesNo);

    carla_stderr2("carla_show_custom_ui(%i, %s) - could not find plugin", pluginId, bool2str(yesNo));
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_buffer_size()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_buffer_size()");

    return gStandalone.engine->getBufferSize();
}

double carla_get_sample_rate()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0);
    carla_debug("carla_get_sample_rate()");

    return gStandalone.engine->getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_last_error()
{
    carla_debug("carla_get_last_error()");

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->getLastError();

    return gStandalone.lastError;
}

const char* carla_get_host_osc_url_tcp()
{
    carla_debug("carla_get_host_osc_url_tcp()");

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return nullptr;
    }

    return gStandalone.engine->getOscServerPathTCP();
}

const char* carla_get_host_osc_url_udp()
{
    carla_debug("carla_get_host_osc_url_udp()");

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return nullptr;
    }

    return gStandalone.engine->getOscServerPathUDP();
}

// -------------------------------------------------------------------------------------------------------------------

#ifdef WANT_DSSI
# include "CarlaDssiUtils.cpp"
#endif

#include "CarlaStateUtils.cpp"

#if 0
int main(int argc, char* argv[])
{
    return 0;
}
#endif

// -------------------------------------------------------------------------------------------------------------------
