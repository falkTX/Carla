/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
# ifdef DISTRHO_UI_QT4
#  include <QtGui/QApplication>
# endif
#endif

#ifndef __cdecl
# define __cdecl
#endif

#define VST_FORCE_DEPRECATED 0

//#if VESTIGE_HEADER
//# include "vestige/aeffectx.h"
//# define effSetProgramName 4
//#else
#include "vst/aeffectx.h"
//#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI

# ifdef DISTRHO_UI_QT4
class QStaticScopedAppInit
{
public:
    QStaticScopedAppInit()
        : fApp(nullptr),
          fInitiated(false) {}

    void initIfNeeded()
    {
        if (fInitiated)
            return;

        fInitiated = true;

        if (qApp != nullptr)
        {
            fApp = qApp;
        }
        else
        {
            static int    qargc = 0;
            static char** qargv = nullptr;
            fApp = new QApplication(qargc, qargv, true);
            fApp->setQuitOnLastWindowClosed(false);
        }
    }

    void idle()
    {
        initIfNeeded();
        fApp->processEvents();
    }

private:
    QApplication* fApp;
    bool fInitiated;
};

static QStaticScopedAppInit sApp;
# endif

class UIVst
{
public:
    UIVst(audioMasterCallback audioMaster, AEffect* effect, PluginInternal* plugin, intptr_t winId)
        : kAudioMaster(audioMaster),
          kEffect(effect),
          kPlugin(plugin),
          fUi(this, winId, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback),
          fParameterChecks(nullptr),
          fParameterValues(nullptr)
    {
        uint32_t paramCount = plugin->parameterCount();

        if (paramCount > 0)
        {
            fParameterChecks = new bool[paramCount];
            fParameterValues = new float[paramCount];

            for (uint32_t i=0; i < paramCount; ++i)
            {
                fParameterChecks[i] = false;
                fParameterValues[i] = 0.0f;
            }
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        fNextProgram = -1;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fMidiEventCount = 0;
#endif
    }

    ~UIVst()
    {
        if (fParameterChecks != nullptr)
            delete[] fParameterChecks;

        if (fParameterValues != nullptr)
            delete[] fParameterValues;
    }

    // ---------------------------------------------

    void idle()
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fNextProgram != -1)
        {
            fUi.programChanged(fNextProgram);
            fNextProgram = -1;
        }
#endif

        for (uint32_t i=0, count = kPlugin->parameterCount(); i < count; ++i)
        {
            if (fParameterChecks[i])
            {
                fParameterChecks[i] = false;

                fUi.parameterChanged(i, fParameterValues[i]);
            }
        }

#if DISTRHO_PLUGIN_IS_SYNTH
        // TODO - notes
#endif

        fUi.idle();
    }

    int16_t getWidth()
    {
        return fUi.width();
    }

    int16_t getHeight()
    {
        return fUi.height();
    }

    // ---------------------------------------------
    // functions called from the plugin side, no block

    void setParameterValueFromPlugin(uint32_t index, float perValue)
    {
        fParameterChecks[index] = true;
        fParameterValues[index] = perValue;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setProgramFromPlugin(uint32_t index)
    {
        fNextProgram = index;

        // set previous parameters invalid
        for (uint32_t i=0, count = kPlugin->parameterCount(); i < count; ++i)
            fParameterChecks[i] = false;
    }
#endif

    // ---------------------------------------------

protected:
    intptr_t hostCallback(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return kAudioMaster(kEffect, opcode, index, value, ptr, opt);
    }

    void editParameter(uint32_t index, bool started)
    {
        if (started)
            hostCallback(audioMasterBeginEdit, index, 0, nullptr, 0.0f);
        else
            hostCallback(audioMasterEndEdit, index, 0, nullptr, 0.0f);
    }

    void setParameterValue(uint32_t index, float realValue)
    {
        const ParameterRanges& ranges = kPlugin->parameterRanges(index);
        float perValue = ranges.normalizeValue(realValue);

        kPlugin->setParameterValue(index, realValue);
        hostCallback(audioMasterAutomate, index, 0, nullptr, perValue);
    }

    void setState(const char* key, const char* value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        kPlugin->setState(key, value);
#endif
    }

    void sendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        // TODO
#endif
    }

    void uiResize(unsigned int width, unsigned int height)
    {
        hostCallback(audioMasterSizeWindow, width, height, nullptr, 0.0f);
    }

private:
    // Vst stuff
    audioMasterCallback const kAudioMaster;
    AEffect* const kEffect;
    PluginInternal* const kPlugin;

    // Plugin UI
    UIInternal fUi;

    // Temporary data
    bool*  fParameterChecks;
    float* fParameterValues;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t fNextProgram;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[MAX_MIDI_EVENTS];
#endif

    // ---------------------------------------------
    // Callbacks

    #define handlePtr ((UIVst*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->editParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->setState(key, value);
    }

    static void sendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(onOff, channel, note, velocity);
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        handlePtr->uiResize(width, height);
    }

    #undef handlePtr
};
#endif

// -------------------------------------------------

class PluginVst
{
public:
    PluginVst(audioMasterCallback audioMaster, AEffect* effect)
        : kAudioMaster(audioMaster),
          kEffect(effect)
    {
#if DISTRHO_PLUGIN_HAS_UI
        fVstUi          = nullptr;
        fVstRect.top    = 0;
        fVstRect.left   = 0;
        fVstRect.bottom = 0;
        fVstRect.right  = 0;
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        fCurProgram = -1;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fMidiEventCount = 0;
#endif
    }

    ~PluginVst()
    {
    }

    intptr_t vst_dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        int32_t ret = 0;

        switch (opcode)
        {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        case effSetProgram:
            if (value >= 0 && value < static_cast<intptr_t>(fPlugin.programCount()))
            {
                fCurProgram = value;
                fPlugin.setProgram(fCurProgram);

#if DISTRHO_PLUGIN_HAS_UI
                if (fVstUi != nullptr)
                    fVstUi->setProgramFromPlugin(fCurProgram);
#endif

                ret = 1;
            }
            break;

        case effGetProgram:
            ret = fCurProgram;
            break;

        case effSetProgramName:
            // unsupported
            break;

        case effGetProgramName:
            if (ptr != nullptr && fCurProgram >= 0 && fCurProgram < static_cast<int32_t>(fPlugin.programCount()))
            {
                std::strncpy((char*)ptr, fPlugin.programName(fCurProgram), kVstMaxProgNameLen);
                ret = 1;
            }
            break;
#endif

        case effGetParamDisplay:
            if (ptr != nullptr && index < static_cast<int32_t>(fPlugin.parameterCount()))
            {
                snprintf((char*)ptr, kVstMaxParamStrLen, "%f", fPlugin.parameterValue(index));
                ret = 1;
            }
            break;

        case effSetSampleRate:
            fPlugin.setSampleRate(opt, true);
            break;

        case effSetBlockSize:
            fPlugin.setBufferSize(value, true);
            break;

        case effMainsChanged:
            if (value == 0)
                fPlugin.deactivate();
            else
                fPlugin.activate();
            break;

#if DISTRHO_PLUGIN_HAS_UI
        case effEditGetRect:
            createUiIfNeeded(0);
            *(ERect**)ptr = &fVstRect;
            ret = 1;
            break;

        case effEditOpen:
            createUiIfNeeded((intptr_t)ptr);
            ret = 1;
            break;

        case effEditClose:
            if (fVstUi != nullptr)
            {
                delete fVstUi;
                fVstUi = nullptr;
# ifdef DISTRHO_UI_QT4
                sApp.idle();
# endif
                ret = 1;
            }
            break;

        case effEditIdle:
            if (fVstUi != nullptr)
                fVstUi->idle();
# ifdef DISTRHO_UI_QT4
            sApp.idle();
# endif
            break;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        case effGetChunk:
        case effSetChunk:
            // TODO
            break;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        case effProcessEvents:
            if (ptr)
            {
                //VstEvents* events = (VstEvents*)ptr;
                // TODO
            }
            break;
#endif

        case effCanBeAutomated:
            if (index < static_cast<int32_t>(fPlugin.parameterCount()))
            {
                uint32_t hints = fPlugin.parameterHints(index);

                // must be automable, and not output
                if ((hints & PARAMETER_IS_AUTOMABLE) != 0 && (hints & PARAMETER_IS_OUTPUT) == 0)
                    ret = 1;
            }
            break;

        case effCanDo:
#if DISTRHO_PLUGIN_IS_SYNTH
#endif
            // TODO
            break;

        case effStartProcess:
        case effStopProcess:
            break;
        }

        return ret;
    }

    float vst_getParameter(int32_t index)
    {
        const ParameterRanges& ranges = fPlugin.parameterRanges(index);
        return ranges.normalizeValue(fPlugin.parameterValue(index));
    }

    void vst_setParameter(int32_t index, float value)
    {
        const ParameterRanges& ranges = fPlugin.parameterRanges(index);
        float realValue = ranges.unnormalizeValue(value);
        fPlugin.setParameterValue(index, realValue);

#if DISTRHO_PLUGIN_HAS_UI
        if (fVstUi != nullptr)
            fVstUi->setParameterValueFromPlugin(index, realValue);
#endif
    }

    void vst_processReplacing(float** inputs, float** outputs, int32_t sampleFrames)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        fPlugin.run(inputs, outputs, sampleFrames, fMidiEventCount, fMidiEvents);

        // TODO - send notes to UI
#else
        fPlugin.run(inputs, outputs, sampleFrames, 0, nullptr);
#endif
    }

    // ---------------------------------------------

private:
    // VST stuff
    audioMasterCallback const kAudioMaster;
    AEffect* const kEffect;

    // Plugin
    PluginInternal fPlugin;

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    UIVst* fVstUi;
    ERect  fVstRect;
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t fCurProgram;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[MAX_MIDI_EVENTS];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    void createUiIfNeeded(intptr_t ptr)
    {
        if (fVstUi != nullptr)
            return;

        d_lastUiSampleRate = kAudioMaster(kEffect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);

        if (ptr == 0)
        {
            // fake temporary window, just to set ui size
            UIVst tmpUi(kAudioMaster, kEffect, &fPlugin, ptr);
            fVstRect.right  = tmpUi.getWidth();
            fVstRect.bottom = tmpUi.getHeight();
        }
        else
        {
            // real parented ui
            fVstUi = new UIVst(kAudioMaster, kEffect, &fPlugin, ptr);
            fVstRect.right  = fVstUi->getWidth();
            fVstRect.bottom = fVstUi->getHeight();

# if DISTRHO_PLUGIN_WANT_PROGRAMS
            if (fCurProgram >= 0)
                fVstUi->setProgramFromPlugin(fCurProgram);
# endif

            for (uint32_t i=0, count = fPlugin.parameterCount(); i < count; ++i)
                fVstUi->setParameterValueFromPlugin(i, fPlugin.parameterValue(i));
        }
    }
#endif
};

// -------------------------------------------------

#define handlePtr ((PluginVst*)effect->object)

static intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    // first internal init
    bool doInternalInit = (opcode == -1 && index == 0xdead && value == 0xf00d);

    if (doInternalInit)
    {
        // set valid but dummy values
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
    }

    // Create dummy plugin to get data from
    static PluginInternal plugin;

    if (doInternalInit)
    {
        // unset
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        *(PluginInternal**)ptr = &plugin;
        return 0;
    }

    // handle base opcodes
    switch (opcode)
    {
    case effOpen:
        if (effect != nullptr && effect->object == nullptr)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->user;
            d_lastBufferSize = audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);
            d_lastSampleRate = audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
            effect->object   = new PluginVst(audioMaster, effect);
#ifdef DISTRHO_UI_QT4
            QStaticScopedAppInit::addOne();
#endif
            return 1;
        }
        return 0;

    case effClose:
        if (effect != nullptr && effect->object != nullptr)
        {
            delete (PluginVst*)effect->object;
            effect->object = nullptr;
            delete effect;
#ifdef DISTRHO_UI_QT4
            QStaticScopedAppInit::removeOne();
#endif
            return 1;
        }
        return 0;

    case effGetParamLabel:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.parameterCount()))
        {
            std::strncpy((char*)ptr, plugin.parameterUnit(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

    case effGetParamName:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.parameterCount()))
        {
            std::strncpy((char*)ptr, plugin.parameterName(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    case effGetProgramNameIndexed:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.parameterCount()))
        {
            std::strncpy((char*)ptr, plugin.programName(index), kVstMaxProgNameLen);
            return 1;
        }
        return 0;
#endif

    case effGetPlugCategory:
#if DISTRHO_PLUGIN_IS_SYNTH
        return kPlugCategSynth;
#else
        return kPlugCategUnknown;
#endif

    case effGetEffectName:
        if (ptr != nullptr)
        {
            std::strncpy((char*)ptr, plugin.name(), kVstMaxProductStrLen);
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (ptr != nullptr)
        {
            std::strncpy((char*)ptr, plugin.maker(), kVstMaxVendorStrLen);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (ptr != nullptr)
        {
            std::strncpy((char*)ptr, plugin.label(), kVstMaxEffectNameLen);
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return plugin.version();

    case effGetVstVersion:
        return kVstVersion;
    };

    // handle object opcodes
    if (effect != nullptr && effect->object != nullptr)
        return handlePtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    if (effect != nullptr && effect->object != nullptr)
        return handlePtr->vst_getParameter(index);
    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    if (effect != nullptr && effect->object != nullptr)
        handlePtr->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (effect != nullptr && effect->object != nullptr)
        handlePtr->vst_processReplacing(inputs, outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (effect != nullptr && effect->object != nullptr)
        handlePtr->vst_processReplacing(inputs, outputs, sampleFrames);
}

#undef handlePtr

END_NAMESPACE_DISTRHO

// -------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    USE_NAMESPACE_DISTRHO

    // old version
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    PluginInternal* plugin = nullptr;
    vst_dispatcherCallback(nullptr, -1, 0xdead, 0xf00d, &plugin, 0.0f);

    AEffect* effect = new AEffect;
    memset(effect, 0, sizeof(AEffect));

    // vst fields
    effect->magic    = kEffectMagic;
    effect->uniqueID = plugin->uniqueId();
    effect->version  = plugin->version();

    // plugin fields
    effect->numParams   = plugin->parameterCount();
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    effect->numPrograms = plugin->programCount();
#endif
    effect->numInputs   = DISTRHO_PLUGIN_NUM_INPUTS;
    effect->numOutputs  = DISTRHO_PLUGIN_NUM_OUTPUTS;

    // static calls
    effect->dispatcher   = vst_dispatcherCallback;
    effect->process      = vst_processCallback;
    effect->getParameter = vst_getParameterCallback;
    effect->setParameter = vst_setParameterCallback;
    effect->processReplacing = vst_processReplacingCallback;
    effect->processDoubleReplacing = nullptr;

    // plugin flags
    effect->flags |= effFlagsCanReplacing;
#if DISTRHO_PLUGIN_HAS_UI
    effect->flags |= effFlagsHasEditor;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    effect->flags |= effFlagsProgramChunks;
#endif

    // pointers
    effect->object = nullptr;
    effect->user   = (void*)audioMaster;

    return effect;
}

// -------------------------------------------------
