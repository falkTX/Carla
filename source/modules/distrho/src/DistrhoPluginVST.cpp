/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
#endif

#ifndef __cdecl
# define __cdecl
#endif

#define VST_FORCE_DEPRECATED 0

//#if VESTIGE_HEADER
//# include "vestige/aeffectx.h"
//# define effSetProgramName 4
//#else
#include "aeffectx.h"
//#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

void strncpy(char* const dst, const char* const src, const size_t size)
{
    std::strncpy(dst, src, size);
    dst[size] = '\0';
}

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI
class UIVst
{
public:
    UIVst(const audioMasterCallback audioMaster, AEffect* const effect, PluginExporter* const plugin, const intptr_t winId)
        : fAudioMaster(audioMaster),
          fEffect(effect),
          fPlugin(plugin),
          fUI(this, winId, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback),
          fParameterChecks(nullptr),
          fParameterValues(nullptr)
    {
        const uint32_t paramCount(plugin->getParameterCount());

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
    }

    ~UIVst()
    {
        if (fParameterChecks != nullptr)
        {
            delete[] fParameterChecks;
            fParameterChecks = nullptr;
        }

        if (fParameterValues != nullptr)
        {
            delete[] fParameterValues;
            fParameterValues = nullptr;
        }
    }

    // -------------------------------------------------------------------

    void idle()
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fNextProgram != -1)
        {
            fUI.programChanged(fNextProgram);
            fNextProgram = -1;
        }
#endif

        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
        {
            if (fParameterChecks[i])
            {
                fParameterChecks[i] = false;
                fUI.parameterChanged(i, fParameterValues[i]);
            }
        }

        fUI.idle();
    }

    int16_t getWidth() const noexcept
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const noexcept
    {
        return fUI.getHeight();
    }

    // -------------------------------------------------------------------
    // functions called from the plugin side, RT no block

    void setParameterValueFromPlugin(const uint32_t index, const float perValue)
    {
        fParameterChecks[index] = true;
        fParameterValues[index] = perValue;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setProgramFromPlugin(const uint32_t index)
    {
        fNextProgram = index;

        // set previous parameters invalid
        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
            fParameterChecks[i] = false;
    }
#endif

    // -------------------------------------------------------------------

protected:
    intptr_t hostCallback(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        return fAudioMaster(fEffect, opcode, index, value, ptr, opt);
    }

    void editParameter(const uint32_t index, const bool started)
    {
        if (started)
            hostCallback(audioMasterBeginEdit, index, 0, nullptr, 0.0f);
        else
            hostCallback(audioMasterEndEdit, index, 0, nullptr, 0.0f);
    }

    void setParameterValue(const uint32_t index, const float realValue)
    {
        const ParameterRanges& ranges(fPlugin->getParameterRanges(index));
        const float perValue(ranges.getNormalizedValue(realValue));

        fPlugin->setParameterValue(index, realValue);
        hostCallback(audioMasterAutomate, index, 0, nullptr, perValue);
    }

    void setState(const char* const key, const char* const value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        fPlugin->setState(key, value);
#else
        return; // unused
        (void)key;
        (void)value;
#endif
    }

    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        // TODO
#else
        return; // unused
        (void)channel;
        (void)note;
        (void)velocity;
#endif
    }

    void uiResize(const unsigned int width, const unsigned int height)
    {
        fUI.setSize(width, height);
        hostCallback(audioMasterSizeWindow, width, height, nullptr, 0.0f);
    }

private:
    // Vst stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;
    PluginExporter* const fPlugin;

    // Plugin UI
    UIExporter fUI;

    // Temporary data
    bool*  fParameterChecks;
    float* fParameterValues;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t fNextProgram;
#endif

    // -------------------------------------------------------------------
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

    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(channel, note, velocity);
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        handlePtr->uiResize(width, height);
    }

    #undef handlePtr
};
#endif

// -----------------------------------------------------------------------

class PluginVst
{
public:
    PluginVst(const audioMasterCallback audioMaster, AEffect* const effect)
        : fAudioMaster(audioMaster),
          fEffect(effect)
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        fCurProgram = -1;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fMidiEventCount = 0;
#endif

#if DISTRHO_PLUGIN_HAS_UI
        fVstUi          = nullptr;
        fVstRect.top    = 0;
        fVstRect.left   = 0;
        fVstRect.bottom = 0;
        fVstRect.right  = 0;
#endif
    }

    intptr_t vst_dispatcher(const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        int32_t ret = 0;

        switch (opcode)
        {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        case effSetProgram:
            if (value >= 0 && value < static_cast<intptr_t>(fPlugin.getProgramCount()))
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
            if (ptr != nullptr && fCurProgram >= 0 && fCurProgram < static_cast<int32_t>(fPlugin.getProgramCount()))
            {
                DISTRHO::strncpy((char*)ptr, fPlugin.getProgramName(fCurProgram), kVstMaxProgNameLen);
                ret = 1;
            }
            break;
#endif

        case effGetParamDisplay:
            if (ptr != nullptr && index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                char* buf = (char*)ptr;
                std::snprintf((char*)ptr, kVstMaxParamStrLen, "%f", fPlugin.getParameterValue(index));
                buf[kVstMaxParamStrLen] = '\0';
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
            if (fVstUi != nullptr)
            {
                fVstRect.right  = fVstUi->getWidth();
                fVstRect.bottom = fVstUi->getHeight();
            }
            else
            {
                d_lastUiSampleRate = fAudioMaster(fEffect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
                UIExporter tmpUI(nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
                fVstRect.right  = tmpUI.getWidth();
                fVstRect.bottom = tmpUI.getHeight();
            }
            *(ERect**)ptr = &fVstRect;
            ret = 1;
            break;

        case effEditOpen:
            if (fVstUi == nullptr)
            {
                d_lastUiSampleRate = fAudioMaster(fEffect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);

                fVstUi = new UIVst(fAudioMaster, fEffect, &fPlugin, (intptr_t)ptr);

# if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (fCurProgram >= 0)
                    fVstUi->setProgramFromPlugin(fCurProgram);
# endif
                for (uint32_t i=0, count = fPlugin.getParameterCount(); i < count; ++i)
                    fVstUi->setParameterValueFromPlugin(i, fPlugin.getParameterValue(i));

                fVstUi->idle();

                ret = 1;
            }
            break;

        case effEditClose:
            if (fVstUi != nullptr)
            {
                delete fVstUi;
                fVstUi = nullptr;
                ret = 1;
            }
            break;

        case effEditIdle:
            if (fVstUi != nullptr)
                fVstUi->idle();
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
            if (index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                const uint32_t hints(fPlugin.getParameterHints(index));

                // must be automable, and not output
                if ((hints & PARAMETER_IS_AUTOMABLE) != 0 && (hints & PARAMETER_IS_OUTPUT) == 0)
                    ret = 1;
            }
            break;

        case effCanDo:
#if DISTRHO_PLUGIN_IS_SYNTH
            if (const char* const canDo = (const char*)ptr)
            {
                if (std::strcmp(canDo, "receiveVstEvents") == 0)
                    ret = 1;
                else if (std::strcmp(canDo, "receiveVstMidiEvent") == 0)
                    ret = 1;
            }
#endif
            break;

        case effStartProcess:
        case effStopProcess:
            break;
        }

        return ret;
    }

    float vst_getParameter(const int32_t index)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        return ranges.getNormalizedValue(fPlugin.getParameterValue(index));
    }

    void vst_setParameter(const int32_t index, const float value)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        const float realValue(ranges.getUnnormalizedValue(value));
        fPlugin.setParameterValue(index, realValue);

#if DISTRHO_PLUGIN_HAS_UI
        if (fVstUi != nullptr)
            fVstUi->setParameterValueFromPlugin(index, realValue);
#endif
    }

    void vst_processReplacing(float** const inputs, float** const outputs, const int32_t sampleFrames)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        fPlugin.run(inputs, outputs, sampleFrames, fMidiEventCount, fMidiEvents);
#else
        fPlugin.run(inputs, outputs, sampleFrames);
#endif
    }

    // -------------------------------------------------------------------

private:
    // VST stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;

    // Plugin
    PluginExporter fPlugin;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t fCurProgram;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    UIVst* fVstUi;
    ERect  fVstRect;
#endif
};

// -----------------------------------------------------------------------

#define handlePtr ((PluginVst*)effect->resvd2)

static intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    // first internal init
    bool doInternalInit = (opcode == -1729 && index == 0xdead && value == 0xf00d);

    if (doInternalInit)
    {
        // set valid but dummy values
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
    }

    // Create dummy plugin to get data from
    static PluginExporter plugin;

    if (doInternalInit)
    {
        // unset
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        *(PluginExporter**)ptr = &plugin;
        return 0;
    }

    // handle base opcodes
    switch (opcode)
    {
    case effOpen:
        if (effect != nullptr && effect->object != nullptr /*&& dynamic_cast<audioMasterCallback>(effect->object)*/)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->object;
            d_lastBufferSize = audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);
            d_lastSampleRate = audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
            PluginVst* const plugin(new PluginVst(audioMaster, effect));
            effect->resvd2 = (intptr_t)plugin;
            return 1;
        }
        return 0;

    case effClose:
        if (effect != nullptr && effect->resvd2 != 0)
        {
            delete (PluginVst*)effect->resvd2;
            effect->object = nullptr;
            effect->resvd2 = 0;
            delete effect;
            return 1;
        }
        return 0;

    case effGetParamLabel:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getParameterUnit(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

    case effGetParamName:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getParameterName(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    case effGetProgramNameIndexed:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getProgramCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getProgramName(index), kVstMaxProgNameLen);
            return 1;
        }
        return 0;
#endif

    case effGetPlugCategory:
#if DISTRHO_PLUGIN_IS_SYNTH
        return kPlugCategSynth;
#else
        return kPlugCategEffect;
#endif

    case effGetEffectName:
        if (ptr != nullptr)
        {
            DISTRHO::strncpy((char*)ptr, plugin.getName(), kVstMaxProductStrLen);
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (ptr != nullptr)
        {
            DISTRHO::strncpy((char*)ptr, plugin.getMaker(), kVstMaxVendorStrLen);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (ptr != nullptr)
        {
            DISTRHO::strncpy((char*)ptr, plugin.getLabel(), kVstMaxEffectNameLen);
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return plugin.getVersion();

    case effGetVstVersion:
        return kVstVersion;
    };

    // handle advanced opcodes
    if (effect != nullptr && effect->resvd2 != 0)
        return handlePtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    if (effect != nullptr && effect->resvd2 != 0)
        return handlePtr->vst_getParameter(index);
    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    if (effect != nullptr && effect->resvd2 != 0)
        handlePtr->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (effect != nullptr && effect->resvd2 != 0)
        handlePtr->vst_processReplacing(inputs, outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (effect != nullptr && effect->resvd2 != 0)
        handlePtr->vst_processReplacing(inputs, outputs, sampleFrames);
}

#undef handlePtr

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    USE_NAMESPACE_DISTRHO

    // old version
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    // first internal init
    PluginExporter* plugin = nullptr;
    vst_dispatcherCallback(nullptr, -1729, 0xdead, 0xf00d, &plugin, 0.0f);

    AEffect* const effect(new AEffect);
    std::memset(effect, 0, sizeof(AEffect));

    // vst fields
    effect->magic    = kEffectMagic;
    effect->uniqueID = plugin->getUniqueId();
    effect->version  = plugin->getVersion();

    // plugin fields
    effect->numParams   = plugin->getParameterCount();
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    effect->numPrograms = plugin->getProgramCount();
#endif
    effect->numInputs   = DISTRHO_PLUGIN_NUM_INPUTS;
    effect->numOutputs  = DISTRHO_PLUGIN_NUM_OUTPUTS;

    // plugin flags
    effect->flags |= effFlagsCanReplacing;
#if DISTRHO_PLUGIN_IS_SYNTH
    effect->flags |= effFlagsIsSynth;
#endif
#if DISTRHO_PLUGIN_HAS_UI
    effect->flags |= effFlagsHasEditor;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    effect->flags |= effFlagsProgramChunks;
#endif

    // static calls
    effect->dispatcher   = vst_dispatcherCallback;
    effect->process      = vst_processCallback;
    effect->getParameter = vst_getParameterCallback;
    effect->setParameter = vst_setParameterCallback;
    effect->processReplacing = vst_processReplacingCallback;
    effect->processDoubleReplacing = nullptr;

    // pointers
    effect->object = (void*)audioMaster;

    return effect;
}
