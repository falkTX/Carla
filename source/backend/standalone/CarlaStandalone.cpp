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
#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaMIDI.h"
#include "CarlaNative.h"

#include <QtGui/QApplication>

#if 0
extern "C" {
#include "siginfo.c"
}
#endif

using CarlaBackend::CarlaEngine;
using CarlaBackend::CarlaPlugin;

using CarlaBackend::CallbackFunc;
using CarlaBackend::EngineOptions;

// -------------------------------------------------------------------------------------------------------------------

// Single, standalone engine
struct CarlaBackendStandalone {
    CallbackFunc  callback;
    void*         callbackPtr;
    CarlaEngine*  engine;
    CarlaString   lastError;
    CarlaString   procName;
    EngineOptions options;

    CarlaBackendStandalone()
        : callback(nullptr),
          callbackPtr(nullptr),
          engine(nullptr)
    {
        QApplication* app = qApp;

        if (app == nullptr)
        {
            static int    argc = 0;
            static char** argv = nullptr;
            app = new QApplication(argc, argv, true);
        }
    }

} standalone;

// -------------------------------------------------------------------------------------------------------------------
// Helpers

CarlaEngine* carla_get_standalone_engine()
{
    return standalone.engine;
}

// -------------------------------------------------------------------------------------------------------------------
// API

const char* carla_get_extended_license_text()
{
    carla_debug("carla_get_extended_license_text()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        retText  = "<p>This current Carla build is using the following features and 3rd-party code:</p>";
        retText += "<ul>";

#ifdef WANT_LADSPA
        retText += "<li>LADSPA plugin support, http://www.ladspa.org/</li>";
#endif
#ifdef WANT_DSSI
        retText += "<li>DSSI plugin support, http://dssi.sourceforge.net/</li>";
#endif
#ifdef WANT_LV2
        retText += "<li>LV2 plugin support, http://lv2plug.in/</li>";
#endif
#ifdef WANT_VST
# ifdef VESTIGE_HEADER
        retText += "<li>VST plugin support, using VeSTige header by Javier Serrano Polo</li>";
# else
        retText += "<li>VST plugin support, using official VST SDK 2.4 (trademark of Steinberg Media Technologies GmbH)</li>";
# endif
#endif
#ifdef WANT_AUDIOFILE
        // TODO
        //retText += "<li>ZynAddSubFX plugin code, http://zynaddsubfx.sf.net/</li>";
#endif
#ifdef WANT_ZYNADDSUBFX
        retText += "<li>ZynAddSubFX plugin code, http://zynaddsubfx.sf.net/</li>";
#endif
#ifdef WANT_FLUIDSYNTH
        retText += "<li>FluidSynth library for SF2 support, http://www.fluidsynth.org/</li>";
#endif
#ifdef WANT_LINUXSAMPLER
        retText += "<li>LinuxSampler library for GIG and SFZ support*, http://www.linuxsampler.org/</li>";
#endif
        retText += "<li>liblo library for OSC support, http://liblo.sourceforge.net/</li>";
#ifdef WANT_LV2
        retText += "<li>serd, sord, sratom and lilv libraries for LV2 discovery, http://drobilla.net/software/lilv/</li>";
#endif
#ifdef WANT_RTAUDIO
        retText += "<li>RtAudio and RtMidi libraries for extra Audio and MIDI support, http://www.music.mcgill.ca/~gary/rtaudio/</li>";
#endif
        retText += "</ul>";

#ifdef WANT_LINUXSAMPLER
        retText += "<p>(*) Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors.</p>";
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

// -------------------------------------------------------------------------------------------------------------------

unsigned int carla_get_internal_plugin_count()
{
    carla_debug("carla_get_internal_plugin_count()");

    return static_cast<unsigned int>(CarlaPlugin::getNativePluginCount());
}

const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int internalPluginId)
{
    carla_debug("carla_get_internal_plugin_info(%i)", internalPluginId);

    static CarlaNativePluginInfo info;

    const PluginDescriptor* const nativePlugin = CarlaPlugin::getNativePluginDescriptor(internalPluginId);

    // as internal plugin, this must never fail
    CARLA_ASSERT(nativePlugin != nullptr);

    if (nativePlugin == nullptr)
        return nullptr;

     info.category  = static_cast<CarlaPluginCategory>(nativePlugin->category);
     info.hints     = 0x0;

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

    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_engine_init(const char* driverName, const char* clientName)
{
    carla_debug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);
    CARLA_ASSERT(standalone.engine == nullptr);
    CARLA_ASSERT(driverName != nullptr);
    CARLA_ASSERT(clientName != nullptr);

#if 0
    static bool sigInfoInitiated = false;

    if (! sigInfoInitiated)
    {
        setup_siginfo();
        sigInfoInitiated = true;
    }
#endif

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
    standalone.engine->setOption(CarlaBackend::OPTION_TRANSPORT_MODE,             static_cast<int>(standalone.options.transportMode), nullptr);
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
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_NATIVE,      0, standalone.options.bridge_native);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX32,     0, standalone.options.bridge_posix32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX64,     0, standalone.options.bridge_posix64);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN32,       0, standalone.options.bridge_win32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN64,       0, standalone.options.bridge_win64);
# ifdef WANT_LV2
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2,    0, standalone.options.bridge_lv2gtk2);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3,    0, standalone.options.bridge_lv2gtk3);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4,     0, standalone.options.bridge_lv2qt4);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5,     0, standalone.options.bridge_lv2qt5);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA,   0, standalone.options.bridge_lv2cocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, standalone.options.bridge_lv2win);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11,     0, standalone.options.bridge_lv2qt4);
# endif
# ifdef WANT_VST
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA,   0, standalone.options.bridge_vstcocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND,    0, standalone.options.bridge_vsthwnd);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_X11,     0, standalone.options.bridge_vstx11);
# endif

    if (standalone.procName.isNotEmpty())
        standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_NAME, 0, standalone.procName);
#endif

    const bool initiated = standalone.engine->init(clientName);

    if (initiated)
    {
        standalone.lastError = "no error";
    }
    else
    {
        standalone.lastError = standalone.engine->getLastError();
        delete standalone.engine;
        standalone.engine = nullptr;
    }

    return initiated;
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

    const bool closed = standalone.engine->close();

    delete standalone.engine;
    standalone.engine = nullptr;

    return closed;
}

void carla_engine_idle()
{
    CARLA_ASSERT(standalone.engine != nullptr);

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

void carla_set_engine_callback(CarlaBackend::CallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p)", func);

    standalone.callback    = func;
    standalone.callbackPtr = ptr;

    if (standalone.engine != nullptr)
        standalone.engine->setCallback(func, ptr);
}

void carla_set_engine_option(CarlaBackend::OptionsType option, int value, const char* valueStr)
{
    carla_debug("carla_set_engine_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2Str(option), value, valueStr);

#ifndef BUILD_BRIDGE
    if (standalone.engine != nullptr)
        standalone.engine->setOption(option, value, valueStr);
#endif

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
        if (value < CarlaBackend::TRANSPORT_MODE_INTERNAL || value > CarlaBackend::TRANSPORT_MODE_JACK)
            return carla_stderr2("carla_set_engine_option(OPTION_TRANSPORT_MODE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.transportMode = static_cast<CarlaBackend::TransportMode>(value);
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
        standalone.options.maxParameters = (value > 0) ? static_cast<unsigned int>(value) : CarlaBackend::MAX_DEFAULT_PARAMETERS;
        break;

    case CarlaBackend::OPTION_OSC_UI_TIMEOUT:
        standalone.options.oscUiTimeout = static_cast<unsigned int>(value);
        break;

    case CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE:
        standalone.options.preferredBufferSize = static_cast<unsigned int>(value);
        break;

    case CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE:
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
        standalone.options.bridge_lv2gtk2 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3:
        standalone.options.bridge_lv2gtk3 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4:
        standalone.options.bridge_lv2qt4 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5:
        standalone.options.bridge_lv2qt5 = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA:
        standalone.options.bridge_lv2cocoa = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS:
        standalone.options.bridge_lv2win = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11:
        standalone.options.bridge_lv2x11 = valueStr;
        break;
#endif
#ifdef WANT_VST
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA:
        standalone.options.bridge_vstcocoa = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND:
        standalone.options.bridge_vsthwnd = valueStr;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_X11:
        standalone.options.bridge_vstx11 = valueStr;
        break;
#endif
    }
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_project(const char* filename)
{
    carla_debug("carla_load_project(\"%s\")", filename);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->loadProject(filename);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_save_project(const char* filename)
{
    carla_debug("carla_save_project(\"%s\")", filename);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->saveProject(filename);

    standalone.lastError = "Engine is not started";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

void carla_patchbay_connect(int portA, int portB)
{
    carla_debug("carla_patchbay_connect(%i, %i)", portA, portB);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->patchbayConnect(portA, portB);
}

void carla_patchbay_disconnect(int connectionId)
{
    carla_debug("carla_patchbay_disconnect(%i)", connectionId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->patchbayDisconnect(connectionId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_transport_play()
{
    carla_debug("carla_transport_play()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->transportPlay();
}

void carla_transport_pause()
{
    carla_debug("carla_transport_pause()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->transportPause();
}

void carla_transport_relocate(uint32_t frames)
{
    carla_debug("carla_transport_relocate(%i)", frames);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr)
        return standalone.engine->transportRelocate(frames);
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* const name, const char* label, const void* extraStuff)
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

bool carla_load_plugin_state(unsigned int pluginId, const char* filename)
{
    carla_debug("carla_load_plugin_state(%i, \"%s\")", pluginId, filename);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return false;

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
        return false;

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
        char strBufLabel[STR_MAX] = { 0 };
        char strBufMaker[STR_MAX] = { 0 };
        char strBufCopyright[STR_MAX] = { 0 };

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
            char strBufName[STR_MAX] = { 0 };
            char strBufSymbol[STR_MAX] = { 0 };
            char strBufUnit[STR_MAX] = { 0 };

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
                char strBufLabel[STR_MAX] = { 0 };

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

const CarlaBackend::ParameterData* carla_get_parameter_data(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::ParameterData data;

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

const CarlaBackend::ParameterRanges* carla_get_parameter_ranges(unsigned int pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::ParameterRanges ranges;

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

const CarlaBackend::MidiProgramData* carla_get_midi_program_data(unsigned int pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::MidiProgramData data;

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

const CarlaBackend::CustomData* carla_get_custom_data(unsigned int pluginId, uint32_t customDataId)
{
    carla_debug("carla_get_custom_data(%i, %i)", pluginId, customDataId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::CustomData data;

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

            if (data != nullptr && dataSize >= 4)
            {
                chunkData.importBinaryAsBase64((const uint8_t*)data, static_cast<size_t>(dataSize));
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

    static char textBuf[STR_MAX];
    carla_zeroMem(textBuf, sizeof(char)*STR_MAX);

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

    static char programName[STR_MAX];
    carla_zeroMem(programName, sizeof(char)*STR_MAX);

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

    static char midiProgramName[STR_MAX];
    carla_zeroMem(midiProgramName, sizeof(char)*STR_MAX);

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

    static char realPluginName[STR_MAX];
    carla_zeroMem(realPluginName, sizeof(char)*STR_MAX);

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

    if (pluginId >= standalone.engine->currentPluginCount())
    {
        carla_stderr2("carla_get_input_peak_value(%i, %i) - invalid plugin value", pluginId, portId);
        return 0.0f;
    }

    if (portId == 1 || portId == 2)
       return standalone.engine->getInputPeak(pluginId, portId);

    carla_stderr2("carla_get_input_peak_value(%i, %i) - invalid port value", pluginId, portId);
    return 0.0f;
}

float carla_get_output_peak_value(unsigned int pluginId, unsigned short portId)
{
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (standalone.engine == nullptr)
        return 0.0f;

    if (pluginId >= standalone.engine->currentPluginCount())
    {
        carla_stderr2("carla_get_input_peak_value(%i, %i) - invalid plugin value", pluginId, portId);
        return 0.0f;
    }

    if (portId == 1 || portId == 2)
       return standalone.engine->getOutputPeak(pluginId, portId);

    carla_stderr2("carla_get_output_peak_value(%i, %i) - invalid port value", pluginId, portId);
    return 0.0f;
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
        return nullptr;

    return standalone.engine->getOscServerPathTCP();
}

#if 0
// -------------------------------------------------------------------------------------------------------------------

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

class CarlaNSM
{
public:
    CarlaNSM()
    {
        m_controlAddr = nullptr;
        m_serverThread = nullptr;
        m_isOpened = false;
        m_isSaved  = false;
    }

    ~CarlaNSM()
    {
        if (m_controlAddr)
            lo_address_free(m_controlAddr);

        if (m_serverThread)
        {
            lo_server_thread_stop(m_serverThread);
            lo_server_thread_del_method(m_serverThread, "/reply", "ssss");
            lo_server_thread_del_method(m_serverThread, "/nsm/client/open", "sss");
            lo_server_thread_del_method(m_serverThread, "/nsm/client/save", "");
            lo_server_thread_free(m_serverThread);
        }
    }

    void announce(const char* const url, const int pid)
    {
        lo_address addr = lo_address_new_from_url(url);
        int proto = lo_address_get_protocol(addr);

        if (! m_serverThread)
        {
            // create new OSC thread
            m_serverThread = lo_server_thread_new_with_proto(nullptr, proto, error_handler);

            // register message handlers and start OSC thread
            lo_server_thread_add_method(m_serverThread, "/reply", "ssss", _reply_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/open", "sss", _nsm_open_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/save", "", _nsm_save_handler, this);
            lo_server_thread_start(m_serverThread);
        }

        lo_send_from(addr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     "Carla", ":switch:", "carla", NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

        lo_address_free(addr);
    }

    void replyOpen()
    {
        m_isOpened = true;
    }

    void replySave()
    {
        m_isSaved = true;
    }

protected:
    int reply_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::reply_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        m_controlAddr = lo_address_new_from_url(lo_address_get_url(lo_message_get_source(msg)));

        const char* const method = &argv[0]->s;

        if (std::strcmp(method, "/nsm/server/announce") == 0 && standalone.callback)
            standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_ANNOUNCE, 0, 0, 0, 0.0, nullptr); // FIXME?

        return 0;
    }

    int nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::nsm_open_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        if (! standalone.callback)
            return 1;

        const char* const projectPath = &argv[0]->s;
        const char* const clientId    = &argv[2]->s;

        standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_OPEN1, 0, 0, 0, 0.0, clientId);
        standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_OPEN2, 0, 0, 0, 0.0, projectPath);

        for (int i=0; i < 30 && ! m_isOpened; i++)
            carla_msleep(100);

        if (m_controlAddr)
            lo_send_from(m_controlAddr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");

        return 0;
    }

    int nsm_save_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        carla_debug("CarlaNSM::nsm_save_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

        if (! standalone.callback)
            return 1;

        standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_SAVE, 0, 0, 0, 0.0, nullptr);

        for (int i=0; i < 30 && ! m_isSaved; i++)
            carla_msleep(100);

        if (m_controlAddr)
            lo_send_from(m_controlAddr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/save", "OK");

        return 0;
    }

private:
    lo_address m_controlAddr;
    lo_server_thread m_serverThread;
    bool m_isOpened, m_isSaved;

    static int _reply_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->reply_handler(path, types, argv, argc, msg);
    }

    static int _nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_open_handler(path, types, argv, argc, msg);
    }

    static int _nsm_save_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_save_handler(path, types, argv, argc, msg);
    }

    static void error_handler(const int num, const char* const msg, const char* const path)
    {
        carla_stderr2("CarlaNSM::error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }
};

static CarlaNSM carlaNSM;

void carla_nsm_announce(const char* url, int pid)
{
    carlaNSM.announce(url, pid);
}

void carla_nsm_reply_open()
{
    carlaNSM.replyOpen();
}

void carla_nsm_reply_save()
{
    carlaNSM.replySave();
}

#endif

// -------------------------------------------------------------------------------------------------------------------

#if 0
//def QTCREATOR_TEST

#include <QtGui/QApplication>
#include <QtGui/QDialog>

QDialog* vstGui = nullptr;

void main_callback(void* ptr, CarlaBackend::CallbackType action, unsigned int pluginId, int value1, int value2, double value3)
{
    switch (action)
    {
    case CarlaBackend::CALLBACK_SHOW_GUI:
        if (vstGui && ! value1)
            vstGui->close();
        break;
    case CarlaBackend::CALLBACK_RESIZE_GUI:
        vstGui->setFixedSize(value1, value2);
        break;
    default:
        break;
    }

    Q_UNUSED(ptr);
    Q_UNUSED(pluginId);
    Q_UNUSED(value3);
}

void run_tests_standalone(short idMax)
{
    for (short id = 0; id <= idMax; id++)
    {
        carla_debug("------------------- TEST @%i: non-parameter calls --------------------", id);
        get_plugin_info(id);
        get_audio_port_count_info(id);
        get_midi_port_count_info(id);
        get_parameter_count_info(id);
        get_gui_info(id);
        get_chunk_data(id);
        get_parameter_count(id);
        get_program_count(id);
        get_midi_program_count(id);
        get_custom_data_count(id);
        get_real_plugin_name(id);
        get_current_program_index(id);
        get_current_midi_program_index(id);

        carla_debug("------------------- TEST @%i: parameter calls [-1] --------------------", id);
        get_parameter_info(id, -1);
        get_parameter_scalepoint_info(id, -1, -1);
        get_parameter_data(id, -1);
        get_parameter_ranges(id, -1);
        get_midi_program_data(id, -1);
        get_custom_data(id, -1);
        get_parameter_text(id, -1);
        get_program_name(id, -1);
        get_midi_program_name(id, -1);
        get_default_parameter_value(id, -1);
        get_current_parameter_value(id, -1);
        get_input_peak_value(id, -1);
        get_output_peak_value(id, -1);

        carla_debug("------------------- TEST @%i: parameter calls [0] --------------------", id);
        get_parameter_info(id, 0);
        get_parameter_scalepoint_info(id, 0, -1);
        get_parameter_scalepoint_info(id, 0, 0);
        get_parameter_data(id, 0);
        get_parameter_ranges(id, 0);
        get_midi_program_data(id, 0);
        get_custom_data(id, 0);
        get_parameter_text(id, 0);
        get_program_name(id, 0);
        get_midi_program_name(id, 0);
        get_default_parameter_value(id, 0);
        get_current_parameter_value(id, 0);
        get_input_peak_value(id, 0);
        get_input_peak_value(id, 1);
        get_input_peak_value(id, 2);
        get_output_peak_value(id, 0);
        get_output_peak_value(id, 1);
        get_output_peak_value(id, 2);

        carla_debug("------------------- TEST @%i: set extra data --------------------", id);
        set_custom_data(id, CarlaBackend::CUSTOM_DATA_STRING, "", "");
        set_chunk_data(id, nullptr);
        set_gui_container(id, (uintptr_t)1);

        carla_debug("------------------- TEST @%i: gui stuff --------------------", id);
        show_gui(id, false);
        show_gui(id, true);
        show_gui(id, true);

        idle_guis();
        idle_guis();
        idle_guis();

        carla_debug("------------------- TEST @%i: other --------------------", id);
        send_midi_note(id, 15,  127,  127);
        send_midi_note(id,  0,  0,  0);

        prepare_for_save(id);
        prepare_for_save(id);
        prepare_for_save(id);
    }
}

int main(int argc, char* argv[])
{
    using namespace CarlaBackend;

    // Qt app
    QApplication app(argc, argv);

    // Qt gui (for vst)
    vstGui = new QDialog(nullptr);

    // set callback and options
    set_callback_function(main_callback);
    set_option(OPTION_PREFER_UI_BRIDGES, 0, nullptr);
    //set_option(OPTION_PROCESS_MODE, PROCESS_MODE_CONTINUOUS_RACK, nullptr);

    // start engine
    if (! engine_init("JACK", "carla_demo"))
    {
        carla_stderr2("failed to start backend engine, reason:\n%s", get_last_error());
        delete vstGui;
        return 1;
    }

    short id_ladspa = add_plugin(BINARY_NATIVE, PLUGIN_LADSPA, "/usr/lib/ladspa/LEET_eqbw2x2.so", "LADSPA plug name, test long name - "
                                 "------- name ------------ name2 ----------- name3 ------------ name4 ------------ name5 ---------- name6", "leet_equalizer_bw2x2", nullptr);

    short id_dssi   = add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/fluidsynth-dssi.so", "DSSI pname, short-utf8 _ \xAE", "FluidSynth-DSSI", (void*)"/usr/lib/dssi/fluidsynth-dssi/FluidSynth-DSSI_gtk");
    short id_native = add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", "ZynHere", "zynaddsubfx", nullptr);

    //short id_lv2 = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    //short id_vst = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    if (id_ladspa < 0 || id_dssi < 0 || id_native < 0)
    {
        carla_stderr2("failed to start load plugins, reason:\n%s", get_last_error());
        delete vstGui;
        return 1;
    }

    //const GuiInfo* const guiInfo = get_gui_info(id);
    //if (guiInfo->type == CarlaBackend::GUI_INTERNAL_QT4 || guiInfo->type == CarlaBackend::GUI_INTERNAL_X11)
    //{
    //    set_gui_data(id, 0, (uintptr_t)gui);
    //gui->show();
    //}

    // activate
    set_active(id_ladspa, true);
    set_active(id_dssi, true);
    set_active(id_native, true);

    // start guis
    show_gui(id_dssi, true);
    carla_sleep(1);

    // do tests
    run_tests_standalone(id_dssi+1);

    // lock
    app.exec();

    delete vstGui;
    vstGui = nullptr;

    remove_plugin(id_ladspa);
    remove_plugin(id_dssi);
    remove_plugin(id_native);
    engine_close();

    return 0;
}

#endif
