/*
 * Carla LV2 Plugin
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

#include "CarlaPluginInternal.hpp"

#ifdef WANT_LV2

#include "CarlaPluginGui.hpp"
#include "CarlaLv2Utils.hpp"

extern "C" {
#include "rtmempool/rtmempool.h"
}

// -----------------------------------------------------
// Our LV2 World class object

Lv2WorldClass gLv2World;

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

class Lv2Plugin : public CarlaPlugin,
                  public CarlaPluginGui::Callback
{
public:
    Lv2Plugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("Lv2Plugin::Lv2Plugin(%p, %i)", engine, id);

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
    }

    ~Lv2Plugin() override
    {
        carla_debug("Lv2Plugin::~Lv2Plugin()");

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            //if (fGui.isOsc)
            {
                // Wait a bit first, then force kill
                if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
                {
                    carla_stderr("LV2 OSC-GUI thread still running, forcing termination now");
                    kData->osc.thread.terminate();
                }
            }
        }

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->active)
        {
            deactivate();
            kData->active = false;
        }

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->cleanup != nullptr)
            {
                if (fHandle != nullptr)
                    fDescriptor->cleanup(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->cleanup(fHandle2);
            }

            fHandle  = nullptr;
            fHandle2 = nullptr;
            fDescriptor = nullptr;
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

    PluginType type() const override
    {
        return PLUGIN_LV2;
    }

    PluginCategory category() override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        const LV2_Property cat1(fRdfDescriptor->Type[0]);
        const LV2_Property cat2(fRdfDescriptor->Type[1]);

        if (LV2_IS_DELAY(cat1, cat2))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_DISTORTION(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_DYNAMICS(cat1, cat2))
            return PLUGIN_CATEGORY_DYNAMICS;
        if (LV2_IS_EQ(cat1, cat2))
            return PLUGIN_CATEGORY_EQ;
        if (LV2_IS_FILTER(cat1, cat2))
            return PLUGIN_CATEGORY_FILTER;
        if (LV2_IS_GENERATOR(cat1, cat2))
            return PLUGIN_CATEGORY_SYNTH;
        if (LV2_IS_MODULATOR(cat1, cat2))
            return PLUGIN_CATEGORY_MODULATOR;
        if (LV2_IS_REVERB(cat1, cat2))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_SIMULATOR(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPATIAL(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPECTRAL(cat1, cat2))
            return PLUGIN_CATEGORY_UTILITY;
        if (LV2_IS_UTILITY(cat1, cat2))
            return PLUGIN_CATEGORY_UTILITY;

        return getPluginCategoryFromName(fName);
    }

    long uniqueId() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        return fRdfDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount() const override
    {
        uint32_t /*i,*/ count = 0;

#if 0
        for (i=0; i < evIn.count; ++i)
        {
            if (evIn.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }
#endif

        return count;
    }

    uint32_t midiOutCount() const override
    {
        uint32_t /*i,*/ count = 0;

#if 0
        for (i=0; i < evOut.count; ++i)
        {
            if (evOut.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }
#endif

        return count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);
            return port.ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions() override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_FIXED_BUFFER;
        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (kData->engine->getProccessMode() != PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (kData->audioIn.count <= 1 && kData->audioOut.count <= 1 && (kData->audioIn.count != 0 || kData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        //if (fDescriptor->midiIns > 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(fParamBuffers != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port.ScalePointCount)
            {
                const LV2_RDF_PortScalePoint& portScalePoint(port.ScalePoints[scalePointId]);
                return portScalePoint.Value;
            }
        }

        return 0.0f;
    }

    void getLabel(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(fRdfDescriptor->URI != nullptr);

        if (fRdfDescriptor->URI != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Author != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->License != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Name != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port.Unit.Hints) && port.Unit.Symbol)
                std::strncpy(strBuf, port.Unit.Symbol, STR_MAX);

            else if (LV2_HAVE_PORT_UNIT_UNIT(port.Unit.Hints))
            {
                switch (port.Unit.Unit)
                {
                case LV2_PORT_UNIT_BAR:
                    std::strncpy(strBuf, "bars", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BEAT:
                    std::strncpy(strBuf, "beats", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BPM:
                    std::strncpy(strBuf, "BPM", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CENT:
                    std::strncpy(strBuf, "ct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CM:
                    std::strncpy(strBuf, "cm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_COEF:
                    std::strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DB:
                    std::strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DEGREE:
                    std::strncpy(strBuf, "deg", STR_MAX);
                    return;
                case LV2_PORT_UNIT_FRAME:
                    std::strncpy(strBuf, "frames", STR_MAX);
                    return;
                case LV2_PORT_UNIT_HZ:
                    std::strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_INCH:
                    std::strncpy(strBuf, "in", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KHZ:
                    std::strncpy(strBuf, "kHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KM:
                    std::strncpy(strBuf, "km", STR_MAX);
                    return;
                case LV2_PORT_UNIT_M:
                    std::strncpy(strBuf, "m", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MHZ:
                    std::strncpy(strBuf, "MHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIDINOTE:
                    std::strncpy(strBuf, "note", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MILE:
                    std::strncpy(strBuf, "mi", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIN:
                    std::strncpy(strBuf, "min", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MM:
                    std::strncpy(strBuf, "mm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MS:
                    std::strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LV2_PORT_UNIT_OCT:
                    std::strncpy(strBuf, "oct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_PC:
                    std::strncpy(strBuf, "%", STR_MAX);
                    return;
                case LV2_PORT_UNIT_S:
                    std::strncpy(strBuf, "s", STR_MAX);
                    return;
                case LV2_PORT_UNIT_SEMITONE:
                    std::strncpy(strBuf, "semi", STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port.ScalePointCount)
            {
                const LV2_RDF_PortScalePoint& portScalePoint(port.ScalePoints[scalePointId]);

                if (portScalePoint.Label != nullptr)
                {
                    std::strncpy(strBuf, portScalePoint.Label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        CARLA_ASSERT(fHandle != nullptr);

#if 0
        if (fExt.state != nullptr && fExt.state->save != nullptr)
        {
            fExt.state->save(handle, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);

            if (fHandle2)
                fExt.state->save(h2, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);
        }
#endif
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo) override
    {
        if (true)
        {
            if (yesNo)
            {
                kData->osc.thread.start();
            }
            else
            {
                if (kData->osc.data.target != nullptr)
                {
                    osc_send_hide(&kData->osc.data);
                    osc_send_quit(&kData->osc.data);
                    kData->osc.data.free();
                }

                if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
                    kData->osc.thread.terminate();
            }
        }
        else
        {
            if (yesNo)
            {
                if (kData->gui == nullptr)
                {
                    // TODO
                    CarlaPluginGui::Options guiOptions;
                    guiOptions.parented  = false;
                    guiOptions.resizable = true;

                    kData->gui = new CarlaPluginGui(kData->engine, this, guiOptions);
                }

                //void* const ptr = kData->gui->getContainerWinId();

                // open
                if (true)
                {
                    kData->gui->setWindowTitle(QString("%1 (GUI)").arg((const char*)fName).toUtf8().constData());
                    kData->gui->show();
                }
                else
                {
                    if (kData->gui != nullptr)
                    {
                        kData->gui->close();
                        delete kData->gui;
                        kData->gui = nullptr;
                    }

                    kData->engine->callback(CALLBACK_ERROR, fId, 0, 0, 0.0f, "Plugin refused to open its own UI");
                    kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
                    return;
                }
            }
            else
            {
                // close here

                if (kData->gui != nullptr)
                {
                    kData->gui->close();
                    delete kData->gui;
                    kData->gui = nullptr;
                }
            }
        }

        //fGui.isVisible = yesNo;
    }

    void idleGui() override
    {
        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->activate != nullptr)
        {
            fDescriptor->activate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->activate(fHandle2);
        }
    }

    void deactivate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->deactivate != nullptr)
        {
            fDescriptor->deactivate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->deactivate(fHandle2);
        }
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < kData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < kData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (kData->audioIn.count > 0)
            {
                CARLA_ASSERT(kData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (kData->audioOut.count > 0)
            {
                CARLA_ASSERT(kData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }

        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("Lv2Plugin::sampleRateChanged(%g) - start", newSampleRate);

        // TODO
        (void)newSampleRate;

        carla_debug("Lv2Plugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
    {
        carla_debug("Lv2Plugin::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
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
            for (uint32_t i=0; i < kData->audioOut.count; ++i)
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

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("Lv2Plugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // -------------------------------------------------------------------

protected:
    void guiClosedCallback() override
    {
        showGui(false);
        kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const bundle, const char* const name, const char* const uri)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(bundle != nullptr);
        CARLA_ASSERT(uri != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (bundle == nullptr)
        {
            kData->engine->setLastError("null bundle");
            return false;
        }

        if (uri == nullptr)
        {
            kData->engine->setLastError("null uri");
            return false;
        }

        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        gLv2World.init();

        fRdfDescriptor = lv2_rdf_new(uri);

        if (fRdfDescriptor == nullptr)
        {
            kData->engine->setLastError("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! kData->libOpen(fRdfDescriptor->Binary))
        {
            kData->engine->setLastError(kData->libError(fRdfDescriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

#if 0
        const LV2_Lib_Descriptor_Function libDescFn = (LV2_Lib_Descriptor_Function)kData->libSymbol("lv2_lib_descriptor");

        if (libDescFn != nullptr)
        {
            // -----------------------------------------------------------
            // get lib descriptor

            const LV2_Lib_Descriptor* libFn = nullptr; //descLibFn(fRdfDescriptor->Bundle, features);

            if (libFn == nullptr || libFn->get_plugin == nullptr)
            {
                kData->engine->setLastError("Plugin failed to return library descriptor");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((fDescriptor = libFn->get_plugin(libFn->handle, i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }

            if (fDescriptor == nullptr)
                libFn->cleanup(libFn->handle);
        else
#endif
        {
            const LV2_Descriptor_Function descFn = (LV2_Descriptor_Function)kData->libSymbol("lv2_descriptor");

            if (descFn == nullptr)
            {
                kData->engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((fDescriptor = descFn(i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }
        }

        if (fDescriptor == nullptr)
        {
            kData->engine->setLastError("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

        // Check supported ports
        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_CONTROL(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes)))
            {
                if (! LV2_IS_PORT_OPTIONAL(fRdfDescriptor->Ports[i].Properties))
                {
                    kData->engine->setLastError("Plugin requires a port type that is not currently supported");
                    canContinue = false;
                    break;
                }
            }
        }

        // Check supported features
        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount && canContinue; ++i)
        {
            if (LV2_IS_FEATURE_REQUIRED(fRdfDescriptor->Features[i].Type) && ! is_lv2_feature_supported(fRdfDescriptor->Features[i].URI))
            {
                QString msg(QString("Plugin requires a feature that is not supported:\n%1").arg(fRdfDescriptor->Features[i].URI));
                kData->engine->setLastError(msg.toUtf8().constData());
                canContinue = false;
                break;
            }
        }

#if 0
        // Check extensions
        for (uint32_t i=0; i < fRdfDescriptor->ExtensionCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                fHints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_STATE__interface) == 0)
                fHints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_WORKER__interface) == 0)
                fHints |= PLUGIN_HAS_EXTENSION_WORKER;
            else
                carla_stdout("Plugin has non-supported extension: '%s'", fRdfDescriptor->Extensions[i]);
        }
#endif

        if (! canContinue)
        {
            // error already set
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = kData->engine->getUniquePluginName(name);
        else
            fName = kData->engine->getUniquePluginName(fRdfDescriptor->Name);

        fFilename = bundle;

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, kData->engine->getSampleRate(), fRdfDescriptor->Bundle, nullptr); // TODO: features

        if (fHandle == nullptr)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_FIXED_BUFFER;
            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (kData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            //if (fDescriptor->midiIns > 0)
            {
                fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
                fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // load settings
            kData->idStr  = "LV2/";
            kData->idStr += uri;
            fOptions = kData->loadSettings(fOptions, availableOptions());
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount == 0)
            return true;

        return true;
    }

private:
    LV2_Handle fHandle;
    LV2_Handle fHandle2;
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    struct UI {
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI*       rdfDescriptor;

        UI()
            : handle(nullptr),
              widget(nullptr),
              descriptor(nullptr),
              rdfDescriptor(nullptr) {}
    } fUi;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lv2Plugin)
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_VST
# warning Building without LV2 support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLV2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newLV2({%p, \"%s\", \"%s\", \"%s\"})", init.engine, init.filename, init.name, init.label);

#ifdef WANT_LV2
    Lv2Plugin* const plugin(new Lv2Plugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo LV2 plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("LV2 support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
