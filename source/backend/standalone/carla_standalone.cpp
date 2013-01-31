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

#include "carla_standalone.hpp"
#include "carla_engine.hpp"
#include "carla_plugin.hpp"
#include "carla_native.h"

using CarlaBackend::CarlaEngine;
using CarlaBackend::CarlaPlugin;

using CarlaBackend::CallbackFunc;
using CarlaBackend::EngineOptions;

#if 0
int main(int argc, char* argv[])
{
    CARLA_BACKEND_USE_NAMESPACE
    //std::printf("%s\n", carla_get_extended_license_text());
    //std::printf("%i\n", carla_get_engine_driver_count());
    std::printf("%i\n", carla_get_internal_plugin_count());
    return 0;
}
#endif

// -------------------------------------------------------------------------------------------------------------------

// Single, standalone engine
struct CarlaBackendStandalone {
    CallbackFunc  callback;
    CarlaEngine*  engine;
    CarlaString   lastError;
    CarlaString   procName;
    EngineOptions options;
    bool          started;

    CarlaBackendStandalone()
        : callback(nullptr),
          engine(nullptr),
          started(false) {}
} standalone;

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_extended_license_text()
{
    qDebug("carla_get_extended_license_text()");

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
#ifdef CARLA_ENGINE_RTAUDIO
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
    qDebug("carla_get_engine_driver_count()");

    return CarlaEngine::getDriverCount();
}

const char* carla_get_engine_driver_name(unsigned int index)
{
    qDebug("carla_get_engine_driver_name(%i)", index);

    return CarlaEngine::getDriverName(index);
}

// -------------------------------------------------------------------------------------------------------------------

unsigned int carla_get_internal_plugin_count()
{
    qDebug("carla_get_internal_plugin_count()");

    return CarlaPlugin::getNativePluginCount();
}

const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int internalPluginId)
{
    qDebug("carla_get_internal_plugin_info(%i)", internalPluginId);

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
         info.hints |= CarlaBackend::PLUGIN_USES_SINGLE_THREAD;

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
    qDebug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);
    CARLA_ASSERT(standalone.engine == nullptr);
    CARLA_ASSERT(driverName != nullptr);
    CARLA_ASSERT(clientName != nullptr);

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

    standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_MODE,               standalone.options.processMode,          nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_FORCE_STEREO,               standalone.options.forceStereo,          nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES,      standalone.options.preferPluginBridges,  nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFER_UI_BRIDGES,          standalone.options.preferUiBridges,      nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS,        standalone.options.useDssiVstChunks,     nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_MAX_PARAMETERS,             standalone.options.maxParameters,        nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE,      standalone.options.preferredBufferSize,  nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE,      standalone.options.preferredSampleRate,  nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_OSC_UI_TIMEOUT,             standalone.options.oscUiTimeout,         nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_NATIVE,      0, standalone.options.bridge_native);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX32,     0, standalone.options.bridge_posix32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX64,     0, standalone.options.bridge_posix64);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN32,       0, standalone.options.bridge_win32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN64,       0, standalone.options.bridge_win64);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2,    0, standalone.options.bridge_lv2gtk2);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3,    0, standalone.options.bridge_lv2gtk3);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4,     0, standalone.options.bridge_lv2qt4);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5,     0, standalone.options.bridge_lv2qt5);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA,   0, standalone.options.bridge_lv2cocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, standalone.options.bridge_lv2win);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11,     0, standalone.options.bridge_lv2qt4);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA,   0, standalone.options.bridge_vstcocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND,    0, standalone.options.bridge_vsthwnd);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_X11,     0, standalone.options.bridge_vstx11);

    if (standalone.procName.isNotEmpty())
        standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_NAME, 0, standalone.procName);

    standalone.started = standalone.engine->init(clientName);

    if (standalone.started)
    {
        standalone.lastError = "no error";
    }
    else
    {
        delete standalone.engine;
        standalone.engine = nullptr;
    }

    return standalone.started;
}

bool carla_engine_close()
{
    qDebug("carla_engine_close()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
    {
        standalone.lastError = "Engine is not started";
        return false;
    }

    standalone.engine->setAboutToClose();
    standalone.engine->removeAllPlugins();

    const bool closed = standalone.engine->close();

    standalone.started = false;

    // cleanup static data
    //get_plugin_info(0);
    //get_parameter_info(0, 0);
    //get_parameter_scalepoint_info(0, 0, 0);
    //get_chunk_data(0);
    //get_program_name(0, 0);
    //get_midi_program_name(0, 0);
    //get_real_plugin_name(0);

    delete standalone.engine;
    standalone.engine = nullptr;

    return closed;
}

bool carla_is_engine_running()
{
    qDebug("carla_is_engine_running()");

    return standalone.engine && standalone.engine->isRunning();
}

#if 0
// -------------------------------------------------------------------------------------------------------------------

short add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* const name, const char* label, void* extraStuff)
{
    qDebug("CarlaBackendStandalone::add_plugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", CarlaBackend::BinaryType2Str(btype), CarlaBackend::PluginType2Str(ptype), filename, name, label, extraStuff);
    CARLA_ASSERT(standalone.engine);

    if (btype != CarlaBackend::BINARY_NATIVE && ! extraStuff)
    {
        switch (btype)
        {
        case CarlaBackend::BINARY_NONE:
        case CarlaBackend::BINARY_OTHER:
            break;
        case CarlaBackend::BINARY_POSIX32:
            extraStuff = (void*)(const char*)standalone.options.bridge_posix32;
            break;
        case CarlaBackend::BINARY_POSIX64:
            extraStuff = (void*)(const char*)standalone.options.bridge_posix64;
            break;
        case CarlaBackend::BINARY_WIN32:
            extraStuff = (void*)(const char*)standalone.options.bridge_win32;
            break;
        case CarlaBackend::BINARY_WIN64:
            extraStuff = (void*)(const char*)standalone.options.bridge_win64;
            break;
        }
    }

    if (standalone.engine && standalone.engine->isRunning())
        return standalone.engine->addPlugin(btype, ptype, filename, name, label, extraStuff);

    standalone.lastError = "Engine is not started";
    return -1;
}

bool remove_plugin(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::remove_plugin(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (standalone.engine)
        return standalone.engine->removePlugin(pluginId);

    standalone.lastError = "Engine is not started";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

const PluginInfo* get_plugin_info(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_plugin_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static PluginInfo info;

    // reset
    info.type     = CarlaBackend::PLUGIN_NONE;
    info.category = CarlaBackend::PLUGIN_CATEGORY_NONE;
    info.hints    = 0x0;
    info.binary   = nullptr;
    info.name     = nullptr;
    info.uniqueId = 0;

    // cleanup
    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    if (info.maker)
    {
        free((void*)info.maker);
        info.maker = nullptr;
    }

    if (info.copyright)
    {
        free((void*)info.copyright);
        info.copyright = nullptr;
    }

    if (! standalone.engine)
        return &info;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
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

        plugin->getLabel(strBufLabel);
        info.label = strdup(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = strdup(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = strdup(strBufCopyright);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_plugin_info(%i) - could not find plugin", pluginId);
    return &info;
}

const PortCountInfo* get_audio_port_count_info(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_audio_port_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! standalone.engine)
        return &info;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        info.ins   = plugin->audioInCount();
        info.outs  = plugin->audioOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_audio_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const PortCountInfo* get_midi_port_count_info(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_midi_port_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! standalone.engine)
        return &info;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        info.ins   = plugin->midiInCount();
        info.outs  = plugin->midiOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_midi_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const PortCountInfo* get_parameter_count_info(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_parameter_count_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! standalone.engine)
        return &info;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        plugin->getParameterCountInfo(&info.ins, &info.outs, &info.total);
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const ParameterInfo* get_parameter_info(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_info(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    static ParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
    if (info.name)
    {
        free((void*)info.name);
        info.name = nullptr;
    }

    if (info.symbol)
    {
        free((void*)info.symbol);
        info.symbol = nullptr;
    }

    if (info.unit)
    {
        free((void*)info.unit);
        info.unit = nullptr;
    }

    if (! standalone.engine)
        return &info;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            char strBufName[STR_MAX] = { 0 };
            char strBufSymbol[STR_MAX] = { 0 };
            char strBufUnit[STR_MAX] = { 0 };

            info.scalePointCount = plugin->parameterScalePointCount(parameter_id);

            plugin->getParameterName(parameter_id, strBufName);
            info.name = strdup(strBufName);

            plugin->getParameterSymbol(parameter_id, strBufSymbol);
            info.symbol = strdup(strBufSymbol);

            plugin->getParameterUnit(parameter_id, strBufUnit);
            info.unit = strdup(strBufUnit);
        }
        else
            qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - could not find plugin", pluginId, parameter_id);
    return &info;
}

const ScalePointInfo* get_parameter_scalepoint_info(unsigned short pluginId, uint32_t parameter_id, uint32_t scalepoint_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameter_id, scalepoint_id);
    CARLA_ASSERT(standalone.engine);

    static ScalePointInfo info;

    // reset
    info.value = 0.0;

    // cleanup
    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    if (! standalone.engine)
        return &info;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            if (scalepoint_id < plugin->parameterScalePointCount(parameter_id))
            {
                char strBufLabel[STR_MAX] = { 0 };

                info.value = plugin->getParameterScalePointValue(parameter_id, scalepoint_id);

                plugin->getParameterScalePointLabel(parameter_id, scalepoint_id, strBufLabel);
                info.label = strdup(strBufLabel);
            }
            else
                qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - scalepoint_id out of bounds", pluginId, parameter_id, scalepoint_id);
        }
        else
            qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - parameter_id out of bounds", pluginId, parameter_id, parameter_id);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", pluginId, parameter_id, scalepoint_id);
    return &info;
}

const GuiInfo* get_gui_info(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_gui_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static GuiInfo info;

    // reset
    info.type      = CarlaBackend::GUI_NONE;
    info.resizable = false;

    if (! standalone.engine)
        return &info;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        plugin->getGuiInfo(&info.type, &info.resizable);
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_gui_info(%i) - could not find plugin", pluginId);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaBackend::ParameterData* get_parameter_data(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_data(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    static CarlaBackend::ParameterData data;

    if (! standalone.engine)
        return &data;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterData(parameter_id);

        qCritical("CarlaBackendStandalone::get_parameter_data(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_parameter_data(%i, %i) - could not find plugin", pluginId, parameter_id);
    return &data;
}

const CarlaBackend::ParameterRanges* get_parameter_ranges(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_ranges(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    static CarlaBackend::ParameterRanges ranges;

    if (! standalone.engine)
        return &ranges;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id);

        qCritical("CarlaBackendStandalone::get_parameter_ranges(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);
        return &ranges;
    }

    qCritical("CarlaBackendStandalone::get_parameter_ranges(%i, %i) - could not find plugin", pluginId, parameter_id);
    return &ranges;
}

const CarlaBackend::MidiProgramData* get_midi_program_data(unsigned short pluginId, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_data(%i, %i)", pluginId, midi_program_id);
    CARLA_ASSERT(standalone.engine);

    static CarlaBackend::MidiProgramData data;

    if (! standalone.engine)
        return &data;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            return plugin->midiProgramData(midi_program_id);

        qCritical("CarlaBackendStandalone::get_midi_program_data(%i, %i) - midi_program_id out of bounds", pluginId, midi_program_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_midi_program_data(%i, %i) - could not find plugin", pluginId, midi_program_id);
    return &data;
}

const CarlaBackend::CustomData* get_custom_data(unsigned short pluginId, uint32_t custom_data_id)
{
    qDebug("CarlaBackendStandalone::get_custom_data(%i, %i)", pluginId, custom_data_id);
    CARLA_ASSERT(standalone.engine);

    static CarlaBackend::CustomData data;

    if (! standalone.engine)
        return &data;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (custom_data_id < plugin->customDataCount())
            return plugin->customData(custom_data_id);

        qCritical("CarlaBackendStandalone::get_custom_data(%i, %i) - custom_data_id out of bounds", pluginId, custom_data_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_custom_data(%i, %i) - could not find plugin", pluginId, custom_data_id);
    return &data;
}

const char* get_chunk_data(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_chunk_data(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static const char* chunk_data = nullptr;

    // cleanup
    if (chunk_data)
    {
        free((void*)chunk_data);
        chunk_data = nullptr;
    }

    if (! standalone.engine)
        return nullptr;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            const int32_t dataSize = plugin->chunkData(&data);

            if (data && dataSize >= 4)
            {
                const QByteArray chunk((const char*)data, dataSize);
                chunk_data = strdup(chunk.toBase64().constData());
            }
            else
                qCritical("CarlaBackendStandalone::get_chunk_data(%i) - got invalid chunk data", pluginId);
        }
        else
            qCritical("CarlaBackendStandalone::get_chunk_data(%i) - plugin does not support chunks", pluginId);

        return chunk_data;
    }

    qCritical("CarlaBackendStandalone::get_chunk_data(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_parameter_count(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_parameter_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->parameterCount();

    qCritical("CarlaBackendStandalone::get_parameter_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t get_program_count(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->programCount();

    qCritical("CarlaBackendStandalone::get_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t get_midi_program_count(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_midi_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->midiProgramCount();

    qCritical("CarlaBackendStandalone::get_midi_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t get_custom_data_count(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_custom_data_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->customDataCount();

    qCritical("CarlaBackendStandalone::get_custom_data_count(%i) - could not find plugin", pluginId);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_parameter_text(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_text(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    static char textBuf[STR_MAX];
    memset(textBuf, 0, sizeof(char)*STR_MAX);

    if (! standalone.engine)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            plugin->getParameterText(parameter_id, textBuf);
            return textBuf;
        }

        qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - could not find plugin", pluginId, parameter_id);
    return nullptr;
}

const char* get_program_name(unsigned short pluginId, uint32_t program_id)
{
    qDebug("CarlaBackendStandalone::get_program_name(%i, %i)", pluginId, program_id);
    CARLA_ASSERT(standalone.engine);

    static const char* program_name = nullptr;

    // cleanup
    if (program_name)
    {
        free((void*)program_name);
        program_name = nullptr;
    }

    if (! standalone.engine)
        return nullptr;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (program_id < plugin->programCount())
        {
            char strBuf[STR_MAX] = { 0 };

            plugin->getProgramName(program_id, strBuf);
            program_name = strdup(strBuf);

            return program_name;
        }

        qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - program_id out of bounds", pluginId, program_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - could not find plugin", pluginId, program_id);
    return nullptr;
}

const char* get_midi_program_name(unsigned short pluginId, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_name(%i, %i)", pluginId, midi_program_id);
    CARLA_ASSERT(standalone.engine);

    static const char* midi_program_name = nullptr;

    // cleanup
    if (midi_program_name)
    {
        free((void*)midi_program_name);
        midi_program_name = nullptr;
    }

    if (! standalone.engine)
        return nullptr;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
        {
            char strBuf[STR_MAX] = { 0 };

            plugin->getMidiProgramName(midi_program_id, strBuf);
            midi_program_name = strdup(strBuf);

            return midi_program_name;
        }

        qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - program_id out of bounds", pluginId, midi_program_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - could not find plugin", pluginId, midi_program_id);
    return nullptr;
}

const char* get_real_plugin_name(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_real_plugin_name(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    static const char* real_plugin_name = nullptr;

    // cleanup
    if (real_plugin_name)
    {
        free((void*)real_plugin_name);
        real_plugin_name = nullptr;
    }

    if (! standalone.engine)
        return nullptr;

    if (! standalone.started)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        char strBuf[STR_MAX] = { 0 };

        plugin->getRealName(strBuf);
        real_plugin_name = strdup(strBuf);

        return real_plugin_name;
    }

    qCritical("CarlaBackendStandalone::get_real_plugin_name(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t get_current_program_index(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_current_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return -1;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->currentProgram();

    qCritical("CarlaBackendStandalone::get_current_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

int32_t get_current_midi_program_index(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::get_current_midi_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return -1;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->currentMidiProgram();

    qCritical("CarlaBackendStandalone::get_current_midi_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

double get_default_parameter_value(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_default_parameter_value(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0.0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id)->def;

        qCritical("CarlaBackendStandalone::get_default_parameter_value(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);
        return 0.0;
    }

    qCritical("CarlaBackendStandalone::get_default_parameter_value(%i, %i) - could not find plugin", pluginId, parameter_id);
    return 0.0;
}

double get_current_parameter_value(unsigned short pluginId, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_current_parameter_value(%i, %i)", pluginId, parameter_id);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0.0;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->getParameterValue(parameter_id);

        qCritical("CarlaBackendStandalone::get_current_parameter_value(%i, %i) - parameter_id out of bounds", pluginId, parameter_id);
        return 0.0;
    }

    qCritical("CarlaBackendStandalone::get_current_parameter_value(%i, %i) - could not find plugin", pluginId, parameter_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

double get_input_peak_value(unsigned short pluginId, unsigned short port_id)
{
    CARLA_ASSERT(standalone.engine);
    CARLA_ASSERT(port_id == 1 || port_id == 2);

    if (! standalone.engine)
        return 0.0;

#if 0
    if (pluginId >= standalone.engine->maxPluginNumber())
    {
        qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid plugin value", pluginId, port_id);
        return 0.0;
    }
#endif

    if (port_id == 1 || port_id == 2)
        return standalone.engine->getInputPeak(pluginId, port_id-1);

    qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid port value", pluginId, port_id);
    return 0.0;
}

double get_output_peak_value(unsigned short pluginId, unsigned short port_id)
{
    CARLA_ASSERT(standalone.engine);
    CARLA_ASSERT(port_id == 1 || port_id == 2);

    if (! standalone.engine)
        return 0.0;

#if 0
    if (pluginId >= standalone.engine->maxPluginNumber())
    {
        qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid plugin value", pluginId, port_id);
        return 0.0;
    }
#endif

    if (port_id == 1 || port_id == 2)
        return standalone.engine->getOutputPeak(pluginId, port_id-1);

    qCritical("CarlaBackendStandalone::get_output_peak_value(%i, %i) - invalid port value", pluginId, port_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

void set_active(unsigned short pluginId, bool onOff)
{
    qDebug("CarlaBackendStandalone::set_active(%i, %s)", pluginId, bool2str(onOff));
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setActive(onOff, true, false);

    qCritical("CarlaBackendStandalone::set_active(%i, %s) - could not find plugin", pluginId, bool2str(onOff));
}

void set_drywet(unsigned short pluginId, double value)
{
    qDebug("CarlaBackendStandalone::set_drywet(%i, %g)", pluginId, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setDryWet(value, true, false);

    qCritical("CarlaBackendStandalone::set_drywet(%i, %g) - could not find plugin", pluginId, value);
}

void set_volume(unsigned short pluginId, double value)
{
    qDebug("CarlaBackendStandalone::set_volume(%i, %g)", pluginId, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setVolume(value, true, false);

    qCritical("CarlaBackendStandalone::set_volume(%i, %g) - could not find plugin", pluginId, value);
}

void set_balance_left(unsigned short pluginId, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_left(%i, %g)", pluginId, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setBalanceLeft(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_left(%i, %g) - could not find plugin", pluginId, value);
}

void set_balance_right(unsigned short pluginId, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_right(%i, %g)", pluginId, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setBalanceRight(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_right(%i, %g) - could not find plugin", pluginId, value);
}

// -------------------------------------------------------------------------------------------------------------------

void set_parameter_value(unsigned short pluginId, uint32_t parameter_id, double value)
{
    qDebug("CarlaBackendStandalone::set_parameter_value(%i, %i, %g)", pluginId, parameter_id, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterValue(parameter_id, value, true, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_value(%i, %i, %g) - parameter_id out of bounds", pluginId, parameter_id, value);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_value(%i, %i, %g) - could not find plugin", pluginId, parameter_id, value);
}

void set_parameter_midi_channel(unsigned short pluginId, uint32_t parameter_id, uint8_t channel)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i)", pluginId, parameter_id, channel);
    CARLA_ASSERT(standalone.engine);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - invalid channel number", pluginId, parameter_id, channel);
        return;
    }

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiChannel(parameter_id, channel, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - parameter_id out of bounds", pluginId, parameter_id, channel);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - could not find plugin", pluginId, parameter_id, channel);
}

void set_parameter_midi_cc(unsigned short pluginId, uint32_t parameter_id, int16_t cc)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i)", pluginId, parameter_id, cc);
    CARLA_ASSERT(standalone.engine);
    CARLA_ASSERT(cc >= -1 && cc <= 0x5F);

    if (cc < -1)
    {
        cc = -1;
    }
    else if (cc > 0x5F) // 95
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - invalid cc number", pluginId, parameter_id, cc);
        return;
    }

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiCC(parameter_id, cc, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - parameter_id out of bounds", pluginId, parameter_id, cc);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - could not find plugin", pluginId, parameter_id, cc);
}

void set_program(unsigned short pluginId, uint32_t program_id)
{
    qDebug("CarlaBackendStandalone::set_program(%i, %i)", pluginId, program_id);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (program_id < plugin->programCount())
            return plugin->setProgram(program_id, true, true, false, true);

        qCritical("CarlaBackendStandalone::set_program(%i, %i) - program_id out of bounds", pluginId, program_id);
        return;
    }

    qCritical("CarlaBackendStandalone::set_program(%i, %i) - could not find plugin", pluginId, program_id);
}

void set_midi_program(unsigned short pluginId, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::set_midi_program(%i, %i)", pluginId, midi_program_id);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            return plugin->setMidiProgram(midi_program_id, true, true, false, true);

        qCritical("CarlaBackendStandalone::set_midi_program(%i, %i) - midi_program_id out of bounds", pluginId, midi_program_id);
        return;
    }

    qCritical("CarlaBackendStandalone::set_midi_program(%i, %i) - could not find plugin", pluginId, midi_program_id);
}

// -------------------------------------------------------------------------------------------------------------------

void set_custom_data(unsigned short pluginId, const char* type, const char* key, const char* value)
{
    qDebug("CarlaBackendStandalone::set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setCustomData(type, key, value, true);

    qCritical("CarlaBackendStandalone::set_custom_data(%i, \"%s\", \"%s\", \"%s\") - could not find plugin", pluginId, type, key, value);
}

void set_chunk_data(unsigned short pluginId, const char* chunk_data)
{
    qDebug("CarlaBackendStandalone::set_chunk_data(%i, \"%s\")", pluginId, chunk_data);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
            return plugin->setChunkData(chunk_data);

        qCritical("CarlaBackendStandalone::set_chunk_data(%i, \"%s\") - plugin does not support chunks", pluginId, chunk_data);
        return;
    }

    qCritical("CarlaBackendStandalone::set_chunk_data(%i, \"%s\") - could not find plugin", pluginId, chunk_data);
}

void set_gui_container(unsigned short pluginId, uintptr_t gui_addr)
{
    qDebug("CarlaBackendStandalone::set_gui_container(%i, " P_UINTPTR ")", pluginId, gui_addr);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->setGuiContainer((GuiContainer*)CarlaBackend::getPointerFromAddress(gui_addr));

    qCritical("CarlaBackendStandalone::set_gui_container(%i, " P_UINTPTR ") - could not find plugin", pluginId, gui_addr);
}

// -------------------------------------------------------------------------------------------------------------------

void show_gui(unsigned short pluginId, bool yesno)
{
    qDebug("CarlaBackendStandalone::show_gui(%i, %s)", pluginId, bool2str(yesno));
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->showGui(yesno);

    qCritical("CarlaBackendStandalone::show_gui(%i, %s) - could not find plugin", pluginId, bool2str(yesno));
}

void idle_guis()
{
    CARLA_ASSERT(standalone.engine);

    if (standalone.engine)
        standalone.engine->idlePluginGuis();
}

// -------------------------------------------------------------------------------------------------------------------

void send_midi_note(unsigned short pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    qDebug("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);
    CARLA_ASSERT(standalone.engine);

    if (! (standalone.engine && standalone.engine->isRunning()))
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    qCritical("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i) - could not find plugin", pluginId, channel, note, velocity);
}

void prepare_for_save(unsigned short pluginId)
{
    qDebug("CarlaBackendStandalone::prepare_for_save(%i)", pluginId);
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId);

    if (plugin)
        return plugin->prepareForSave();

    qCritical("CarlaBackendStandalone::prepare_for_save(%i) - could not find plugin", pluginId);
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_buffer_size()
{
    qDebug("CarlaBackendStandalone::get_buffer_size()");
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0;

    return standalone.engine->getBufferSize();
}

double get_sample_rate()
{
    qDebug("CarlaBackendStandalone::get_sample_rate()");
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return 0.0;

    return standalone.engine->getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_last_error()
{
    qDebug("CarlaBackendStandalone::get_last_error()");

    if (standalone.engine)
        return standalone.engine->getLastError();

    return standalone.lastError;
}

const char* get_host_osc_url()
{
    qDebug("CarlaBackendStandalone::get_host_osc_url()");
    CARLA_ASSERT(standalone.engine);

    if (! standalone.engine)
        return nullptr;

    return standalone.engine->getOscServerPathTCP();
}

// -------------------------------------------------------------------------------------------------------------------

void set_callback_function(CarlaBackend::CallbackFunc func)
{
    qDebug("CarlaBackendStandalone::set_callback_function(%p)", func);

    standalone.callback = func;

    if (standalone.engine)
        standalone.engine->setCallback(func, nullptr);
}

void set_option(CarlaBackend::OptionsType option, int value, const char* value_str)
{
    qDebug("CarlaBackendStandalone::set_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2Str(option), value, value_str);

    if (standalone.engine)
        standalone.engine->setOption(option, value, value_str);

    switch (option)
    {
    case CarlaBackend::OPTION_PROCESS_NAME:
        standalone.procName = value_str;
        break;

    case CarlaBackend::OPTION_PROCESS_MODE:
        if (value < CarlaBackend::PROCESS_MODE_SINGLE_CLIENT || value > CarlaBackend::PROCESS_MODE_PATCHBAY)
            return qCritical("CarlaBackendStandalone::set_option(%s, %i, \"%s\") - invalid value", OptionsType2Str(option), value, value_str);

        standalone.options.processMode = static_cast<CarlaBackend::ProcessMode>(value);
        break;

    case CarlaBackend::OPTION_PROCESS_HIGH_PRECISION:
        standalone.options.processHighPrecision = value;
        break;

    case CarlaBackend::OPTION_MAX_PARAMETERS:
        standalone.options.maxParameters = (value > 0) ? value : CarlaBackend::MAX_PARAMETERS;
        break;

    case CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE:
        standalone.options.preferredBufferSize = value;
        break;

    case CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE:
        standalone.options.preferredSampleRate = value;
        break;

    case CarlaBackend::OPTION_FORCE_STEREO:
        standalone.options.forceStereo = value;
        break;

    case CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS:
        standalone.options.useDssiVstChunks = value;
        break;

    case CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES:
        standalone.options.preferPluginBridges = value;
        break;

    case CarlaBackend::OPTION_PREFER_UI_BRIDGES:
        standalone.options.preferUiBridges = value;
        break;

    case CarlaBackend::OPTION_OSC_UI_TIMEOUT:
        standalone.options.oscUiTimeout = value;
        break;

    case CarlaBackend::OPTION_PATH_BRIDGE_POSIX32:
        standalone.options.bridge_posix32 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_POSIX64:
        standalone.options.bridge_posix64 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_WIN32:
        standalone.options.bridge_win32 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_WIN64:
        standalone.options.bridge_win64 = value_str;
        break;

    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2:
        standalone.options.bridge_lv2gtk2 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3:
        standalone.options.bridge_lv2gtk3 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4:
        standalone.options.bridge_lv2qt4 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5:
        standalone.options.bridge_lv2qt5 = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA:
        standalone.options.bridge_lv2cocoa = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS:
        standalone.options.bridge_lv2win = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11:
        standalone.options.bridge_lv2x11 = value_str;
        break;

    case CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA:
        standalone.options.bridge_vstcocoa = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND:
        standalone.options.bridge_vsthwnd = value_str;
        break;
    case CarlaBackend::OPTION_PATH_BRIDGE_VST_X11:
        standalone.options.bridge_vstx11 = value_str;
        break;
    }
}

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
        qDebug("CarlaNSM::reply_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        m_controlAddr = lo_address_new_from_url(lo_address_get_url(lo_message_get_source(msg)));

        const char* const method = &argv[0]->s;

        if (strcmp(method, "/nsm/server/announce") == 0 && standalone.callback)
            standalone.callback(nullptr, CarlaBackend::CALLBACK_NSM_ANNOUNCE, 0, 0, 0, 0.0, nullptr); // FIXME?

        return 0;
    }

    int nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::nsm_open_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

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
        qDebug("CarlaNSM::nsm_save_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

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
        qCritical("CarlaNSM::error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }
};

static CarlaNSM carlaNSM;

void nsm_announce(const char* url, int pid)
{
    carlaNSM.announce(url, pid);
}

void nsm_reply_open()
{
    carlaNSM.replyOpen();
}

void nsm_reply_save()
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

void main_callback(void* ptr, CarlaBackend::CallbackType action, unsigned short pluginId, int value1, int value2, double value3)
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
        qDebug("------------------- TEST @%i: non-parameter calls --------------------", id);
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

        qDebug("------------------- TEST @%i: parameter calls [-1] --------------------", id);
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

        qDebug("------------------- TEST @%i: parameter calls [0] --------------------", id);
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

        qDebug("------------------- TEST @%i: set extra data --------------------", id);
        set_custom_data(id, CarlaBackend::CUSTOM_DATA_STRING, "", "");
        set_chunk_data(id, nullptr);
        set_gui_container(id, (uintptr_t)1);

        qDebug("------------------- TEST @%i: gui stuff --------------------", id);
        show_gui(id, false);
        show_gui(id, true);
        show_gui(id, true);

        idle_guis();
        idle_guis();
        idle_guis();

        qDebug("------------------- TEST @%i: other --------------------", id);
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
        qCritical("failed to start backend engine, reason:\n%s", get_last_error());
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
        qCritical("failed to start load plugins, reason:\n%s", get_last_error());
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
