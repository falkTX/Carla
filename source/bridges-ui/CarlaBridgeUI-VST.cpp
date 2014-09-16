/*
 * Carla Bridge UI, VST version
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeClient.hpp"
#include "CarlaBridgeToolkit.hpp"
#include "CarlaVstUtils.hpp"
#include "CarlaMIDI.h"

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

// fake values
uint32_t bufferSize = 512;
double   sampleRate = 44100.0;

class CarlaVstClient : public CarlaBridgeClient
{
public:
    CarlaVstClient(const char* const uiTitle)
        : CarlaBridgeClient(uiTitle),
          unique1(0),
          effect(nullptr),
          needIdle(false),
          unique2(1),
          leakDetector_CarlaVstClient()
    {
        // make client valid
        srand((uint)(uintptr_t)uiTitle[0]);
        unique1 = unique2 = rand();
    }

    ~CarlaVstClient() override
    {
        // make client invalid
        unique2 += 1;
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool uiInit(const char* binary, const char*, const char*) override
    {
        // -----------------------------------------------------------------
        // init

        CarlaBridgeClient::uiInit(binary, nullptr, nullptr);

        // -----------------------------------------------------------------
        // open DLL

        if (! uiLibOpen(binary))
        {
            carla_stderr("%s", uiLibError());
            return false;
        }

        // -----------------------------------------------------------------
        // get DLL main entry

        VST_Function vstFn = (VST_Function)uiLibSymbol("VSTPluginMain");

        if (vstFn == nullptr)
            vstFn = (VST_Function)uiLibSymbol("main");

        if (vstFn == nullptr)
            return false;

        // -----------------------------------------------------------------
        // initialize plugin

        lastVstPlugin = this;
        effect = vstFn(hostCallback);
        lastVstPlugin = nullptr;

        if (! (effect && effect->magic == kEffectMagic))
            return false;

        // -----------------------------------------------------------------
        // initialize VST stuff

#ifdef VESTIGE_HEADER
        effect->ptr1 = this;
#else
        effect->resvd1 = ToVstPtr(this);
#endif

        intptr_t value = 0;
#if defined(HAVE_X11) && defined(BRIDGE_X11)
        value = (intptr_t)getContainerId2();
#endif

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
        effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, bufferSize, nullptr, static_cast<float>(sampleRate));
#endif
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, static_cast<float>(sampleRate));
        effect->dispatcher(effect, effSetBlockSize, 0, bufferSize, nullptr, 0.0f);
        effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

        if (effect->dispatcher(effect, effEditOpen, 0, value, getContainerId(), 0.0f) != 1)
        {
            //effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
            //return false;
            carla_stderr("VST UI failed to open, trying to init anyway...");
        }

        // -----------------------------------------------------------------
        // initialize gui stuff

        ERect* vstRect = nullptr;
        effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f);

        if (vstRect)
        {
            int width  = vstRect->right - vstRect->left;
            int height = vstRect->bottom - vstRect->top;

            if (width > 0 && height > 0)
                toolkitResize(width, height);
        }

        return true;
    }

    void uiIdle() override
    {
        if (needIdle)
            effect->dispatcher(effect, effIdle, 0, 0, nullptr, 0.0f);
        effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
    }

    void uiClose() override
    {
        if (effect != nullptr)
        {
            effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
            effect = nullptr;
        }

        CarlaBridgeClient::uiClose();
        uiLibClose();
    }

    // ---------------------------------------------------------------------
    // ui management

    void* getWidget() const override
    {
        return nullptr; // VST always uses reparent
    }

    bool isResizable() const override
    {
        return false;
    }

    // ---------------------------------------------------------------------
    // ui processing

    void setParameter(const int32_t rindex, const float value) override
    {
        if (effect != nullptr)
            effect->setParameter(effect, rindex, value);
    }

    void setProgram(const uint32_t index) override
    {
        if (effect != nullptr)
            effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
    }

    void setMidiProgram(const uint32_t, const uint32_t) override
    {
    }

    void noteOn(const uint8_t, const uint8_t, const uint8_t) override
    {
    }

    void noteOff(const uint8_t, const uint8_t) override
    {
    }

    // ---------------------------------------------------------------------

    void handleAudioMasterAutomate(const uint32_t index, const float value)
    {
        effect->setParameter(effect, static_cast<int32_t>(index), value);

        if (isOscControlRegistered())
            sendOscControl(static_cast<int32_t>(index), value);
    }

    void handleAudioMasterNeedIdle()
    {
        needIdle = true;
    }

    intptr_t handleAudioMasterProcessEvents(const VstEvents* const vstEvents)
    {
        if (isOscControlRegistered())
            return 1;

        for (int32_t i=0; i < vstEvents->numEvents; ++i)
        {
            if (! vstEvents->events[i])
                break;

            const VstMidiEvent* const vstMidiEvent = (const VstMidiEvent*)vstEvents->events[i];

            if (vstMidiEvent->type != kVstMidiType)
            {
                uint8_t status = uint8_t(vstMidiEvent->midiData[0]);

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && vstMidiEvent->midiData[2] == 0)
                    status = uint8_t(status - 0x10);

                uint8_t midiBuf[4] = { 0, status, uint8_t(vstMidiEvent->midiData[1]), uint8_t(vstMidiEvent->midiData[2]) };
                sendOscMidi(midiBuf);
            }
        }

        return 1;
    }

    intptr_t handleAdioMasterSizeWindow(int32_t width, int32_t height)
    {
        toolkitResize(width, height);

        return 1;
    }

    void handleAudioMasterUpdateDisplay()
    {
        if (isOscControlRegistered())
            sendOscConfigure("reloadprograms", "");
    }

    // ---------------------------------------------------------------------

    static intptr_t hostCanDo(const char* const feature)
    {
        carla_debug("CarlaVstClient::hostCanDo(\"%s\")", feature);

        if (std::strcmp(feature, "supplyIdle") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstEvents") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstMidiEvent") == 0)
            return 1;
        if (std::strcmp(feature, "sendVstMidiEventFlagIsRealtime") == 0)
            return -1;
        if (std::strcmp(feature, "sendVstTimeInfo") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstEvents") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstMidiEvent") == 0)
            return 1;
        if (std::strcmp(feature, "receiveVstTimeInfo") == 0)
            return -1;
        if (std::strcmp(feature, "reportConnectionChanges") == 0)
            return -1;
        if (std::strcmp(feature, "acceptIOChanges") == 0)
            return 1;
        if (std::strcmp(feature, "sizeWindow") == 0)
            return 1;
        if (std::strcmp(feature, "offline") == 0)
            return -1;
        if (std::strcmp(feature, "openFileSelector") == 0)
            return -1;
        if (std::strcmp(feature, "closeFileSelector") == 0)
            return -1;
        if (std::strcmp(feature, "startStopProcess") == 0)
            return 1;
        if (std::strcmp(feature, "supportShell") == 0)
            return -1;
        if (std::strcmp(feature, "shellCategory") == 0)
            return -1;

        // unimplemented
        carla_stderr("CarlaVstClient::hostCanDo(\"%s\") - unknown feature", feature);
        return 0;
    }

    static intptr_t VSTCALLBACK hostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#if DEBUG
        carla_debug("CarlaVstClient::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

        // Check if 'resvd1' points to this client, or register ourselfs if possible
        CarlaVstClient* self = nullptr;

        if (effect)
        {
#ifdef VESTIGE_HEADER
            if (effect && effect->ptr1)
            {
                self = (CarlaVstClient*)effect->ptr1;
#else
            if (effect && effect->resvd1)
            {
                self = FromVstPtr<CarlaVstClient>(effect->resvd1);
#endif
                if (self->unique1 != self->unique2)
                    self = nullptr;
            }

            if (self != nullptr)
            {
                if (! self->effect)
                    self->effect = effect;

                CARLA_ASSERT(self->effect == effect);

                if (self->effect != effect)
                {
                    carla_stderr("CarlaVstClient::hostCallback() - host pointer mismatch: %p != %p", self->effect, effect);
                    self = nullptr;
                }
            }
            else if (lastVstPlugin)
            {
#ifdef VESTIGE_HEADER
                effect->ptr1 = lastVstPlugin;
#else
                effect->resvd1 = ToVstPtr(lastVstPlugin);
#endif
                self = lastVstPlugin;
            }
        }

        intptr_t ret = 0;

        switch (opcode)
        {
        case audioMasterAutomate:
            if (self != nullptr && index >= 0)
                self->handleAudioMasterAutomate(static_cast<uint32_t>(index), opt);
            break;

        case audioMasterVersion:
            ret = kVstVersion;
            break;

        case audioMasterCurrentId:
            // TODO
            break;

        case audioMasterIdle:
            //if (effect)
            //    effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            break;

        case audioMasterGetTime:
            static VstTimeInfo vstTimeInfo;
            carla_zeroStruct(vstTimeInfo);

            vstTimeInfo.sampleRate = sampleRate;

            // Tempo
            vstTimeInfo.tempo  = 120.0;
            vstTimeInfo.flags |= kVstTempoValid;

            // Time Signature
            vstTimeInfo.timeSigNumerator   = 4;
            vstTimeInfo.timeSigDenominator = 4;
            vstTimeInfo.flags |= kVstTimeSigValid;

            ret = (intptr_t)&vstTimeInfo;
            break;

        case audioMasterProcessEvents:
            if (self != nullptr && ptr != nullptr)
                ret = self->handleAudioMasterProcessEvents((const VstEvents*)ptr);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            ret = 120 * 10000;
            break;
#endif

        case audioMasterSizeWindow:
            if (self != nullptr && index > 0 && value > 0)
                ret = self->handleAdioMasterSizeWindow(index, static_cast<int32_t>(value));
            break;

        case audioMasterGetSampleRate:
            ret = static_cast<intptr_t>(sampleRate);
            break;

        case audioMasterGetBlockSize:
            ret = static_cast<intptr_t>(bufferSize);
            break;

        case audioMasterGetCurrentProcessLevel:
            ret = kVstProcessLevelUser;
            break;

        case audioMasterGetAutomationState:
            ret = kVstAutomationReadWrite;
            break;

        case audioMasterGetVendorString:
            if (ptr != nullptr)
                std::strcpy((char*)ptr, "falkTX");
            break;

        case audioMasterGetProductString:
            if (ptr != nullptr)
                std::strcpy((char*)ptr, "Carla-Bridge");
            break;

        case audioMasterGetVendorVersion:
            ret = 0x110; // 1.1.0
            break;

        case audioMasterCanDo:
            if (ptr != nullptr)
                ret = hostCanDo((const char*)ptr);
            break;

        case audioMasterGetLanguage:
            ret = kVstLangEnglish;
            break;

        case audioMasterUpdateDisplay:
            if (self != nullptr)
                self->handleAudioMasterUpdateDisplay();
            break;

        default:
#ifdef DEBUG
            carla_debug("CarlaVstClient::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif
            break;
        }

        return ret;
    }

private:
    int unique1;

    AEffect* effect;

    bool needIdle;
    static CarlaVstClient* lastVstPlugin;

    int unique2;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaVstClient)
};

CarlaVstClient* CarlaVstClient::lastVstPlugin = nullptr;

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE

    if (argc != 4)
    {
        carla_stderr("usage: %s <osc-url|\"null\"> <binary> <ui-title>", argv[0]);
        return 1;
    }

    const char* oscUrl  = argv[1];
    const char* binary  = argv[2];
    const char* uiTitle = argv[3];

    const bool useOsc = std::strcmp(oscUrl, "null");

    // try to get sampleRate value
    if (const char* const sampleRateStr = std::getenv("CARLA_SAMPLE_RATE"))
        sampleRate = std::atof(sampleRateStr);

    // Init VST client
    CarlaVstClient client(uiTitle);

    // Init OSC
    if (useOsc)
    {
        client.oscInit(oscUrl);
        client.sendOscUpdate();
    }

    // Load UI
    int ret;

    if (client.uiInit(binary, nullptr, nullptr))
    {
        client.toolkitExec(!useOsc);
        ret = 0;
    }
    else
    {
        carla_stderr("Failed to load VST UI");
        ret = 1;
    }

    // Close OSC
    if (useOsc)
        client.oscClose();

    // Close VST client
    client.uiClose();

    return ret;
}
