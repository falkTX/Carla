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

#include "carla_backend_utils.hpp"
#include "carla_engine.hpp"
#include "carla_plugin.hpp"
#include "carla_midi.h"
#include "carla_native.h"

#include <QtCore/QByteArray>

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

    CarlaBackendStandalone()
        : callback(nullptr),
          engine(nullptr) {}
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
#ifdef WANT_DSSI
    standalone.engine->setOption(CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS,        standalone.options.useDssiVstChunks,     nullptr);
#endif
    standalone.engine->setOption(CarlaBackend::OPTION_MAX_PARAMETERS,             standalone.options.maxParameters,        nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE,      standalone.options.preferredBufferSize,  nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE,      standalone.options.preferredSampleRate,  nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_OSC_UI_TIMEOUT,             standalone.options.oscUiTimeout,         nullptr);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_NATIVE,      0, standalone.options.bridge_native);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX32,     0, standalone.options.bridge_posix32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_POSIX64,     0, standalone.options.bridge_posix64);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN32,       0, standalone.options.bridge_win32);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_WIN64,       0, standalone.options.bridge_win64);
#ifdef WANT_LV2
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK2,    0, standalone.options.bridge_lv2gtk2);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_GTK3,    0, standalone.options.bridge_lv2gtk3);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT4,     0, standalone.options.bridge_lv2qt4);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_QT5,     0, standalone.options.bridge_lv2qt5);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_COCOA,   0, standalone.options.bridge_lv2cocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, standalone.options.bridge_lv2win);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_LV2_X11,     0, standalone.options.bridge_lv2qt4);
#endif
#ifdef WANT_VST
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_COCOA,   0, standalone.options.bridge_vstcocoa);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_HWND,    0, standalone.options.bridge_vsthwnd);
    standalone.engine->setOption(CarlaBackend::OPTION_PATH_BRIDGE_VST_X11,     0, standalone.options.bridge_vstx11);
#endif

    if (standalone.procName.isNotEmpty())
        standalone.engine->setOption(CarlaBackend::OPTION_PROCESS_NAME, 0, standalone.procName);

    const bool initiated = standalone.engine->init(clientName);

    if (initiated)
    {
        standalone.lastError = "no error";
    }
    else
    {
        delete standalone.engine;
        standalone.engine = nullptr;
    }

    return initiated;
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

    delete standalone.engine;
    standalone.engine = nullptr;

    return closed;
}

void carla_engine_idle()
{
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine)
        standalone.engine->idle();
}

bool carla_is_engine_running()
{
    qDebug("carla_is_engine_running()");

    return standalone.engine != nullptr && standalone.engine->isRunning();
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_project(const char* filename)
{
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);
}

bool carla_save_project(const char* filename)
{
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(filename != nullptr);
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* const name, const char* label, void* extraStuff)
{
    qDebug("carla_add_plugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", CarlaBackend::BinaryType2Str(btype), CarlaBackend::PluginType2Str(ptype), filename, name, label, extraStuff);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->addPlugin(btype, ptype, filename, name, label, extraStuff);

    standalone.lastError = "Engine is not started";
    return false;
}

bool carla_remove_plugin(unsigned int pluginId)
{
    qDebug("carla_remove_plugin(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        return standalone.engine->removePlugin(pluginId);

    standalone.lastError = "Engine is not started";
    return false;
}

CARLA_EXPORT void carla_remove_all_plugins()
{
    qDebug("carla_remove_all_plugins()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine != nullptr && standalone.engine->isRunning())
        standalone.engine->removeAllPlugins();
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(unsigned int pluginId)
{
    qDebug("carla_get_plugin_info(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaPluginInfo info;

    // reset
    info.type     = CarlaBackend::PLUGIN_NONE;
    info.category = CarlaBackend::PLUGIN_CATEGORY_NONE;
    info.hints    = 0x0;
    info.binary   = nullptr;
    info.name     = nullptr;
    info.uniqueId = 0;
    info.latency  = 0;

    // cleanup
    if (info.label != nullptr)
    {
        std::free((void*)info.label);
        info.label = nullptr;
    }

    if (info.maker != nullptr)
    {
        std::free((void*)info.maker);
        info.maker = nullptr;
    }

    if (info.copyright != nullptr)
    {
        std::free((void*)info.copyright);
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

        plugin->getLabel(strBufLabel);
        info.label = strdup(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = strdup(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = strdup(strBufCopyright);

        return &info;
    }

    qCritical("carla_get_plugin_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(unsigned int pluginId)
{
    qDebug("carla_get_audio_port_count_info(%i)", pluginId);
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

    qCritical("carla_get_audio_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(unsigned int pluginId)
{
    qDebug("carla_get_midi_port_count_info(%i)", pluginId);
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

    qCritical("carla_get_midi_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(unsigned int pluginId)
{
    qDebug("carla_get_parameter_count_info(%i)", pluginId);
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

    qCritical("carla_get_parameter_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaParameterInfo* carla_get_parameter_info(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_parameter_info(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
    if (info.name != nullptr)
    {
        std::free((void*)info.name);
        info.name = nullptr;
    }

    if (info.symbol != nullptr)
    {
        std::free((void*)info.symbol);
        info.symbol = nullptr;
    }

    if (info.unit != nullptr)
    {
        std::free((void*)info.unit);
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
            info.name = strdup(strBufName);

            plugin->getParameterSymbol(parameterId, strBufSymbol);
            info.symbol = strdup(strBufSymbol);

            plugin->getParameterUnit(parameterId, strBufUnit);
            info.unit = strdup(strBufUnit);
        }
        else
            qCritical("carla_get_parameter_info(%i, %i) - parameterId out of bounds", pluginId, parameterId);

        return &info;
    }

    qCritical("carla_get_parameter_info(%i, %i) - could not find plugin", pluginId, parameterId);
    return &info;
}

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(unsigned int pluginId, uint32_t parameterId, uint32_t scalePointId)
{
    qDebug("carla_get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameterId, scalePointId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaScalePointInfo info;

    // reset
    info.value = 0.0f;

    // cleanup
    if (info.label != nullptr)
    {
        free((void*)info.label);
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
                info.label = strdup(strBufLabel);
            }
            else
                qCritical("carla_get_parameter_scalepoint_info(%i, %i, %i) - scalePointId out of bounds", pluginId, parameterId, scalePointId);
        }
        else
            qCritical("carla_get_parameter_scalepoint_info(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, scalePointId);

        return &info;
    }

    qCritical("carla_get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", pluginId, parameterId, scalePointId);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaBackend::ParameterData* carla_get_parameter_data(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::ParameterData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return &plugin->parameterData(parameterId);

        qCritical("carla_get_parameter_data(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &data;
    }

    qCritical("carla_get_parameter_data(%i, %i) - could not find plugin", pluginId, parameterId);
    return &data;
}

const CarlaBackend::ParameterRanges* carla_get_parameter_ranges(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::ParameterRanges ranges;

    if (standalone.engine == nullptr)
        return &ranges;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return &plugin->parameterRanges(parameterId);

        qCritical("carla_get_parameter_ranges(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &ranges;
    }

    qCritical("carla_get_parameter_ranges(%i, %i) - could not find plugin", pluginId, parameterId);
    return &ranges;
}

const CarlaBackend::MidiProgramData* carla_get_midi_program_data(unsigned int pluginId, uint32_t midiProgramId)
{
    qDebug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::MidiProgramData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->midiProgramCount())
            return &plugin->midiProgramData(midiProgramId);

        qCritical("carla_get_midi_program_data(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return &data;
    }

    qCritical("carla_get_midi_program_data(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return &data;
}

const CarlaBackend::CustomData* carla_get_custom_data(unsigned int pluginId, uint32_t customDataId)
{
    qDebug("carla_get_custom_data(%i, %i)", pluginId, customDataId);
    CARLA_ASSERT(standalone.engine != nullptr);

    static CarlaBackend::CustomData data;

    if (standalone.engine == nullptr)
        return &data;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (customDataId < plugin->customDataCount())
            return &plugin->customData(customDataId);

        qCritical("carla_get_custom_data(%i, %i) - customDataId out of bounds", pluginId, customDataId);
        return &data;
    }

    qCritical("carla_get_custom_data(%i, %i) - could not find plugin", pluginId, customDataId);
    return &data;
}

const char* carla_get_chunk_data(unsigned int pluginId)
{
    qDebug("carla_get_chunk_data(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    static CarlaString chunkData;

    // cleanup
    chunkData.clear();

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            const int32_t dataSize = plugin->chunkData(&data);

            if (data != nullptr && dataSize >= 4)
            {
                const QByteArray chunk((const char*)data, dataSize);
                chunkData = chunk.toBase64().constData();
            }
            else
                qCritical("carla_get_chunk_data(%i) - got invalid chunk data", pluginId);
        }
        else
            qCritical("carla_get_chunk_data(%i) - plugin does not support chunks", pluginId);

        return (const char*)chunkData;
    }

    qCritical("carla_get_chunk_data(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(unsigned int pluginId)
{
    qDebug("carla_get_parameter_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->parameterCount();

    qCritical("carla_get_parameter_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_program_count(unsigned int pluginId)
{
    qDebug("carla_get_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->programCount();

    qCritical("carla_get_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_midi_program_count(unsigned int pluginId)
{
    qDebug("carla_get_midi_program_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->midiProgramCount();

    qCritical("carla_get_midi_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_custom_data_count(unsigned int pluginId)
{
    qDebug("carla_get_custom_data_count(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->customDataCount();

    qCritical("carla_get_custom_data_count(%i) - could not find plugin", pluginId);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_parameter_text(%i, %i)", pluginId, parameterId);
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

        qCritical("carla_get_parameter_text(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return nullptr;
    }

    qCritical("carla_get_parameter_text(%i, %i) - could not find plugin", pluginId, parameterId);
    return nullptr;
}

const char* carla_get_program_name(unsigned int pluginId, uint32_t programId)
{
    qDebug("carla_get_program_name(%i, %i)", pluginId, programId);
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

        qCritical("carla_get_program_name(%i, %i) - programId out of bounds", pluginId, programId);
        return nullptr;
    }

    qCritical("carla_get_program_name(%i, %i) - could not find plugin", pluginId, programId);
    return nullptr;
}

const char* carla_get_midi_program_name(unsigned int pluginId, uint32_t midiProgramId)
{
    qDebug("carla_get_midi_program_name(%i, %i)", pluginId, midiProgramId);
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

        qCritical("carla_get_midi_program_name(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return nullptr;
    }

    qCritical("carla_get_midi_program_name(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return nullptr;
}

const char* carla_get_real_plugin_name(unsigned int pluginId)
{
    qDebug("carla_get_real_plugin_name(%i)", pluginId);
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

    qCritical("carla_get_real_plugin_name(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(unsigned int pluginId)
{
    qDebug("carla_get_current_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return -1;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->currentProgram();

    qCritical("carla_get_current_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

int32_t carla_get_current_midi_program_index(unsigned int pluginId)
{
    qDebug("carla_get_current_midi_program_index(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return -1;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->currentMidiProgram();

    qCritical("carla_get_current_midi_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_default_parameter_value(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0f;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->parameterRanges(parameterId).def;

        qCritical("carla_get_default_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    qCritical("carla_get_default_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_current_parameter_value(unsigned int pluginId, uint32_t parameterId)
{
    qDebug("carla_get_current_parameter_value(%i, %i)", pluginId, parameterId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0f;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->getParameterValue(parameterId);

        qCritical("carla_get_current_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    qCritical("carla_get_current_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
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
        qCritical("carla_get_input_peak_value(%i, %i) - invalid plugin value", pluginId, portId);
        return 0.0f;
    }

    if (portId == 1 || portId == 2)
       return standalone.engine->getInputPeak(pluginId, portId-1);

    qCritical("carla_get_input_peak_value(%i, %i) - invalid port value", pluginId, portId);
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
        qCritical("carla_get_input_peak_value(%i, %i) - invalid plugin value", pluginId, portId);
        return 0.0f;
    }

    if (portId == 1 || portId == 2)
       return standalone.engine->getOutputPeak(pluginId, portId-1);

    qCritical("carla_get_output_peak_value(%i, %i) - invalid port value", pluginId, portId);
    return 0.0f;
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_active(unsigned int pluginId, bool onOff)
{
    qDebug("carla_set_active(%i, %s)", pluginId, bool2str(onOff));
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setActive(onOff, true, false);

    qCritical("carla_set_active(%i, %s) - could not find plugin", pluginId, bool2str(onOff));
}

void carla_set_drywet(unsigned int pluginId, float value)
{
    qDebug("carla_set_drywet(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setDryWet(value, true, false);

    qCritical("carla_set_drywet(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_volume(unsigned int pluginId, float value)
{
    qDebug("carla_set_volume(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setVolume(value, true, false);

    qCritical("carla_set_volume(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_left(unsigned int pluginId, float value)
{
    qDebug("carla_set_balance_left(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setBalanceLeft(value, true, false);

    qCritical("carla_set_balance_left(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_right(unsigned int pluginId, float value)
{
    qDebug("carla_set_balance_right(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setBalanceRight(value, true, false);

    qCritical("carla_set_balance_right(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_panning(unsigned int pluginId, float value)
{
    qDebug("carla_set_panning(%i, %f)", pluginId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setPanning(value, true, false);

    qCritical("carla_set_panning(%i, %f) - could not find plugin", pluginId, value);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(unsigned int pluginId, uint32_t parameterId, float value)
{
    qDebug("carla_set_parameter_value(%i, %i, %f)", pluginId, parameterId, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterValue(parameterId, value, true, true, false);

        qCritical("carla_set_parameter_value(%i, %i, %f) - parameterId out of bounds", pluginId, parameterId, value);
        return;
    }

    qCritical("carla_set_parameter_value(%i, %i, %f) - could not find plugin", pluginId, parameterId, value);
}

void carla_set_parameter_midi_channel(unsigned int pluginId, uint32_t parameterId, uint8_t channel)
{
    qDebug("carla_set_parameter_midi_channel(%i, %i, %i)", pluginId, parameterId, channel);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
    {
        qCritical("carla_set_parameter_midi_channel(%i, %i, %i) - invalid channel number", pluginId, parameterId, channel);
        return;
    }

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterMidiChannel(parameterId, channel, true, false);

        qCritical("carla_set_parameter_midi_channel(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, channel);
        return;
    }

    qCritical("carla_set_parameter_midi_channel(%i, %i, %i) - could not find plugin", pluginId, parameterId, channel);
}

void carla_set_parameter_midi_cc(unsigned int pluginId, uint32_t parameterId, int16_t cc)
{
    qDebug("carla_set_parameter_midi_cc(%i, %i, %i)", pluginId, parameterId, cc);
    CARLA_ASSERT(standalone.engine != nullptr);
    CARLA_ASSERT(cc >= -1 && cc <= 0x5F);

    if (cc < -1)
    {
        cc = -1;
    }
    else if (cc > 0x5F) // 95
    {
        qCritical("carla_set_parameter_midi_cc(%i, %i, %i) - invalid cc number", pluginId, parameterId, cc);
        return;
    }

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->parameterCount())
            return plugin->setParameterMidiCC(parameterId, cc, true, false);

        qCritical("carla_set_parameter_midi_cc(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, cc);
        return;
    }

    qCritical("carla_set_parameter_midi_cc(%i, %i, %i) - could not find plugin", pluginId, parameterId, cc);
}

void carla_set_program(unsigned int pluginId, uint32_t programId)
{
    qDebug("carla_set_program(%i, %i)", pluginId, programId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->programCount())
            return plugin->setProgram(programId, true, true, false, true);

        qCritical("carla_set_program(%i, %i) - programId out of bounds", pluginId, programId);
        return;
    }

    qCritical("carla_set_program(%i, %i) - could not find plugin", pluginId, programId);
}

void carla_set_midi_program(unsigned int pluginId, uint32_t midiProgramId)
{
    qDebug("carla_set_midi_program(%i, %i)", pluginId, midiProgramId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->midiProgramCount())
            return plugin->setMidiProgram(midiProgramId, true, true, false, true);

        qCritical("carla_set_midi_program(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return;
    }

    qCritical("carla_set_midi_program(%i, %i) - could not find plugin", pluginId, midiProgramId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(unsigned int pluginId, const char* type, const char* key, const char* value)
{
    qDebug("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->setCustomData(type, key, value, true);

    qCritical("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\") - could not find plugin", pluginId, type, key, value);
}

void carla_set_chunk_data(unsigned int pluginId, const char* chunkData)
{
    qDebug("carla_set_chunk_data(%i, \"%s\")", pluginId, chunkData);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
            return plugin->setChunkData(chunkData);

        qCritical("carla_set_chunk_data(%i, \"%s\") - plugin does not support chunks", pluginId, chunkData);
        return;
    }

    qCritical("carla_set_chunk_data(%i, \"%s\") - could not find plugin", pluginId, chunkData);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(unsigned int pluginId)
{
    qDebug("carla_prepare_for_save(%i)", pluginId);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->prepareForSave();

    qCritical("carla_prepare_for_save(%i) - could not find plugin", pluginId);
}

void carla_send_midi_note(unsigned int pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    qDebug("carla_send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr || ! standalone.engine->isRunning())
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    qCritical("carla_send_midi_note(%i, %i, %i, %i) - could not find plugin", pluginId, channel, note, velocity);
}

void carla_show_gui(unsigned int pluginId, bool yesno)
{
    qDebug("carla_show_gui(%i, %s)", pluginId, bool2str(yesno));
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return;

    if (CarlaPlugin* const plugin = standalone.engine->getPlugin(pluginId))
        return plugin->showGui(yesno);

    qCritical("carla_show_gui(%i, %s) - could not find plugin", pluginId, bool2str(yesno));
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_buffer_size()
{
    qDebug("carla_get_buffer_size()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0;

    return standalone.engine->getBufferSize();
}

double carla_get_sample_rate()
{
    qDebug("carla_get_sample_rate()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return 0.0;

    return standalone.engine->getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_last_error()
{
    qDebug("carla_get_last_error()");

    if (standalone.engine != nullptr)
        return standalone.engine->getLastError();

    return standalone.lastError;
}

const char* carla_get_host_osc_url()
{
    qDebug("carla_get_host_osc_url()");
    CARLA_ASSERT(standalone.engine != nullptr);

    if (standalone.engine == nullptr)
        return nullptr;

    return standalone.engine->getOscServerPathTCP();
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_callback_function(CarlaBackend::CallbackFunc func)
{
    qDebug("carla_set_callback_function(%p)", func);

    standalone.callback = func;

    if (standalone.engine)
        standalone.engine->setCallback(func, nullptr);
}

void carla_set_option(CarlaBackend::OptionsType option, int value, const char* valueStr)
{
    qDebug("carla_set_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2Str(option), value, valueStr);

    if (standalone.engine)
        standalone.engine->setOption(option, value, valueStr);

    switch (option)
    {
    case CarlaBackend::OPTION_PROCESS_NAME:
        standalone.procName = valueStr;
        break;

    case CarlaBackend::OPTION_PROCESS_MODE:
        if (value < CarlaBackend::PROCESS_MODE_SINGLE_CLIENT || value > CarlaBackend::PROCESS_MODE_PATCHBAY)
            return qCritical("carla_set_option(OPTION_PROCESS_MODE, %i, \"%s\") - invalid value", value, valueStr);

        standalone.options.processMode = static_cast<CarlaBackend::ProcessMode>(value);
        break;

    case CarlaBackend::OPTION_FORCE_STEREO:
        standalone.options.forceStereo = value;
        break;

    case CarlaBackend::OPTION_PREFER_PLUGIN_BRIDGES:
        standalone.options.preferPluginBridges = value;
        break;

    case CarlaBackend::OPTION_PREFER_UI_BRIDGES:
        standalone.options.preferUiBridges = value;
        break;

#ifdef WANT_DSSI
    case CarlaBackend::OPTION_USE_DSSI_VST_CHUNKS:
        standalone.options.useDssiVstChunks = value;
        break;
#endif

    case CarlaBackend::OPTION_MAX_PARAMETERS:
        standalone.options.maxParameters = (value > 0) ? value : CarlaBackend::MAX_DEFAULT_PARAMETERS;
        break;

    case CarlaBackend::OPTION_OSC_UI_TIMEOUT:
        standalone.options.oscUiTimeout = value;
        break;

    case CarlaBackend::OPTION_PREFERRED_BUFFER_SIZE:
        standalone.options.preferredBufferSize = value;
        break;

    case CarlaBackend::OPTION_PREFERRED_SAMPLE_RATE:
        standalone.options.preferredSampleRate = value;
        break;

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
