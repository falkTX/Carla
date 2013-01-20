/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#ifdef DISTRHO_PLUGIN_TARGET_VST

#include "DistrhoPluginInternal.h"

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.h"
#endif

#define VST_FORCE_DEPRECATED 0
#include <pluginterfaces/vst2.x/aeffectx.h>

// -------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI
class UIVst
{
public:
    UIVst(audioMasterCallback audioMaster, AEffect* effect, PluginInternal* plugin, intptr_t winId)
        : m_audioMaster(audioMaster),
          m_effect(effect),
          m_plugin(plugin),
          ui(this, winId, setParameterCallback, setStateCallback, uiEditParameterCallback, uiSendNoteCallback, uiResizeCallback)
    {
        uint32_t paramCount = plugin->parameterCount();

        if (paramCount > 0)
        {
            parameterChecks = new bool [paramCount];
            parameterValues = new float [paramCount];

            for (uint32_t i=0; i < paramCount; i++)
            {
                parameterChecks[i] = false;
                parameterValues[i] = 0.0f;
            }
        }
        else
        {
            parameterChecks = nullptr;
            parameterValues = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    nextProgram = -1;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        midiEventCount = 0;
#endif
    }

    ~UIVst()
    {
        if (parameterChecks)
            delete[] parameterChecks;

        if (parameterValues)
            delete[] parameterValues;
    }

    // ---------------------------------------------

    void idle()
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (nextProgram != -1)
        {
            ui.programChanged(nextProgram);
            nextProgram = -1;
        }
#endif

        for (uint32_t i=0, count = m_plugin->parameterCount(); i < count; i++)
        {
            if (parameterChecks[i])
            {
                parameterChecks[i] = false;

                ui.parameterChanged(i, parameterValues[i]);
            }
        }

#if DISTRHO_PLUGIN_IS_SYNTH
        // TODO - notes
#endif

        ui.idle();
    }

    int16_t getWidth()
    {
        return ui.getWidth();
    }

    int16_t getHeight()
    {
        return ui.getHeight();
    }

    void setParameterValueFromPlugin(uint32_t index, float perValue)
    {
        parameterChecks[index] = true;
        parameterValues[index] = perValue;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setProgramFromPlugin(uint32_t index)
    {
        nextProgram = index;

        // set previous parameters invalid
        for (uint32_t i=0, count = m_plugin->parameterCount(); i < count; i++)
            parameterChecks[i] = false;
    }
#endif

    // ---------------------------------------------

protected:
    intptr_t hostCallback(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return m_audioMaster(m_effect, opcode, index, value, ptr, opt);
    }

    void setParameterValue(uint32_t index, float realValue)
    {
        const ParameterRanges* ranges = m_plugin->parameterRanges(index);
        float perValue = (realValue - ranges->min) / (ranges->max - ranges->min);

        m_plugin->setParameterValue(index, realValue);

        hostCallback(audioMasterAutomate, index, 0, nullptr, perValue);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* key, const char* value)
    {
        m_plugin->setState(key, value);
    }
#endif

    void uiEditParameter(uint32_t index, bool started)
    {
        if (started)
            hostCallback(audioMasterBeginEdit, index, 0, nullptr, 0.0f);
        else
            hostCallback(audioMasterEndEdit, index, 0, nullptr, 0.0f);
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void uiSendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        // TODO
    }
#endif

    void uiResize(unsigned int width, unsigned int height)
    {
        hostCallback(audioMasterSizeWindow, width, height, nullptr, 0.0f);
    }

private:
    // Vst stuff
    audioMasterCallback const m_audioMaster;
    AEffect* const m_effect;
    PluginInternal* const m_plugin;

    // Plugin UI
    UIInternal ui;

    // Temporary data
    bool*  parameterChecks;
    float* parameterValues;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t nextProgram;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t midiEventCount;
    MidiEvent midiEvents[MAX_MIDI_EVENTS];
#endif

    // ---------------------------------------------
    // Callbacks

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        UIVst* _this_ = (UIVst*)ptr;
        assert(_this_);

        _this_->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        UIVst* _this_ = (UIVst*)ptr;
        assert(_this_);

        _this_->setState(key, value);
#else
        // unused
        (void)ptr;
        (void)key;
        (void)value;
#endif
    }

    static void uiEditParameterCallback(void* ptr, uint32_t index, bool started)
    {
        UIVst* _this_ = (UIVst*)ptr;
        assert(_this_);

        _this_->uiEditParameter(index, started);
    }

    static void uiSendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        UIVst* _this_ = (UIVst*)ptr;
        assert(_this_);

        _this_->uiSendNote(onOff, channel, note, velocity);
#else
        // unused
        (void)ptr;
        (void)onOff;
        (void)channel;
        (void)note;
        (void)velocity;
#endif
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        UIVst* _this_ = (UIVst*)ptr;
        assert(_this_);

        _this_->uiResize(width, height);
    }
};
#endif

class PluginVst
{
public:
    PluginVst(audioMasterCallback audioMaster, AEffect* effect)
        : m_audioMaster(audioMaster),
          m_effect(effect)
    {
#if DISTRHO_PLUGIN_HAS_UI
        vstui = nullptr;
        rect.top  = 0;
        rect.left = 0;
        rect.bottom = 0;
        rect.right  = 0;
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        curProgram = -1;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
        midiEventCount = 0;
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
            if (value >= 0 && value < plugin.programCount())
            {
                curProgram = value;
                plugin.setProgram(curProgram);

                if (vstui)
                    vstui->setProgramFromPlugin(curProgram);

                ret = 1;
            }
            break;

        case effGetProgram:
            ret = curProgram;
            break;

        case effSetProgramName:
            break;

        case effGetProgramName:
            if (ptr && curProgram >= 0 && curProgram < (int32_t)plugin.programCount())
            {
                strncpy((char*)ptr, plugin.programName(curProgram), kVstMaxProgNameLen);
                ret = 1;
            }
            break;
#endif

        case effGetParamDisplay:
            if (ptr && index < (int32_t)plugin.parameterCount())
            {
                snprintf((char*)ptr, kVstMaxParamStrLen, "%f", plugin.parameterValue(index));
                ret = 1;
            }
            break;

        case effSetSampleRate:
            // should not happen
            break;

        case effSetBlockSize:
            plugin.setBufferSize(value, true);
            break;

        case effMainsChanged:
            if (value)
                plugin.activate();
            else
                plugin.deactivate();
            break;

#if DISTRHO_PLUGIN_HAS_UI
        case effEditGetRect:
            if (rect.bottom == 0 && ! vstui)
            {
                // This is stupid, but some hosts want to know the UI size before creating it,
                // so we have to create a temporary UI here
                setLastUiSampleRate(d_lastSampleRate);

                UIInternal tempUI(nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
                rect.bottom = tempUI.getHeight();
                rect.right  = tempUI.getWidth();
            }
            else
            {
                rect.bottom = vstui->getHeight();
                rect.right  = vstui->getWidth();
            }

            *(ERect**)ptr = &rect;
            ret = 1;

            break;

        case effEditOpen:
            if (! vstui)
            {
                setLastUiSampleRate(d_lastSampleRate);
                vstui = new UIVst(m_audioMaster, m_effect, &plugin, (intptr_t)ptr);
            }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
            if (curProgram >= 0)
                vstui->setProgramFromPlugin(curProgram);
#endif

            for (uint32_t i=0, count = plugin.parameterCount(); i < count; i++)
                vstui->setParameterValueFromPlugin(i, plugin.parameterValue(i));

            ret = 1;
            break;

        case effEditClose:
            if (vstui)
            {
                delete vstui;
                vstui = nullptr;
                ret = 1;
            }
            break;

        case effEditIdle:
            if (vstui)
                vstui->idle();
            break;
#endif

        case effGetChunk:
        case effSetChunk:
            // TODO
            break;

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
            if (index < (int32_t)plugin.parameterCount())
            {
                uint32_t hints = plugin.parameterHints(index);

                // must be automable, and not output
                if ((hints & PARAMETER_IS_AUTOMABLE) > 0 && (hints & PARAMETER_IS_OUTPUT) == 0)
                    ret = 1;
            }
            break;

        case effCanDo:
            // TODO
            break;

        case effStartProcess:
        case effStopProcess:
            break;
        }

        return ret;

        // unused
        (void)opt;
    }

    float vst_getParameter(int32_t index)
    {
        const ParameterRanges* ranges = plugin.parameterRanges(index);
        float realValue = plugin.parameterValue(index);
        float perValue  = (realValue - ranges->min) / (ranges->max - ranges->min);
        return perValue;
    }

    void vst_setParameter(int32_t index, float value)
    {
        const ParameterRanges* ranges = plugin.parameterRanges(index);
        float realValue = ranges->min + (ranges->max - ranges->min) * value;
        plugin.setParameterValue(index, realValue);

        if (vstui)
            vstui->setParameterValueFromPlugin(index, realValue);
    }

    void vst_processReplacing(float** inputs, float** outputs, int32_t sampleFrames)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        plugin.run((const float**)inputs, outputs, sampleFrames, midiEventCount, midiEvents);

        // TODO - send notes to UI

#else
        plugin.run((const float**)inputs, outputs, sampleFrames, 0, nullptr);
#endif
    }

    // ---------------------------------------------

private:
    // VST stuff
    audioMasterCallback const m_audioMaster;
    AEffect* const m_effect;

    // Plugin
    PluginInternal plugin;

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    UIVst* vstui;
    ERect rect;
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t curProgram;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t midiEventCount;
    MidiEvent midiEvents[MAX_MIDI_EVENTS];
#endif
};

// -------------------------------------------------

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

    // handle opcodes
    switch (opcode)
    {
    case effOpen:
        if (! effect->object)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->user;
            d_lastBufferSize = audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);
            d_lastSampleRate = audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
            effect->object   = new PluginVst(audioMaster, effect);
            return 1;
        }
        return 0;

    case effClose:
        if (effect->object)
        {
            delete (PluginVst*)effect->object;
            effect->object = nullptr;
            delete effect;
            return 1;
        }
        return 0;

    case effGetParamLabel:
        if (ptr && index < (int32_t)plugin.parameterCount())
        {
            strncpy((char*)ptr, plugin.parameterUnit(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

    case effGetParamName:
        if (ptr && index < (int32_t)plugin.parameterCount())
        {
            strncpy((char*)ptr, plugin.parameterName(index), kVstMaxParamStrLen);
            return 1;
        }
        return 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    case effGetProgramNameIndexed:
        if (ptr && index < (int32_t)plugin.programCount())
        {
            strncpy((char*)ptr, plugin.programName(index), kVstMaxProgNameLen);
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
        if (ptr)
        {
            strncpy((char*)ptr, plugin.name(), kVstMaxProductStrLen);
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (ptr)
        {
            strncpy((char*)ptr, plugin.maker(), kVstMaxVendorStrLen);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (ptr)
        {
            strncpy((char*)ptr, plugin.label(), kVstMaxEffectNameLen);
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return plugin.version();

    case effGetVstVersion:
        return kVstVersion;
    };

    if (effect->object)
    {
        PluginVst* _this_ = (PluginVst*)effect->object;
        return _this_->vst_dispatcher(opcode, index, value, ptr, opt);
    }

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    PluginVst* _this_ = (PluginVst*)effect->object;
    assert(_this_);

    if (_this_)
        return _this_->vst_getParameter(index);

    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    PluginVst* _this_ = (PluginVst*)effect->object;
    assert(_this_);

    if (_this_)
        _this_->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    PluginVst* _this_ = (PluginVst*)effect->object;
    assert(_this_);

    if (_this_)
        _this_->vst_processReplacing(inputs, outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    PluginVst* _this_ = (PluginVst*)effect->object;
    assert(_this_);

    if (_this_)
        _this_->vst_processReplacing(inputs, outputs, sampleFrames);
}

END_NAMESPACE_DISTRHO

// -------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    USE_NAMESPACE_DISTRHO

    // old version
    if (! audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f))
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
# ifdef DISTRHO_UI_QT4
    if (QApplication::instance())
# endif
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

#endif // DISTRHO_PLUGIN_TARGET_VST
