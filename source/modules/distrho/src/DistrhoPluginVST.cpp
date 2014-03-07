/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

// has some conflicts
#ifdef noexcept
# undef noexcept
#endif

#define VESTIGE_HEADER
#define VST_FORCE_DEPRECATED 0

#include <map>
#include <string>

#ifdef VESTIGE_HEADER
# include "vestige/aeffectx.h"
#define effFlagsProgramChunks (1 << 5)
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effGetProgramNameIndexed 29
#define effGetPlugCategory 35
#define effIdle 53
#define kPlugCategEffect 1
#define kPlugCategSynth 2
#define kVstVersion 2400
struct ERect {
    int16_t top, left, bottom, right;
};
#else
# include "vst/aeffectx.h"
#endif

#if DISTRHO_PLUGIN_WANT_STATE
# warning VST State still TODO (working but needs final testing)
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
# warning VST TimePos still TODO (only basic BBT working)
#endif

typedef std::map<d_string,d_string> StringMap;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

void strncpy(char* const dst, const char* const src, const size_t size)
{
    std::strncpy(dst, src, size);
    dst[size] = '\0';
}

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------

class UiHelper
{
public:
    UiHelper()
        : parameterChecks(nullptr),
          parameterValues(nullptr),
          nextProgram(-1) {}

    virtual ~UiHelper()
    {
        if (parameterChecks != nullptr)
        {
            delete[] parameterChecks;
            parameterChecks = nullptr;
        }
        if (parameterValues != nullptr)
        {
            delete[] parameterValues;
            parameterValues = nullptr;
        }
    }

    bool*   parameterChecks;
    float*  parameterValues;
    int32_t nextProgram;

#if DISTRHO_PLUGIN_WANT_STATE
    virtual void setStateFromUi(const char* const newKey, const char* const newValue) = 0;
#endif
};

// -----------------------------------------------------------------------

class UIVst
{
public:
    UIVst(const audioMasterCallback audioMaster, AEffect* const effect, UiHelper* const uiHelper, PluginExporter* const plugin, const intptr_t winId)
        : fAudioMaster(audioMaster),
          fEffect(effect),
          fUiHelper(uiHelper),
          fPlugin(plugin),
          fUI(this, winId, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback)
    {
    }

    // -------------------------------------------------------------------

    void idle()
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fUiHelper->nextProgram != -1)
        {
            fUI.programChanged(fUiHelper->nextProgram);
            fUiHelper->nextProgram = -1;
        }
#endif

        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
        {
            if (fUiHelper->parameterChecks[i])
            {
                fUiHelper->parameterChecks[i] = false;
                fUI.parameterChanged(i, fUiHelper->parameterValues[i]);
            }
        }

        fUI.idle();
    }

    int16_t getWidth() const
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const
    {
        return fUI.getHeight();
    }

    // -------------------------------------------------------------------
    // functions called from the plugin side, may block

#if DISTRHO_PLUGIN_WANT_STATE
    void setStateFromPlugin(const char* const key, const char* const value)
    {
        fUI.stateChanged(key, value);
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
        fUiHelper->setStateFromUi(key, value);
#else
        return; // unused
        (void)key;
        (void)value;
#endif
    }

    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
#if 0 //DISTRHO_PLUGIN_IS_SYNTH
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
    UiHelper* const fUiHelper;
    PluginExporter* const fPlugin;

    // Plugin UI
    UIExporter fUI;

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

#if DISTRHO_PLUGIN_HAS_UI
class PluginVst : public UiHelper
#else
class PluginVst
#endif
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

        if (const uint32_t paramCount = fPlugin.getParameterCount())
        {
            parameterChecks = new bool[paramCount];
            parameterValues = new float[paramCount];

            for (uint32_t i=0; i < paramCount; ++i)
            {
                parameterChecks[i] = false;
                parameterValues[i] = 0.0f;
            }
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        fStateChunk = nullptr;
#endif
    }

#if DISTRHO_PLUGIN_WANT_STATE
    ~PluginVst()
    {
        if (fStateChunk != nullptr)
        {
            delete[] fStateChunk;
            fStateChunk = nullptr;
        }

        fStateMap.clear();
    }
#endif

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
                    setProgramFromPlugin(fCurProgram);
#endif

                ret = 1;
            }
            break;

        case effGetProgram:
            ret = fCurProgram;
            break;

        //case effSetProgramName:
        // unsupported
        //    break;

        case effGetProgramName:
            if (ptr != nullptr && fCurProgram >= 0 && fCurProgram < static_cast<int32_t>(fPlugin.getProgramCount()))
            {
                DISTRHO::strncpy((char*)ptr, fPlugin.getProgramName(fCurProgram), 24);
                ret = 1;
            }
            break;
#endif

        case effGetParamDisplay:
            if (ptr != nullptr && index < static_cast<int32_t>(fPlugin.getParameterCount()))
            {
                char* buf = (char*)ptr;
                std::snprintf((char*)ptr, 8, "%f", fPlugin.getParameterValue(index));
                buf[8] = '\0';
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
            if (value != 0)
            {
                fPlugin.activate();
#if DISTRHO_PLUGIN_IS_SYNTH
                fMidiEventCount = 0;
#endif
            }
            else
            {
                fPlugin.deactivate();
            }
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
# if DISTRHO_OS_MAC && ! defined(__LP64__)
                if ((fEffect->dispatcher(fEffect, effCanDo, 0, 0, (void*)"hasCockosViewAsConfig", 0.0f) & 0xffff0000) != 0xbeef0000)
                    return 0;
# endif

                d_lastUiSampleRate = fAudioMaster(fEffect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);

                fVstUi = new UIVst(fAudioMaster, fEffect, this, &fPlugin, (intptr_t)ptr);

# if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (fCurProgram >= 0)
                    setProgramFromPlugin(fCurProgram);
# endif
                for (uint32_t i=0, count = fPlugin.getParameterCount(); i < count; ++i)
                    setParameterValueFromPlugin(i, fPlugin.getParameterValue(i));

                fVstUi->idle();

#if DISTRHO_PLUGIN_WANT_STATE
                if (fStateMap.size() > 0)
                {
                    for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
                    {
                        const d_string& key   = it->first;
                        const d_string& value = it->second;

                        fVstUi->setStateFromPlugin((const char*)key, (const char*)value);
                    }

                    fVstUi->idle();
                }
#endif
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
        case effIdle:
            if (fVstUi != nullptr)
                fVstUi->idle();
            break;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        case effGetChunk:
            if (ptr == nullptr)
                return 0;

            if (fStateChunk != nullptr)
                delete[] fStateChunk;

            if (fStateMap.size() == 0)
            {
                fStateChunk    = new char[1];
                fStateChunk[0] = '\0';
                ret = 1;
            }
            else
            {
                std::string tmpStr;

                for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
                {
                    const d_string& key   = it->first;
                    const d_string& value = it->second;

                    tmpStr += (const char*)key;
                    tmpStr += "\xff";
                    tmpStr += (const char*)value;
                    tmpStr += "\xff";
                }

                const size_t size(tmpStr.size());
                fStateChunk = new char[size];
                std::memcpy(fStateChunk, tmpStr.c_str(), size*sizeof(char));

                for (size_t i=0; i < size; ++i)
                {
                    if (fStateChunk[i] == '\xff')
                        fStateChunk[i] = '\0';
                }

                ret = size;
            }

            *(void**)ptr = fStateChunk;
            break;

        case effSetChunk:
            if (value <= 0)
                return 0;
            if (value == 1)
                return 1;

            if (const char* const state = (const char*)ptr)
            {
                const size_t stateSize  = value;
                const char*  stateKey   = state;
                const char*  stateValue = nullptr;

                for (size_t i=0; i < stateSize; ++i)
                {
                    // find next null char
                    if (state[i] != '\0')
                        continue;

                    // found, set value
                    stateValue = &state[i+1];
                    setSharedState(stateKey, stateValue);

                    if (fVstUi != nullptr)
                        fVstUi->setStateFromPlugin(stateKey, stateValue);

                    // increment text position
                    i += std::strlen(stateValue) + 2;

                    // check if end of data
                    if (i >= stateSize)
                        break;

                    // get next key
                    stateKey = &state[i];
                }

                ret = 1;
            }

            break;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        case effProcessEvents:
            if (const VstEvents* const events = (const VstEvents*)ptr)
            {
                if (events->numEvents == 0)
                    break;

                for (int i=0, count=events->numEvents; i < count; ++i)
                {
                    const VstMidiEvent* const vstMidiEvent((const VstMidiEvent*)events->events[i]);

                    if (vstMidiEvent == nullptr)
                        break;
                    if (vstMidiEvent->type != kVstMidiType)
                        continue;
                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;

                    MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
                    midiEvent.frame  = vstMidiEvent->deltaFrames;
                    midiEvent.size   = 3;
                    std::memcpy(midiEvent.buf, vstMidiEvent->midiData, 3*sizeof(uint8_t));
                }
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

#if (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_TIMEPOS)
        case effCanDo:
            if (const char* const canDo = (const char*)ptr)
            {
# if DISTRHO_PLUGIN_IS_SYNTH
                if (std::strcmp(canDo, "receiveVstEvents") == 0)
                    return 1;
                if (std::strcmp(canDo, "receiveVstMidiEvent") == 0)
                    return 1;
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
                if (std::strcmp(canDo, "receiveVstTimeInfo") == 0)
                    return 1;
# endif
            }
            break;
#endif

        //case effStartProcess:
        //case effStopProcess:
        // unused
        //    break;
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
            setParameterValueFromPlugin(index, realValue);
#endif
    }

    void vst_processReplacing(float** const inputs, float** const outputs, const int32_t sampleFrames)
    {
#if DISTRHO_PLUGIN_WANT_TIMEPOS
        static const int kWantedVstTimeFlags(kVstTransportPlaying|kVstTempoValid|kVstTimeSigValid);

        if (const VstTimeInfo* const vstTimeInfo = (const VstTimeInfo*)fAudioMaster(fEffect, audioMasterGetTime, 0, kWantedVstTimeFlags, nullptr, 0.0f))
        {
            fTimePos.playing = (vstTimeInfo->flags & kVstTransportPlaying);
            fTimePos.frame   = vstTimeInfo->samplePos;

            if (vstTimeInfo->flags & kVstTempoValid)
            {
                fTimePos.bbt.valid = true;
                fTimePos.bbt.beatsPerMinute = vstTimeInfo->tempo;
            }
            if (vstTimeInfo->flags & kVstTimeSigValid)
            {
                fTimePos.bbt.valid = true;
                fTimePos.bbt.beatsPerBar = vstTimeInfo->timeSigNumerator;
                fTimePos.bbt.beatType    = vstTimeInfo->timeSigDenominator;
            }

            fPlugin.setTimePos(fTimePos);
        }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fPlugin.run(inputs, outputs, sampleFrames, fMidiEvents, fMidiEventCount);
        fMidiEventCount = 0;
#else
        fPlugin.run(inputs, outputs, sampleFrames);
#endif

#if DISTRHO_PLUGIN_HAS_UI
        if (fVstUi == nullptr)
            return;

        for (uint32_t i=0, count = fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
                setParameterValueFromPlugin(i, fPlugin.getParameterValue(i));
        }
#endif
    }

    // -------------------------------------------------------------------

    friend class UIVst;

private:
    // VST stuff
    const audioMasterCallback fAudioMaster;
    AEffect* const fEffect;

    // Plugin
    PluginExporter fPlugin;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    // Current state
    int32_t fCurProgram;
#endif

    // Temporary data
#if DISTRHO_PLUGIN_IS_SYNTH
    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePos fTimePos;
#endif

    // UI stuff
#if DISTRHO_PLUGIN_HAS_UI
    UIVst* fVstUi;
    ERect  fVstRect;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    char*     fStateChunk;
    StringMap fStateMap;
#endif

    // -------------------------------------------------------------------
    // functions called from the plugin side, RT no block

    void setParameterValueFromPlugin(const uint32_t index, const float realValue)
    {
        parameterValues[index] = realValue;
        parameterChecks[index] = true;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setProgramFromPlugin(const uint32_t index)
    {
        // set previous parameters invalid
        for (uint32_t i=0, count = fPlugin.getParameterCount(); i < count; ++i)
            parameterChecks[i] = false;

        nextProgram = index;
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    // -------------------------------------------------------------------
    // functions called from the UI side, may block

    void setStateFromUi(const char* const newKey, const char* const newValue) override
    {
        fPlugin.setState(newKey, newValue);

        // check if key already exists
        for (auto it = fStateMap.begin(), end = fStateMap.end(); it != end; ++it)
        {
            const d_string& key = it->first;

            if (key == newKey)
            {
                it->second = newValue;
                return;
            }
        }

        // add a new one then
        d_string d_key(newKey);
        fStateMap[d_key] = newValue;
    }
#endif
};

// -----------------------------------------------------------------------

#ifdef VESTIGE_HEADER
# define handlePtr   ((PluginVst*)effect->ptr2)
# define validEffect effect != nullptr && effect->ptr2 != nullptr
#else
# define handlePtr   ((PluginVst*)effect->resvd2)
# define validEffect effect != nullptr && effect->resvd2 != 0
#endif

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
#ifdef VESTIGE_HEADER
        if (effect != nullptr && effect->ptr3 != nullptr)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->ptr3;
#else
        if (effect != nullptr && effect->object != nullptr)
        {
            audioMasterCallback audioMaster = (audioMasterCallback)effect->object;
#endif
            d_lastBufferSize = audioMaster(effect, audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);
            d_lastSampleRate = audioMaster(effect, audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
            PluginVst* const plugin(new PluginVst(audioMaster, effect));
#ifdef VESTIGE_HEADER
            effect->ptr2 = plugin;
#else
            effect->resvd2 = (intptr_t)plugin;
#endif
            return 1;
        }
        return 0;

    case effClose:
        if (validEffect)
        {
#ifdef VESTIGE_HEADER
            delete (PluginVst*)effect->ptr2;
            effect->ptr2 = nullptr;
            effect->ptr3 = nullptr;
#else
            delete (PluginVst*)effect->resvd2;
            effect->resvd2 = 0;
            effect->object = nullptr;
#endif
            delete effect;
            return 1;
        }
        return 0;

    case effGetParamLabel:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getParameterUnit(index), 8);
            return 1;
        }
        return 0;

    case effGetParamName:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getParameterCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getParameterName(index), 8);
            return 1;
        }
        return 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    case effGetProgramNameIndexed:
        if (ptr != nullptr && index < static_cast<int32_t>(plugin.getProgramCount()))
        {
            DISTRHO::strncpy((char*)ptr, plugin.getProgramName(index), 24);
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
            DISTRHO::strncpy((char*)ptr, plugin.getName(), 64);
            return 1;
        }
        return 0;

    case effGetVendorString:
        if (ptr != nullptr)
        {
            DISTRHO::strncpy((char*)ptr, plugin.getMaker(), 64);
            return 1;
        }
        return 0;

    case effGetProductString:
        if (ptr != nullptr)
        {
            DISTRHO::strncpy((char*)ptr, plugin.getLabel(), 32);
            return 1;
        }
        return 0;

    case effGetVendorVersion:
        return plugin.getVersion();

    case effGetVstVersion:
        return kVstVersion;
    };

    // handle advanced opcodes
    if (validEffect)
        return handlePtr->vst_dispatcher(opcode, index, value, ptr, opt);

    return 0;
}

static float vst_getParameterCallback(AEffect* effect, int32_t index)
{
    if (validEffect)
        return handlePtr->vst_getParameter(index);
    return 0.0f;
}

static void vst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    if (validEffect)
        handlePtr->vst_setParameter(index, value);
}

static void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validEffect)
        handlePtr->vst_processReplacing(inputs, outputs, sampleFrames);
}

static void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    if (validEffect)
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
#ifdef VESTIGE_HEADER
    *(int32_t*)&effect->unknown1 = plugin->getVersion();
#else
    effect->version  = plugin->getVersion();
#endif

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

    // pointers
#ifdef VESTIGE_HEADER
    effect->ptr3   = (void*)audioMaster;
#else
    effect->object = (void*)audioMaster;
#endif

    return effect;
}
