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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaStandalone.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaMIDI.h"
#include "CarlaNative.h"
#include "CarlaStyle.hpp"

#include <QtCore/QSettings>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QApplication>
#else
# include <QtGui/QApplication>
#endif

using CarlaBackend::CarlaEngine;
using CarlaBackend::CarlaPlugin;
using CarlaBackend::CallbackFunc;
using CarlaBackend::EngineOptions;
using CarlaBackend::EngineTimeInfo;

// -------------------------------------------------------------------------------------------------------------------
// Single, standalone engine

struct CarlaBackendStandalone {
    CallbackFunc  callback;
    void*         callbackPtr;
    CarlaEngine*  engine;
    CarlaString   lastError;
    CarlaString   procName;
#ifndef BUILD_BRIDGE
    EngineOptions options;
#endif

    QApplication* app;
    bool needsInit;

    CarlaBackendStandalone()
        : callback(nullptr),
          callbackPtr(nullptr),
          engine(nullptr),
          app(qApp),
          needsInit(app == nullptr)
    {
        if (app != nullptr)
        {
            QSettings settings;

            if (settings.value("Main/UseProTheme", true).toBool())
            {
                CarlaStyle* const style(new CarlaStyle());
                app->setStyle(style);
                style->ready(app);

                QString color(settings.value("Main/ProThemeColor", "Black").toString());

                if (color == "Blue")
                    style->setColorScheme(CarlaStyle::COLOR_BLUE);
                else if (color == "System")
                    pass(); //style->setColorScheme(CarlaStyle::COLOR_SYSTEM);
                else
                    style->setColorScheme(CarlaStyle::COLOR_BLACK);
            }
        }
    }

    ~CarlaBackendStandalone()
    {
        CARLA_ASSERT(engine == nullptr);
    }

    void init()
    {
        if (! needsInit)
            return;
        if (app != nullptr)
            return;

        static int    argc = 0;
        static char** argv = nullptr;
        app = new QApplication(argc, argv, true);
    }

    void close()
    {
        if (! needsInit)
            return;
        if (app == nullptr)
            return;

        app->quit();
        app->processEvents();
        delete app;
        app = nullptr;
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(CarlaBackendStandalone)

} standalone;

// -------------------------------------------------------------------------------------------------------------------
// API

const char* carla_get_extended_license_text()
{
    carla_debug("carla_get_extended_license_text()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        CarlaString text1, text2;

        text1 += "<p>This current Carla build is using the following features and 3rd-party code:</p>";
        text1 += "<ul>";

        // Plugin formats
#ifdef WANT_LADSPA
        text1 += "<li>LADSPA plugin support, http://www.ladspa.org/</li>";
#endif
#ifdef WANT_DSSI
        text1 += "<li>DSSI plugin support, http://dssi.sourceforge.net/</li>";
#endif
#ifdef WANT_LV2
        text1 += "<li>LV2 plugin support, http://lv2plug.in/</li>";
#endif
#ifdef WANT_VST
# ifdef VESTIGE_HEADER
        text1 += "<li>VST plugin support, using VeSTige header by Javier Serrano Polo</li>";
# else
        text1 += "<li>VST plugin support, using official VST SDK 2.4 (trademark of Steinberg Media Technologies GmbH)</li>";
# endif
#endif

        // Sample kit libraries
#ifdef WANT_FLUIDSYNTH
        text1 += "<li>FluidSynth library for SF2 support, http://www.fluidsynth.org/</li>";
#endif
#ifdef WANT_LINUXSAMPLER
        text1 += "<li>LinuxSampler library for GIG and SFZ support*, http://www.linuxsampler.org/</li>";
#endif

        // Internal plugins
#ifdef WANT_OPENGL
        text1 += "<li>DISTRHO Mini-Series plugin code, based on LOSER-dev suite by Michael Gruhn</li>";
#endif
        text1 += "<li>NekoFilter plugin code, based on lv2fil by Nedko Arnaudov and Fons Adriaensen</li>";
        text1 += "<li>SunVox library file support, http://www.warmplace.ru/soft/sunvox/</li>";

#ifdef WANT_AUDIOFILE
        text1 += "<li>AudioDecoder library for Audio file support, by Robin Gareus</li>";
#endif
#ifdef WANT_MIDIFILE
        text1 += "<li>LibSMF library for MIDI file support, http://libsmf.sourceforge.net/</li>";
#endif
#ifdef WANT_ZYNADDSUBFX
        text1 += "<li>ZynAddSubFX plugin code, http://zynaddsubfx.sf.net/</li>";
# ifdef WANT_ZYNADDSUBFX_UI
        text1 += "<li>ZynAddSubFX UI using NTK, http://non.tuxfamily.org/wiki/NTK</li>";
# endif
#endif

        // misc libs
        text1 += "<li>liblo library for OSC support, http://liblo.sourceforge.net/</li>";
#ifdef WANT_LV2
        text1 += "<li>serd, sord, sratom and lilv libraries for LV2 discovery, http://drobilla.net/software/lilv/</li>";
#endif
#ifdef WANT_RTAUDIO
        text1 += "<li>RtAudio+RtMidi libraries for extra Audio and MIDI support, http://www.music.mcgill.ca/~gary/rtaudio/</li>";
#endif
        text1 += "</ul>";

        // code snippets
        text2 += "<p>Additionally, Carla uses code snippets from the following projects:</p>";
        text2 += "<ul>";
        text2 += "<li>Pointer and data leak utils from JUCE, http://www.rawmaterialsoftware.com/juce.php</li>";
        text2 += "<li>Shared memory utils from dssi-vst, http://www.breakfastquay.com/dssi-vst/</li>";
        text2 += "<li>Real-time memory pool, by Nedko Arnaudov</li>";
        text2 += "</ul>";

        // LinuxSampler GPL exception
#ifdef WANT_LINUXSAMPLER
        text2 += "<p>(*) Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors.</p>";
#endif

        retText += text1;
        retText += text2;
    }

    return retText;
}

const char* carla_get_supported_file_types()
{
    carla_debug("carla_get_supported_file_types()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        // Base types
        retText += "*.carxp;*.carxs";

        // Sample kits
#ifdef WANT_FLUIDSYNTH
        retText += ";*.sf2";
#endif
#ifdef WANT_LINUXSAMPLER
        retText += ";*.gig;*.sfz";
#endif

        // Files provided by internal plugins
#ifdef WANT_AUDIOFILE
        retText += "*.3g2;*.3gp;*.aac;*.ac3;*.aiff;*.amr;*.ape;*.flac;*.mp2;*.mp3;*.mpc;*.oga;*.ogg;*.w64;*.wav;*.wma;";
#endif
#ifdef WANT_MIDIFILE
        retText += ";*.mid;*.midi";
#endif

        // Plugin presets
#ifdef WANT_ZYNADDSUBFX
        retText += ";*.xmz";
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

const void* carla_get_engine_driver_options(unsigned int index)
{
    carla_debug("carla_get_engine_driver_options(%i)", index);

    return nullptr;

    // unused
    (void)index;
}

// -------------------------------------------------------------------------------------------------------------------

unsigned int carla_get_internal_plugin_count()
{
    carla_debug("carla_get_internal_plugin_count()");

#ifdef WANT_NATIVE
    return static_cast<unsigned int>(CarlaPlugin::getNativePluginCount());
#endif

    return 0;
}

const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int internalPluginId)
{
    carla_debug("carla_get_internal_plugin_info(%i)", internalPluginId);

    static CarlaNativePluginInfo info;

#ifdef WANT_NATIVE
    const PluginDescriptor* const nativePlugin(CarlaPlugin::getNativePluginDescriptor(internalPluginId));

    // as internal plugin, this must never fail
    CARLA_ASSERT(nativePlugin != nullptr);

    if (nativePlugin == nullptr)
        return nullptr;

     info.category = static_cast<CarlaPluginCategory>(nativePlugin->category);
     info.hints    = 0x0;

    if (nativePlugin->hints & PLUGIN_IS_RTSAFE)
         info.hints |= CarlaBackend::PLUGIN_IS_RTSAFE;
     if (nativePlugin->hints & PLUGIN_IS_SYNTH)
         info.hints |= CarlaBackend::PLUGIN_IS_SYNTH;
     if (nativePlugin->hints & PLUGIN_HAS_GUI)
         info.hints |= CarlaBackend::PLUGIN_HAS_GUI;
     if (nativePlugin->hints & PLUGIN_USES_SINGLE_THREAD)
         info.hints |= CarlaBackend::PLUGIN_HAS_SINGLE_THREAD;

     info.audioIns  = nativePlugin->audioIns;
     info.audioOuts = nativePlugin->audioOuts;
     info.midiIns   = nativePlugin->midiIns;
     info.midiOuts  = nativePlugin->midiOuts;
     info.parameterIns  = nativePlugin->parameterIns;
     info.parameterOuts = nativePlugin->parameterOuts;

     info.name      = nativePlugin->name;
     info.label     = nativePlugin->label;
     info.maker     = nativePlugin->maker;
     info.copyright = nativePlugin->copyright;
#endif

    return &info;

#ifndef WANT_NATIVE
    // unused
    (void)internalPluginId;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_engine_init(const char* driverName, const char* clientName)
{
    carla_debug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);
    CARLA_ASSERT(standalone.engine == nullptr);
    CARLA_ASSERT(driverName != nullptr);
    CARLA_ASSERT(clientName != nullptr);

    if (standalone.engine != nullptr)
    {
        standalone.lastError = "Engine is already running";
        return false;
    }

    standalone.engine = CarlaEngine::newDriverByName(driverName);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "The seleted audio driver is not available!";
        return false;
    }

#ifndef Q_OS_WIN
    // TODO: make this an option, put somewhere else
    if (getenv("WINE_RT") == nullptr)
    {
        carla_setenv("WINE_RT", "15");
        carla_setenv("WINE_SVR_RT", "10");
    }
#endif

    if (standalone.callback != nullptr)
        standalone.engine->setCallback(standalone.callback, nullptr);

#ifndef BUILD_BRIDGE
    standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_MODE,               static_cast<int>(standalone.options.processMode),   nullptr);
# if 0 // disabled for now
    standalone.engine->setOption(CarlaBackend::OPTION_TRANSPORT_MODE,             static_cast<int>(standalone.options.transportMode), nullptr);
# endif
    standalone.engine->setOption(CarlaBackend::OPTION_FORCE_STEREO,               standalone.options.forceStereo ? 1 : 0,             nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES,      standalone.options.preferPluginBridges ? 1 : 0,     nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_UI_BRIDGES,          standalone.options.preferUiBridges ? 1 : 0,         nullptr);
# ifdef WANT_DSSI
    standalone.engine->setOption(CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS,        standalone.options.useDssiVstChunks ? 1 : 0,        nullptr);
# endif
    standalone.engine->setOption(CarlaBackend::OPTION_MAX_PARAMETERS,             static_cast<int>(standalone.options.maxParameters),       nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE,      static_cast<int>(standalone.options.preferredBufferSize), nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE,      static_cast<int>(standalone.options.preferredSampleRate), nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_OSC_UI_TIMEOUT,             static_cast<int>(standalone.options.oscUiTimeout),        nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_NATIVE,      0, (const char*)standalone.options.bridge_native);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX32,     0, (const char*)standalone.options.bridge_posix32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX64,     0, (const char*)standalone.options.bridge_posix64);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN32,       0, (const char*)standalone.options.bridge_win32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN64,       0, (const char*)standalone.options.bridge_win64);
# ifdef WANT_LV2
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2,    0, (const char*)standalone.options.bridge_lv2Gtk2);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3,    0, (const char*)standalone.options.bridge_lv2Gtk3);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4,     0, (const char*)standalone.options.bridge_lv2Qt4);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5,     0, (const char*)standalone.options.bridge_lv2Qt5);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA,   0, (const char*)standalone.options.bridge_lv2Cocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, (const char*)standalone.options.bridge_lv2Win);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11,     0, (const char*)standalone.options.bridge_lv2X11);
# endif
# ifdef WANT_VST
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA,   0, (const char*)standalone.options.bridge_vstCocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND,    0, (const char*)standalone.options.bridge_vstHWND);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_X11,     0, (const char*)standalone.options.bridge_vstX11);
# endif

    if (standalone.procName.isNotEmpty())
        standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_NAME,        0, (const char*)standalone.procName);
#endif

    if (standalone.engine->init(clientName))
    {
        standalone.lastError = "no error";
        standalone.init();
        return true;
    }
    else
    {
        standalone.lastError = standalone.engine->getLastError();
        delete standalone.engine;
        standalone.engine = nullptr;
        return false;
    }
}

bool carla_engine_close()
{
    carla_debug("carla_engine_close()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "Engine is not started";
        return false;
    }

    standalone.engine->setAboutToClose();
    standalone.engine->removeAllPlugins();

    const bool closed(standalone.engine->close());

    if (! closed)
        standalone.lastError = standalone.engine->getLastError();

    delete standalone.engine;
    standalone.engine = nullptr;
    standalone.close();

    return closed;
}

void carla_engine_idle()
{
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.needsInit && standalone.app != nullptr)
        standalone.app->processEvents();

    if (standalone.engine != nullptr)
        standalone.engine->idle();
}

bool carla_is_engine_running()
{
    carla_debug("carla_is_engine_running()");

    return (standalone.engine != nullptr && standalone.engine->isRunning());
}

void carla_set_engine_about_to_close()
{
    carla_debug("carla_set_engine_about_to_close()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        standalone.engine->setAboutToClose();
}

void carla_set_engine_callback(CarlaCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p)", func);

    standalone.callback    = func;
    standalone.callbackPtr = ptr;

    if (standalone.engine != nullptr)
        standalone.engine->setCallback(func, ptr);
}

void carla_set_engine_option(CarlaOptionsType option, int value, const char* valueStr)
{
    carla_debug("carla_set_engine_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2Str(option), value, valueStr);

    switch (option)
    {
    case CarlaBackend::OPTION_PROCESS_NAME:
        standalone.procName = valueStr;
        break;

    case CarlaBackend::OPTION_PROCESS_MODE:
        if (value < CarlaBackend::PROCESS_MODE_SINGLE_CLIENT || value > CarlaBackend::PROCESS_MODE_PATCHBAY)
            return carla_stderr2("carla_set_engine_option(OPTION_PROCESS_MODE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.processMode = static_cast<CarlaBackend::ProcessMode>(value);
        break;

    case CarlaBackend::OPTION_TRANSPORT_MODE:
#if 0 // disabled for now
        if (value < CarlaBackend::TRANSPORT_MODE_INTERNAL || value > CarlaBackend::TRANSPORT_MODE_JACK)
            return carla_stderr2("carla_set_engine_option(OPTION_TRANSPORT_MODE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.transportMode = static_cast<CarlaBackend::TransportMode>(value);
#endif
        break;

    case CarlaBackend::OPTION_FORCE_STEREO:
        standalone.options.forceStereo = (value != 0);
        break;

    case CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES:
        standalone.options.preferPluginBridges = (value != 0);
        break;

    case CarlaBackend::OPTION_PREFER_UI_BRIDGES:
        standalone.options.preferUiBridges = (value != 0);
        break;

#ifdef WANT_DSSI
    case CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS:
        standalone.options.useDssiVstChunks = (value != 0);
        break;
#endif

    case CarlaBackend::OPTION_MAX_PARAMETERS:
        if (value <= 0)
            return carla_stderr2("carla_set_engine_option(OPTION_MAX_PARAMETERS, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.maxParameters = static_cast<unsigned int>(value);
        break;

    case CarlaBackend::OPTION_OSC_UI_TIMEOUT:
        if (value <= 0)
            return carla_stderr2("carla_set_engine_option(OPTION_OSC_UI_TIMEOUT, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.oscUiTimeout = static_cast<unsigned int>(value);
        break;

    case CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE:
        if (value <= 0)
            return carla_stderr2("carla_set_engine_option(OPTION_PREFERRED_BUFFER_SIZE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.preferredBufferSize = static_cast<unsigned int>(value);
        break;

    case CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE:
        if (value <= 0)
            return carla_stderr2("carla_set_engine_option(OPTION_PREFERRED_SAMPLE_RATE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.preferredSampleRate = static_cast<unsigned int>(value);
        break;

#ifndef BUILD_BRIDGE
    case CarlaBackend::OPTION_PATH_BRIDGE_NATIVE:
        standalone.options.bridge_native = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_POSIX32:
        standalone.options.bridge_posix32 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_POSIX64:
        standalone.options.bridge_posix64 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_WIN32:
        standalone.options.bridge_win32 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_WIN64:
        standalone.options.bridge_win64 = valueStr;
        break;
#endif
#ifdef WANT_LV2
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2:
        standalone.options.bridge_lv2Gtk2 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3:
        standalone.options.bridge_lv2Gtk3 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4:
        standalone.options.bridge_lv2Qt4 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5:
        standalone.options.bridge_lv2Qt5 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA:
        standalone.options.bridge_lv2Cocoa = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS:
        standalone.options.bridge_lv2Win = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11:
        standalone.options.bridge_lv2X11 = valueStr;
        break;
#endif
#ifdef WANT_VST
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA:
        standalone.options.bridge_vstCocoa = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND:
        standalone.options.bridge_vstHWND = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_X11:
        standalone.options.bridge_vstX11 = valueStr;
        break;
#endif
    }

    if (standalone.engine != nullptr)
        standalone.engine->setOption(option, value, valueStr);
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_filename(const char* filename)
{
    carla_debug("carla_load_filename(\"%s\")", filename);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->loadFilename(filename);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_load_project(const char* filename)
{
    carla_debug("carla_load_project(\"%s\")", filename);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->loadProject(filename);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_save_project(const char* filename)
{
    carla_debug("carla_save_project(\"%s\")", filename);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);

    if (standalone.engine != nullptr) // allow to save even if engine stopped
        return standalone.engine->saveProject(filename);

    standalone.lastError = "Engine is not started";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(int portA, int portB)
{
    carla_debug("carla_patchbay_connect(%i, %i)", portA, portB);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->patchbayConnect(portA, portB);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_patchbay_disconnect(int connectionId)
{
    carla_debug("carla_patchbay_disconnect(%i)", connectionId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->patchbayDisconnect(connectionId);

    standalone.lastError = "Engine is not started";
    return false;
}

void carla_patchbay_refresh()
{
    carla_debug("carla_patchbay_refresh()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->patchbayRefresh();
}

// -------------------------------------------------------------------------------------------------------------------

void carla_transport_play()
{
    carla_debug("carla_transport_play()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->transportPlay();
}

void carla_transport_pause()
{
    carla_debug("carla_transport_pause()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->transportPause();
}

void carla_transport_relocate(uint32_t frames)
{
    carla_debug("carla_transport_relocate(%i)", frames);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->transportRelocate(frames);
}

uint32_t carla_get_current_transport_frame()
{
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
    {
        const EngineTimeInfo& timeInfo(standalone.engine->getTimeInfo());
        return timeInfo.frame;
    }

    return 0;
}

const CarlaTransportInfo* carla_get_transport_info()
{
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaTransportInfo info;

    if (standalone.engine != nullptr)
    {
        const EngineTimeInfo& timeInfo(standalone.engine->getTimeInfo());

        info.playing = timeInfo.playing;
        info.frame   = timeInfo.frame;

        if (timeInfo.valid & timeInfo.ValidBBT)
        {
            info.bar  = timeInfo.bbt.bar;
            info.beat = timeInfo.bbt.beat;
            info.tick = timeInfo.bbt.tick;
            info.bpm  = timeInfo.bbt.beatsPerMinute;
        }
        else
        {
            info.bar  = 0;
            info.beat = 0;
            info.tick = 0;
            info.bpm  = 0.0;
        }
    }
    else
    {
        info.playing = false;
        info.frame   = 0;
        info.bar     = 0;
        info.beat    = 0;
        info.tick    = 0;
        info.bpm     = 0.0;
    }

    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(CarlaBinaryType btype, CarlaPluginType ptype, const char* filename, const char* const name, const char* label, const void* extraStuff)
{
    carla_debug("carla_add_plugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", CarlaBackend::BinaryType2Str(btype), CarlaBackend::PluginType2Str(ptype), filename, name, label, extraStuff);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->addPlugin(btype, ptype, filename, name, label, extraStuff);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_remove_plugin(unsigned int pluginId)
{
    carla_debug("carla_remove_plugin(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->removePlugin(pluginId);

    standalone.lastError = "Engine is not started";
    return false;
}

void carla_remove_all_plugins()
{
    carla_debug("carla_remove_all_plugins()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->removeAllPlugins();
}

const char* carla_rename_plugin(unsigned int pluginId, const char* newName)
{
    carla_debug("carla_rename_plugin(%i, \"%s\")", pluginId, newName);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->renamePlugin(pluginId, newName);

    standalone.lastError = "Engine is not started";
    return nullptr;
}

bool carla_clone_plugin(unsigned int pluginId)
{
    carla_debug("carla_clone_plugin(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->clonePlugin(pluginId);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_replace_plugin(unsigned int pluginId)
{
    carla_debug("carla_replace_plugin(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->replacePlugin(pluginId);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_switch_plugins(unsigned int pluginIdA, unsigned int pluginIdB)
{
    carla_debug("carla_switch_plugins(%i, %i)", pluginIdA, pluginIdB);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->switchPlugins(pluginIdA, pluginIdB);

    standalone.lastError = "Engine is not started";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_plugin_state(unsigned int pluginId, const char* filename)
{
    carla_debug("carla_load_plugin_state(%i, \"%s\")", pluginId, filename);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr || ! standalone.engine->isRunning())
    {
        standalone.lastError = "Engine is not started";
        return false;
    }

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->loadStateFromFile(filename);

    carla_stderr2("carla_load_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

bool carla_save_plugin_state(unsigned int pluginId, const char* filename)
{
    carla_debug("carla_save_plugin_state(%i, \"%s\")", pluginId, filename);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "Engine is not started";
        return false;
    }

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->saveStateToFile(filename);

    carla_stderr2("carla_save_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(unsigned int pluginId)
{
    carla_debug("carla_get_plugin_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaPluginInfo info;

    // reset
    info.type     = CarlaBackend::PLUGIN_NONE;
    info.category = CarlaBackend::PLUGIN_CATEGORY_NONE;
    info.hints    = 0x0;
    info.hints    = 0x0;
    info.binary   = nullptr;
    info.name     = nullptr;
    info.uniqueId = 0;
    info.latency  = 0;
    info.optionsAvailable = 0x0;
    info.optionsEnabled   = 0x0;

    // cleanup
    if (info.label != nullptr)
    {
        delete[] info.label;
        info.label = nullptr;
    }

    if (info.maker != nullptr)
    {
        delete[] info.maker;
        info.maker = nullptr;
    }

    if (info.copyright != nullptr)
    {
        delete[] info.copyright;
        info.copyright = nullptr;
    }

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        char strBufLabel[STR_MAX+1] = { '\0' };
        char strBufMaker[STR_MAX+1] = { '\0' };
        char strBufCopyright[STR_MAX+1] = { '\0' };

        info.type     = plugin->type();
        info.category = plugin->category();
        info.hints    = plugin->hints();
        info.binary   = plugin->filename();
        info.name     = plugin->name();
        info.uniqueId = plugin->uniqueId();
        info.latency  = plugin->latency();

        info.optionsAvailable = plugin->availableOptions();
        info.optionsEnabled   = plugin->options();

        plugin->getLabel(strBufLabel);
        info.label = carla_strdup(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = carla_strdup(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = carla_strdup(strBufCopyright);

        return &info;
    }

    carla_stderr2("carla_get_plugin_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(unsigned int pluginId)
{
    carla_debug("carla_get_audio_port_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->audioInCount();
        info.outs  = plugin->audioOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    carla_stderr2("carla_get_audio_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(unsigned int pluginId)
{
    carla_debug("carla_get_midi_port_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->midiInCount();
        info.outs  = plugin->midiOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    carla_stderr2("carla_get_midi_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(unsigned int pluginId)
{
    carla_debug("carla_get_parameter_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        plugin->getParameterCountInfo(&info.ins, &info.outs, &info.total);
        return &info;
    }

    carla_stderr2("carla_get_parameter_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaParameterInfo* carla_get_parameter_info(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_info(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
    if (info.name != nullptr)
    {
        delete[] info.name;
        info.name = nullptr;
    }

    if (info.symbol != nullptr)
    {
        delete[] info.symbol;
        info.symbol = nullptr;
    }

    if (info.unit != nullptr)
    {
        delete[] info.unit;
        info.unit = nullptr;
    }

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
        {
            char strBufName[STR_MAX+1] = { '\0' };
            char strBufSymbol[STR_MAX+1] = { '\0' };
            char strBufUnit[STR_MAX+1] = { '\0' };

            info.scalePointCount = plugin->parameterScalePointCount(parameterId);

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

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(unsigned int pluginId, uint32_t parameterId, uint32_t scalePointId)
{
    carla_debug("carla_get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameterId, scalePointId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaScalePointInfo info;

    // reset
    info.value = 0.0f;

    // cleanup
    if (info.label != nullptr)
    {
        delete[] info.label;
        info.label = nullptr;
    }

    if (standalone.engine == nullptr)
        return &info;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
        {
            if (scalePointId < plugin->parameterScalePointCount(parameterId))
            {
                char strBufLabel[STR_MAX+1] = { '\0' };

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

const CarlaParameterData* carla_get_parameter_data(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaParameterData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return &plugin->parameterData(parameterId);

        carla_stderr2("carla_get_parameter_data(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &data;
    }

    carla_stderr2("carla_get_parameter_data(%i, %i) - could not find plugin", pluginId, parameterId);
    return &data;
}

const CarlaParameterRanges* carla_get_parameter_ranges(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaParameterRanges ranges;

    if (standalone.engine == nullptr)
        return &ranges;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return &plugin->parameterRanges(parameterId);

        carla_stderr2("carla_get_parameter_ranges(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &ranges;
    }

    carla_stderr2("carla_get_parameter_ranges(%i, %i) - could not find plugin", pluginId, parameterId);
    return &ranges;
}

const CarlaMidiProgramData* carla_get_midi_program_data(unsigned int pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaMidiProgramData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->midiProgramCount())
            return &plugin->midiProgramData(midiProgramId);

        carla_stderr2("carla_get_midi_program_data(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return &data;
    }

    carla_stderr2("carla_get_midi_program_data(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return &data;
}

const CarlaCustomData* carla_get_custom_data(unsigned int pluginId, uint32_t customDataId)
{
    carla_debug("carla_get_custom_data(%i, %i)", pluginId, customDataId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaCustomData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (customDataId < plugin->customDataCount())
            return &plugin->customData(customDataId);

        carla_stderr2("carla_get_custom_data(%i, %i) - customDataId out of bounds", pluginId, customDataId);
        return &data;
    }

    carla_stderr2("carla_get_custom_data(%i, %i) - could not find plugin", pluginId, customDataId);
    return &data;
}

const char* carla_get_chunk_data(unsigned int pluginId)
{
    carla_debug("carla_get_chunk_data(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static CarlaString chunkData;

    // cleanup
    chunkData.clear();

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (plugin->options() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
        {
            void* data = nullptr;
            const int32_t dataSize = plugin->chunkData(&data);

            if (data != nullptr && dataSize > 0)
            {
                QByteArray chunk(QByteArray((char*)data, dataSize).toBase64());
                chunkData = chunk.constData();
                return (const char*)chunkData;
            }
            else
                carla_stderr2("carla_get_chunk_data(%i) - got invalid chunk data", pluginId);
        }
        else
            carla_stderr2("carla_get_chunk_data(%i) - plugin does not support chunks", pluginId);

        return nullptr;
    }

    carla_stderr2("carla_get_chunk_data(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(unsigned int pluginId)
{
    carla_debug("carla_get_parameter_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->parameterCount();

    carla_stderr2("carla_get_parameter_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_program_count(unsigned int pluginId)
{
    carla_debug("carla_get_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->programCount();

    carla_stderr2("carla_get_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_midi_program_count(unsigned int pluginId)
{
    carla_debug("carla_get_midi_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->midiProgramCount();

    carla_stderr2("carla_get_midi_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_custom_data_count(unsigned int pluginId)
{
    carla_debug("carla_get_custom_data_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->customDataCount();

    carla_stderr2("carla_get_custom_data_count(%i) - could not find plugin", pluginId);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_text(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static char textBuf[STR_MAX+1];
    carla_fill<char>(textBuf, STR_MAX+1, '\0');

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
        {
            plugin->getParameterText(parameterId, textBuf);
            return textBuf;
        }

        carla_stderr2("carla_get_parameter_text(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return nullptr;
    }

    carla_stderr2("carla_get_parameter_text(%i, %i) - could not find plugin", pluginId, parameterId);
    return nullptr;
}

const char* carla_get_program_name(unsigned int pluginId, uint32_t programId)
{
    carla_debug("carla_get_program_name(%i, %i)", pluginId, programId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static char programName[STR_MAX+1];
    carla_fill<char>(programName, STR_MAX+1, '\0');

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->programCount())
        {
            plugin->getProgramName(programId, programName);
            return programName;
        }

        carla_stderr2("carla_get_program_name(%i, %i) - programId out of bounds", pluginId, programId);
        return nullptr;
    }

    carla_stderr2("carla_get_program_name(%i, %i) - could not find plugin", pluginId, programId);
    return nullptr;
}

const char* carla_get_midi_program_name(unsigned int pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_get_midi_program_name(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static char midiProgramName[STR_MAX+1];
    carla_fill<char>(midiProgramName, STR_MAX+1, '\0');

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->midiProgramCount())
        {
            plugin->getMidiProgramName(midiProgramId, midiProgramName);
            return midiProgramName;
        }

        carla_stderr2("carla_get_midi_program_name(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return nullptr;
    }

    carla_stderr2("carla_get_midi_program_name(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return nullptr;
}

const char* carla_get_real_plugin_name(unsigned int pluginId)
{
    carla_debug("carla_get_real_plugin_name(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static char realPluginName[STR_MAX+1];
    carla_fill<char>(realPluginName, STR_MAX+1, '\0');

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        plugin->getRealName(realPluginName);
        return realPluginName;
    }

    carla_stderr2("carla_get_real_plugin_name(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(unsigned int pluginId)
{
    carla_debug("carla_get_current_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return -1;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->currentProgram();

    carla_stderr2("carla_get_current_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

int32_t carla_get_current_midi_program_index(unsigned int pluginId)
{
    carla_debug("carla_get_current_midi_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return -1;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->currentMidiProgram();

    carla_stderr2("carla_get_current_midi_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_default_parameter_value(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0f;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->parameterRanges(parameterId).def;

        carla_stderr2("carla_get_default_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_default_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_current_parameter_value(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_current_parameter_value(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0f;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->getParameterValue(parameterId);

        carla_stderr2("carla_get_current_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_current_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_input_peak_value(unsigned int pluginId, unsigned short portId)
{
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (standalone.engine == nullptr)
        return 0.0f;

    return standalone.engine->getInputPeak(pluginId, portId);
}

float carla_get_output_peak_value(unsigned int pluginId, unsigned short portId)
{
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (standalone.engine == nullptr)
        return 0.0f;

    return standalone.engine->getOutputPeak(pluginId, portId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_option(unsigned int pluginId, unsigned int option, bool yesNo)
{
    carla_debug("carla_set_option(%i, %i, %s)", pluginId, option, bool2str(yesNo));
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setOption(option, yesNo);

    carla_stderr2("carla_set_option(%i, %i, %s) - could not find plugin", pluginId, option, bool2str(yesNo));
}

void carla_set_active(unsigned int pluginId, bool onOff)
{
    carla_debug("carla_set_active(%i, %s)", pluginId, bool2str(onOff));
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setActive(onOff, true, false);

    carla_stderr2("carla_set_active(%i, %s) - could not find plugin", pluginId, bool2str(onOff));
}

void carla_set_drywet(unsigned int pluginId, float value)
{
    carla_debug("carla_set_drywet(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setDryWet(value, true, false);

    carla_stderr2("carla_set_drywet(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_volume(unsigned int pluginId, float value)
{
    carla_debug("carla_set_volume(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setVolume(value, true, false);

    carla_stderr2("carla_set_volume(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_left(unsigned int pluginId, float value)
{
    carla_debug("carla_set_balance_left(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setBalanceLeft(value, true, false);

    carla_stderr2("carla_set_balance_left(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_right(unsigned int pluginId, float value)
{
    carla_debug("carla_set_balance_right(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setBalanceRight(value, true, false);

    carla_stderr2("carla_set_balance_right(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_panning(unsigned int pluginId, float value)
{
    carla_debug("carla_set_panning(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setPanning(value, true, false);

    carla_stderr2("carla_set_panning(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_ctrl_channel(unsigned int pluginId, int8_t channel)
{
    carla_debug("carla_set_ctrl_channel(%i, %i)", pluginId, channel);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setCtrlChannel(channel, true, false);

    carla_stderr2("carla_set_ctrl_channel(%i, %i) - could not find plugin", pluginId, channel);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(unsigned int pluginId, uint32_t parameterId, float value)
{
    carla_debug("carla_set_parameter_value(%i, %i, %f)", pluginId, parameterId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterValue(parameterId, value, true, true, false);

        carla_stderr2("carla_set_parameter_value(%i, %i, %f) - parameterId out of bounds", pluginId, parameterId, value);
        return;
    }

    carla_stderr2("carla_set_parameter_value(%i, %i, %f) - could not find plugin", pluginId, parameterId, value);
}

void carla_set_parameter_midi_channel(unsigned int pluginId, uint32_t parameterId, uint8_t channel)
{
    carla_debug("carla_set_parameter_midi_channel(%i, %i, %i)", pluginId, parameterId, channel);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
    {
        carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - invalid channel number", pluginId, parameterId, channel);
        return;
    }

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterMidiChannel(parameterId, channel, true, false);

        carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, channel);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - could not find plugin", pluginId, parameterId, channel);
}

void carla_set_parameter_midi_cc(unsigned int pluginId, uint32_t parameterId, int16_t cc)
{
    carla_debug("carla_set_parameter_midi_cc(%i, %i, %i)", pluginId, parameterId, cc);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(cc >= -1 && cc <= 0x5F);

    if (cc < -1)
    {
        cc = -1;
    }
    else if (cc > 0x5F) // 95
    {
        carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - invalid cc number", pluginId, parameterId, cc);
        return;
    }

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterMidiCC(parameterId, cc, true, false);

        carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, cc);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - could not find plugin", pluginId, parameterId, cc);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_program(unsigned int pluginId, uint32_t programId)
{
    carla_debug("carla_set_program(%i, %i)", pluginId, programId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->programCount())
            return plugin->setProgram(static_cast<int32_t>(programId), true, true, false);

        carla_stderr2("carla_set_program(%i, %i) - programId out of bounds", pluginId, programId);
        return;
    }

    carla_stderr2("carla_set_program(%i, %i) - could not find plugin", pluginId, programId);
}

void carla_set_midi_program(unsigned int pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_set_midi_program(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->midiProgramCount())
            return plugin->setMidiProgram(static_cast<int32_t>(midiProgramId), true, true, false);

        carla_stderr2("carla_set_midi_program(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return;
    }

    carla_stderr2("carla_set_midi_program(%i, %i) - could not find plugin", pluginId, midiProgramId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(unsigned int pluginId, const char* type, const char* key, const char* value)
{
    carla_debug("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setCustomData(type, key, value, true);

    carla_stderr2("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\") - could not find plugin", pluginId, type, key, value);
}

void carla_set_chunk_data(unsigned int pluginId, const char* chunkData)
{
    carla_debug("carla_set_chunk_data(%i, \"%s\")", pluginId, chunkData);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (plugin->options() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
            return plugin->setChunkData(chunkData);

        carla_stderr2("carla_set_chunk_data(%i, \"%s\") - plugin does not support chunks", pluginId, chunkData);
        return;
    }

    carla_stderr2("carla_set_chunk_data(%i, \"%s\") - could not find plugin", pluginId, chunkData);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(unsigned int pluginId)
{
    carla_debug("carla_prepare_for_save(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->prepareForSave();

    carla_stderr2("carla_prepare_for_save(%i) - could not find plugin", pluginId);
}

void carla_send_midi_note(unsigned int pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    carla_debug("carla_send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr || ! standalone.engine->isRunning())
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    carla_stderr2("carla_send_midi_note(%i, %i, %i, %i) - could not find plugin", pluginId, channel, note, velocity);
}

void carla_show_gui(unsigned int pluginId, bool yesno)
{
    carla_debug("carla_show_gui(%i, %s)", pluginId, bool2str(yesno));
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->showGui(yesno);

    carla_stderr2("carla_show_gui(%i, %s) - could not find plugin", pluginId, bool2str(yesno));
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_buffer_size()
{
    carla_debug("carla_get_buffer_size()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    return standalone.engine->getBufferSize();
}

double carla_get_sample_rate()
{
    carla_debug("carla_get_sample_rate()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0;

    return standalone.engine->getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_last_error()
{
    carla_debug("carla_get_last_error()");

    if (standalone.engine != nullptr)
        return standalone.engine->getLastError();

    return standalone.lastError;
}

const char* carla_get_host_osc_url()
{
    carla_debug("carla_get_host_osc_url()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "Engine is not started";
        return nullptr;
    }

    return standalone.engine->getOscServerPathTCP();
}

// -------------------------------------------------------------------------------------------------------------------

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 2

class CarlaNSM
{
public:
    CarlaNSM()
        : fServerThread(nullptr),
          fReplyAddr(nullptr),
          fIsOpened(false),
          fIsSaved(false)
    {
    }

    ~CarlaNSM()
    {
        if (fReplyAddr != nullptr)
            lo_address_free(fReplyAddr);

        if (fServerThread != nullptr)
        {
            lo_server_thread_stop(fServerThread);
            lo_server_thread_del_method(fServerThread, "/reply", "ssss");
            lo_server_thread_del_method(fServerThread, "/nsm/client/open", "sss");
            lo_server_thread_del_method(fServerThread, "/nsm/client/save", "");
            lo_server_thread_free(fServerThread);
        }
    }

    void announce(const char* const url, const int pid)
    {
        lo_address const addr = lo_address_new_from_url(url);

        if (addr == nullptr)
            return;

        const int proto = lo_address_get_protocol(addr);

        if (fServerThread == nullptr)
        {
            // create new OSC thread
            fServerThread = lo_server_thread_new_with_proto(nullptr, proto, error_handler);

            // register message handlers and start OSC thread
            lo_server_thread_add_method(fServerThread, "/reply",           "ssss", _reply_handler, this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/open", "sss",  _open_handler,  this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/save", "",     _save_handler,  this);
            lo_server_thread_start(fServerThread);
        }

        lo_send_from(addr, lo_server_thread_get_server(fServerThread), LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                    "Carla", ":switch:", "carla", NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

        lo_address_free(addr);
    }

    void replyOpen()
    {
        fIsOpened = true;
    }

    void replySave()
    {
        fIsSaved = true;
    }

protected:
    int handleReply(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::handleReply(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        if (fReplyAddr != nullptr)
            lo_address_free(fReplyAddr);

        fIsOpened = false;
        fIsSaved  = false;

        char* const url = lo_address_get_url(lo_message_get_source(msg));
        fReplyAddr      = lo_address_new_from_url(url);
        std::free(url);

        const char* const method = &argv[0]->s;
        const char* const smName = &argv[2]->s;

        if (std::strcmp(method, "/nsm/server/announce") == 0 && standalone.callback != nullptr)
            standalone.callback(standalone.callbackPtr, CarlaBackend::CALLBACK_NSM_ANNOUNCE, 0, 0, 0, 0.0f, smName);

        return 0;

#ifndef DEBUG
        // unused
        (void)path;
        (void)types;
        (void)argc;
#endif
    }

    int handleOpen(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::handleOpen(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        if (standalone.callback == nullptr)
            return 1;
        if (fServerThread == nullptr)
            return 1;
        if (fReplyAddr == nullptr)
            return 1;

        const char* const projectPath = &argv[0]->s;
        const char* const clientId    = &argv[2]->s;

        char data[std::strlen(projectPath)+std::strlen(clientId)+2];
        std::strcpy(data, projectPath);
        std::strcat(data, ":");
        std::strcat(data, clientId);

        standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_OPEN, 0, 0, 0, 0.0f, data);

        for (int i=0; i < 30 && ! fIsOpened; i++)
            carla_msleep(100);

        if (fIsOpened)
            lo_send_from(fReplyAddr, lo_server_thread_get_server(fServerThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");

        return 0;

#ifndef DEBUG
        // unused
        (void)path;
        (void)types;
        (void)argc;
        (void)msg;
#endif
    }

    int handleSave(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::handleSave(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        if (standalone.callback == nullptr)
            return 1;
        if (fServerThread == nullptr)
            return 1;
        if (fReplyAddr == nullptr)
            return 1;

        standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_SAVE, 0, 0, 0, 0.0f, nullptr);

        for (int i=0; i < 30 && ! fIsSaved; i++)
            carla_msleep(100);

        if (fIsSaved)
            lo_send_from(fReplyAddr, lo_server_thread_get_server(fServerThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/save", "OK");

        return 0;

#ifndef DEBUG
        // unused
        (void)path;
        (void)types;
        (void)argv;
        (void)argc;
        (void)msg;
#endif
    }

private:
    lo_server_thread fServerThread;
    lo_address       fReplyAddr;

    bool fIsOpened;
    bool fIsSaved;

    #define handlePtr ((CarlaNSM*)data)

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

    #undef handlePtr

    static void error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr2("CarlaNSM::error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }
};

static CarlaNSM gCarlaNSM;

void carla_nsm_announce(const char* url, int pid)
{
    gCarlaNSM.announce(url, pid);
}

void carla_nsm_reply_open()
{
    gCarlaNSM.replyOpen();
}

void carla_nsm_reply_save()
{
    gCarlaNSM.replySave();
}

// -------------------------------------------------------------------------------------------------------------------

#ifdef BUILD_BRIDGE
CarlaEngine* carla_get_standalone_engine()
{
    return standalone.engine;
}

bool carla_engine_init_bridge(const char* audioBaseName, const char* controlBaseName, const char* clientName)
{
    carla_debug("carla_engine_init_bridge(\"%s\", \"%s\", \"%s\")", audioBaseName, controlBaseName, clientName);
    CARLA_ASSERT(standalone.engine == nullptr);
    CARLA_ASSERT(audioBaseName != nullptr);
    CARLA_ASSERT(controlBaseName != nullptr);
    CARLA_ASSERT(clientName != nullptr);

    standalone.engine = CarlaEngine::newBridge(audioBaseName, controlBaseName);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "The seleted audio driver is not available!";
        return false;
    }

    if (standalone.callback != nullptr)
        standalone.engine->setCallback(standalone.callback, nullptr);

    standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_MODE,          CarlaBackend::PROCESS_MODE_BRIDGE,   nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_TRANSPORT_MODE,        CarlaBackend::TRANSPORT_MODE_BRIDGE, nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES, false, nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_UI_BRIDGES,     false, nullptr);

    // TODO - read from environment
#ifndef BUILD_BRIDGE
    standalone.engine->setOption(CarlaBackend::OPTION_FORCE_STEREO,          standalone.options.forceStereo ? 1 : 0,             nullptr);
# ifdef WANT_DSSI
    standalone.engine->setOption(CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS,   standalone.options.useDssiVstChunks ? 1 : 0,        nullptr);
# endif
    standalone.engine->setOption(CarlaBackend::OPTION_MAX_PARAMETERS,        static_cast<int>(standalone.options.maxParameters),       nullptr);
#endif

    const bool initiated = standalone.engine->init(clientName);

    if (initiated)
    {
        standalone.lastError = "no error";
        standalone.init();
    }
    else
    {
        standalone.lastError = standalone.engine->getLastError();
        delete standalone.engine;
        standalone.engine = nullptr;
    }

    return initiated;
}
#endif
