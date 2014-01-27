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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef WANT_LV2

#include "CarlaLv2Utils.hpp"
#include "CarlaMathUtils.hpp"

#include "Lv2AtomQueue.hpp"

#include "../engine/CarlaEngineOsc.hpp"

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

#include <QtCore/QDir>
#include <QtCore/QUrl>

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

// Extra Parameter Hints
const unsigned int PARAMETER_IS_STRICT_BOUNDS = 0x1000;
const unsigned int PARAMETER_IS_TRIGGER       = 0x2000;

// LV2 Feature Ids
const uint32_t kFeatureCount = 0;

// -----------------------------------------------------

class Lv2Plugin : public CarlaPlugin
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

        carla_fill<LV2_Feature*>(fFeatures, kFeatureCount+1, nullptr);

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
    }

    ~Lv2Plugin() override
    {
        carla_debug("Lv2Plugin::~Lv2Plugin()");

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

    PluginType getType() const noexcept override
    {
        return PLUGIN_LV2;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, CarlaPlugin::getCategory());

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

        return CarlaPlugin::getCategory();
    }

    long getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        return fRdfDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_INPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_OUTPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        const int32_t rindex(pData->param.data[parameterId].rindex);

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

    unsigned int getOptionsAvailable() const noexcept override
    {
        const uint32_t hasMidiIn(getMidiInCount() > 0);

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (! (hasMidiIn || needsFixedBuffer()))
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (hasMidiIn)
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
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        if (pData->param.data[parameterId].hints & PARAMETER_IS_STRICT_BOUNDS)
            pData->param.ranges[parameterId].fixValue(fParamBuffers[parameterId]);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), 0.0f);

        const int32_t rindex(pData->param.data[parameterId].rindex);

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

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor->URI != nullptr,);

        if (fRdfDescriptor->URI != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->Author != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->License != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->Name != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port.Unit.Hints) && port.Unit.Symbol != nullptr)
            {
                std::strncpy(strBuf, port.Unit.Symbol, STR_MAX);
                return;
            }
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

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId),);

        const int32_t rindex(pData->param.data[parameterId].rindex);

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
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        carla_debug("Lv2Plugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(static_cast<uint32_t>(fRdfDescriptor->PortCount));

        uint32_t aIns, aOuts, params, j;
        aIns = aOuts = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (uint32_t i=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_AUDIO(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    aIns += 1;
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    aOuts += 1;
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
                params += 1;
        }

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) /*&& fExt.state == nullptr && fExt.worker == nullptr*/)
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate, fRdfDescriptor->Bundle, fFeatures);

            if (fHandle2 != nullptr)
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
            FLOAT_CLEAR(fParamBuffers, params);
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            portName.clear();

            if (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_CV(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += fRdfDescriptor->Ports[i].Name;
                portName.truncate(portNameSize);
            }

            if (LV2_IS_PORT_AUDIO(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iAudioIn++;
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                        pData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
            {
                const LV2_Property portProps(fRdfDescriptor->Ports[i].Properties);
                const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);
                const LV2_RDF_PortPoints portPoints(fRdfDescriptor->Ports[i].Points);

                j = iCtrl++;
                pData->param.data[j].type   = PARAMETER_UNKNOWN;
                pData->param.data[j].hints  = 0x0;
                pData->param.data[j].index  = j;
                pData->param.data[j].rindex = i;
                pData->param.data[j].midiCC = -1;
                pData->param.data[j].midiChannel = 0;

                float min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(portPoints.Hints))
                    min = portPoints.Minimum;
                else
                    min = 0.0f;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(portPoints.Hints))
                    max = portPoints.Maximum;
                else
                    max = 1.0f;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                // stupid hack for ir.lv2 (broken plugin)
                if (std::strcmp(fRdfDescriptor->URI, "http://factorial.hu/plugins/lv2/ir") == 0 && std::strncmp(fRdfDescriptor->Ports[i].Name, "FileHash", 8) == 0)
                {
                    min = 0.0f;
                    max = (float)0xffffff;
                }

                if (max - min == 0.0f)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': max - min == 0.0f", fRdfDescriptor->Ports[i].Name);
                    max = min + 0.1f;
                }

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(portPoints.Hints))
                {
                    def = portPoints.Default;
                }
                else
                {
                    // no default value
                    if (min < 0.0f && max > 0.0f)
                        def = 0.0f;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LV2_IS_PORT_SAMPLE_RATE(portProps))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_TOGGLED(portProps))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LV2_IS_PORT_INTEGER(portProps))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        carla_stderr("Plugin has latency input port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_FREEWHEEL;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        pData->param.data[j].type   = PARAMETER_INPUT;
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlIn = true;
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap& portMidiMap(fRdfDescriptor->Ports[i].MidiMap);

                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap.Type))
                    {
                        if (portMidiMap.Number < 0x5F && ! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap.Number))
                            pData->param.data[j].midiCC = int16_t(portMidiMap.Number);
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_LATENCY;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        carla_stderr("Plugin has freewheeling output port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        pData->param.data[j].type   = PARAMETER_OUTPUT;
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    pData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(portProps))
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_LOGARITHMIC(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                {
                    pData->param.data[j].hints &= ~(PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE);
                }
                else if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps) || LV2_IS_PORT_NOT_AUTOMATIC(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                //if (pData->param.data[j].type != PARAMETER_LV2_FREEWHEEL)
                    fParamBuffers[j] = def;
                //else
                //    fParamBuffers[j] = min;

                fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Port Type not supported, but it's optional anyway
                fDescriptor->connect_port(fHandle, i, nullptr);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, nullptr);
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

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (isRealtimeSafe())
            pData->hints |= PLUGIN_IS_RTSAFE;

        if (LV2_IS_GENERATOR(fRdfDescriptor->Type[0], fRdfDescriptor->Type[1]))
            pData->hints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints &= ~PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("Lv2Plugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(fHandle);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle2);
                } catch(...) {}
            }
        }
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(fHandle);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle2);
                } catch(...) {}
            }
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (i=0; i < pData->audioOut.count; ++i)
                FLOAT_CLEAR(outBuffer[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        {
            processSingle(inBuffer, outBuffer, frames, 0);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                if (pData->param.data[k].hints & PARAMETER_IS_STRICT_BOUNDS)
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

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
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

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (i=0; i < pData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < pData->audioIn.count; ++i)
            FLOAT_COPY(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);

        for (i=0; i < pData->audioOut.count; ++i)
            FLOAT_CLEAR(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fDescriptor->run(fHandle, frames);

        if (fHandle2 != nullptr)
            fDescriptor->run(fHandle2, frames);

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

        for (k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type != PARAMETER_INPUT)
                continue;

            if (pData->param.data[k].hints & PARAMETER_IS_TRIGGER)
            {
                if (fParamBuffers[k] != pData->param.ranges[k].def)
                {
                    fParamBuffers[k] = pData->param.ranges[k].def;
                    pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, fParamBuffers[k]);
                }
            }
        }

        pData->postRtEvents.trySplice();

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        // TODO
                        //if (k < pData->latency && pData->latency < frames)
                        //    bufValue = (pData->audioIn.count == 1) ? pData->latencyBuffers[0][k] : pData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (pData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = fAudioInBuffers[(pData->audioIn.count == 1) ? 0 : i][k];
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
                        FLOAT_COPY(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }
        } // End of Post-processing
#else
        for (i=0; i < pData->audioOut.count; ++i)
        {
            for (k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (pData->audioIn.count > 0)
            {
                CARLA_ASSERT(pData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (pData->audioOut.count > 0)
            {
                CARLA_ASSERT(pData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }

        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, int(newSampleRate));
        carla_debug("Lv2Plugin::sampleRateChanged(%g) - start", newSampleRate);

        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type == PARAMETER_INPUT && pData->param.special[k] == PARAMETER_SPECIAL_SAMPLE_RATE)
            {
                fParamBuffers[k] = float(newSampleRate);
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
                break;
            }
        }

        carla_debug("Lv2Plugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type == PARAMETER_INPUT && pData->param.special[k] == PARAMETER_SPECIAL_LV2_FREEWHEEL)
            {
                fParamBuffers[k] = isOffline ? pData->param.ranges[k].max : pData->param.ranges[k].min;
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
                break;
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
    {
        carla_debug("Lv2Plugin::clearBuffers() - start");

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

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("Lv2Plugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool isRealtimeSafe() const
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_CORE__hardRTCapable) == 0)
                return true;
        }

        return false;
    }

    bool needsFixedBuffer() const
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_BUF_SIZE__fixedBlockLength) == 0)
                return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const bundle, const char* const name, const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (bundle == nullptr || bundle[0] == '\0')
        {
            pData->engine->setLastError("null bundle");
            return false;
        }

        if (uri == nullptr || uri[0] == '\0')
        {
            pData->engine->setLastError("null uri");
            return false;
        }

        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

        // Convert bundle filename to URI
        QString qBundle(QUrl::fromLocalFile(bundle).toString());
        if (! qBundle.endsWith(OS_SEP_STR))
            qBundle += OS_SEP_STR;

        // Load bundle
        Lilv::Node lilvBundle(lv2World.new_uri(qBundle.toUtf8().constData()));
        lv2World.load_bundle(lilvBundle);

        fRdfDescriptor = lv2_rdf_new(uri, true);

        if (fRdfDescriptor == nullptr)
        {
            pData->engine->setLastError("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! pData->libOpen(fRdfDescriptor->Binary))
        {
            pData->engine->setLastError(pData->libError(fRdfDescriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const LV2_Descriptor_Function descFn = (LV2_Descriptor_Function)pData->libSymbol("lv2_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
            return false;
        }

        // -----------------------------------------------------------
        // get descriptor that matches URI

        uint32_t i = 0;
        while ((fDescriptor = descFn(i++)))
        {
            carla_debug("LV2 Init @%i => '%s' vs '%s'", i, fDescriptor->URI, uri);
            if (std::strcmp(fDescriptor->URI, uri) == 0)
                break;
        }

        if (fDescriptor == nullptr)
        {
            pData->engine->setLastError("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

        // Check supported ports
        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! is_lv2_port_supported(portTypes))
            {
                if (! LV2_IS_PORT_OPTIONAL(fRdfDescriptor->Ports[i].Properties))
                {
                    pData->engine->setLastError("Plugin requires a port type that is not currently supported");
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
//                 QString msg(QString("Plugin requires a feature that is not supported:\n%1").arg(fRdfDescriptor->Features[i].URI));
//                 pData->engine->setLastError(msg.toUtf8().constData());
                canContinue = false;
                break;
            }
        }

        if (! canContinue)
        {
            // error already set
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(fRdfDescriptor->Name);

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

        fHandle = fDescriptor->instantiate(fDescriptor, pData->engine->getSampleRate(), fRdfDescriptor->Bundle, fFeatures);

        if (fHandle == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (getMidiInCount() > 0 || needsFixedBuffer())
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getOptions().forceStereo)
                pData->options |= PLUGIN_OPTION_FORCE_STEREO;

            if (getMidiInCount() > 0)
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // set identifier string
            CarlaString identifier("V2/");
            identifier += uri;

            // load settings
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());

            // ignore settings, we need this anyway
            if (getMidiInCount() > 0 || needsFixedBuffer())
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount == 0)
            return true;

        return true;
    }

    // -------------------------------------------------------------------

private:
    LV2_Handle   fHandle;
    LV2_Handle   fHandle2;
    LV2_Feature* fFeatures[kFeatureCount+1];
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lv2Plugin)
};

// -------------------------------------------------------------------------------------------------------------------

#define lv2PluginPtr ((Lv2Plugin*)plugin)

int CarlaEngineOsc::handleMsgLv2AtomTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaOsc::handleMsgLv2AtomTransfer()");

    return 0;

    // unused for now
    (void)argv;
    (void)plugin;
}

int CarlaEngineOsc::handleMsgLv2UridMap(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaOsc::handleMsgLv2EventTransfer()");

    return 0;

    // unused for now
    (void)argv;
    (void)plugin;
}

#undef lv2PluginPtr

CARLA_BACKEND_END_NAMESPACE

#endif // WANT_LV2

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLV2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newLV2({%p, \"%s\", \"%s\"})", init.engine, init.name, init.label);

#ifdef WANT_LV2
    Lv2Plugin* const plugin(new Lv2Plugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
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

// -------------------------------------------------------------------------------------------------------------------
