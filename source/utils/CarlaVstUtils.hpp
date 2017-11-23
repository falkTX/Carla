/*
 * Carla VST utils
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

#ifndef CARLA_VST_UTILS_HPP_INCLUDED
#define CARLA_VST_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------
// Include fixes

// Disable deprecated VST features (NOT)
#define VST_2_4_EXTENSIONS 1
#define VST_FORCE_DEPRECATED 0

#include "vestige/aeffectx.h"
#define audioMasterGetOutputSpeakerArrangement audioMasterGetSpeakerArrangement
#define effFlagsProgramChunks (1 << 5)
#define effSetProgramName 4
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetVu 9
#define effEditDraw 16
#define effEditMouse 17
#define effEditKey 18
#define effEditSleep 21
#define effIdentify 22
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effString2Parameter 27
#define effGetNumProgramCategories 28
#define effGetProgramNameIndexed 29
#define effCopyProgram 30
#define effConnectInput 31
#define effConnectOutput 32
#define effGetInputProperties 33
#define effGetOutputProperties 34
#define effGetCurrentPosition 36
#define effGetDestinationBuffer 37
#define effOfflineNotify 38
#define effOfflinePrepare 39
#define effOfflineRun 40
#define effProcessVarIo 41
#define effSetSpeakerArrangement 42
#define effSetBlockSizeAndSampleRate 43
#define effSetBypass 44
#define effGetErrorText 46
#define effVendorSpecific 50
#define effGetTailSize 52
#define effGetIcon 54
#define effSetViewPosition 55
#define effKeysRequired 57
#define effEditKeyDown 59
#define effEditKeyUp 60
#define effSetEditKnobMode 61
#define effGetMidiProgramName 62
#define effGetCurrentMidiProgram 63
#define effGetMidiProgramCategory 64
#define effHasMidiProgramsChanged 65
#define effGetMidiKeyName 66
#define effGetSpeakerArrangement 69
#define effSetTotalSampleToProcess 73
#define effSetPanLaw 74
#define effBeginLoadBank 75
#define effBeginLoadProgram 76
#define effSetProcessPrecision 77
#define effGetNumMidiInputChannels 78
#define effGetNumMidiOutputChannels 79
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#define kVstAutomationOff 1
#define kVstAutomationReadWrite 4
#define kVstProcessLevelUnknown 0
#define kVstProcessLevelUser 1
#define kVstProcessLevelRealtime 2
#define kVstProcessLevelOffline 4
#define kVstProcessPrecision32 0
#define kVstTransportChanged 1
#define kVstVersion 2400
#define DECLARE_VST_DEPRECATED(idx) idx
#if defined(CARLA_OS_WIN) && defined(__cdecl)
# define VSTCALLBACK __cdecl
#else
# define VSTCALLBACK
#endif
struct ERect {
    int16_t top, left, bottom, right;
};

// -----------------------------------------------------------------------
// Plugin callback

typedef AEffect* (*VST_Function)(audioMasterCallback);

// -----------------------------------------------------------------------
// Check if feature is supported by the plugin

static inline
bool vstPluginCanDo(AEffect* const effect, const char* const feature) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(effect != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(feature != nullptr && feature[0] != '\0', false);

    try {
        return (effect->dispatcher(effect, effCanDo, 0, 0, const_cast<char*>(feature), 0.0f) == 1);
    } CARLA_SAFE_EXCEPTION_RETURN("vstPluginCanDo", false);
}

// -----------------------------------------------------------------------
// Convert Effect opcode to string

static inline
const char* vstEffectOpcode2str(const int32_t opcode) noexcept
{
    switch (opcode)
    {
    case effOpen:
        return "effOpen";
    case effClose:
        return "effClose";
    case effSetProgram:
        return "effSetProgram";
    case effGetProgram:
        return "effGetProgram";
    case effSetProgramName:
        return "effSetProgramName";
    case effGetProgramName:
        return "effGetProgramName";
    case effGetParamLabel:
        return "effGetParamLabel";
    case effGetParamDisplay:
        return "effGetParamDisplay";
    case effGetParamName:
        return "effGetParamName";
    case DECLARE_VST_DEPRECATED(effGetVu):
        return "effGetVu";
    case effSetSampleRate:
        return "effSetSampleRate";
    case effSetBlockSize:
        return "effSetBlockSize";
    case effMainsChanged:
        return "effMainsChanged";
    case effEditGetRect:
        return "effEditGetRect";
    case effEditOpen:
        return "effEditOpen";
    case effEditClose:
        return "effEditClose";
    case DECLARE_VST_DEPRECATED(effEditDraw):
        return "effEditDraw";
    case DECLARE_VST_DEPRECATED(effEditMouse):
        return "effEditMouse";
    case DECLARE_VST_DEPRECATED(effEditKey):
        return "effEditKey";
    case effEditIdle:
        return "effEditIdle";
    case DECLARE_VST_DEPRECATED(effEditTop):
        return "effEditTop";
    case DECLARE_VST_DEPRECATED(effEditSleep):
        return "effEditSleep";
    case DECLARE_VST_DEPRECATED(effIdentify):
        return "effIdentify";
    case effGetChunk:
        return "effGetChunk";
    case effSetChunk:
        return "effSetChunk";
    case effProcessEvents:
        return "effProcessEvents";
    case effCanBeAutomated:
        return "effCanBeAutomated";
    case effString2Parameter:
        return "effString2Parameter";
    case DECLARE_VST_DEPRECATED(effGetNumProgramCategories):
        return "effGetNumProgramCategories";
    case effGetProgramNameIndexed:
        return "effGetProgramNameIndexed";
    case DECLARE_VST_DEPRECATED(effCopyProgram):
        return "effCopyProgram";
    case DECLARE_VST_DEPRECATED(effConnectInput):
        return "effConnectInput";
    case DECLARE_VST_DEPRECATED(effConnectOutput):
        return "effConnectOutput";
    case effGetInputProperties:
        return "effGetInputProperties";
    case effGetOutputProperties:
        return "effGetOutputProperties";
    case effGetPlugCategory:
        return "effGetPlugCategory";
    case DECLARE_VST_DEPRECATED(effGetCurrentPosition):
        return "effGetCurrentPosition";
    case DECLARE_VST_DEPRECATED(effGetDestinationBuffer):
        return "effGetDestinationBuffer";
    case effOfflineNotify:
        return "effOfflineNotify";
    case effOfflinePrepare:
        return "effOfflinePrepare";
    case effOfflineRun:
        return "effOfflineRun";
    case effProcessVarIo:
        return "effProcessVarIo";
    case effSetSpeakerArrangement:
        return "effSetSpeakerArrangement";
    case DECLARE_VST_DEPRECATED(effSetBlockSizeAndSampleRate):
        return "effSetBlockSizeAndSampleRate";
    case effSetBypass:
        return "effSetBypass";
    case effGetEffectName:
        return "effGetEffectName";
    case DECLARE_VST_DEPRECATED(effGetErrorText):
        return "effGetErrorText";
    case effGetVendorString:
        return "effGetVendorString";
    case effGetProductString:
        return "effGetProductString";
    case effGetVendorVersion:
        return "effGetVendorVersion";
    case effVendorSpecific:
        return "effVendorSpecific";
    case effCanDo:
        return "effCanDo";
    case effGetTailSize:
        return "effGetTailSize";
    case DECLARE_VST_DEPRECATED(effIdle):
        return "effIdle";
    case DECLARE_VST_DEPRECATED(effGetIcon):
        return "effGetIcon";
    case DECLARE_VST_DEPRECATED(effSetViewPosition):
        return "effSetViewPosition";
    case effGetParameterProperties:
        return "effGetParameterProperties";
    case DECLARE_VST_DEPRECATED(effKeysRequired):
        return "effKeysRequired";
    case effGetVstVersion:
        return "effGetVstVersion";
    case effEditKeyDown:
        return "effEditKeyDown";
    case effEditKeyUp:
        return "effEditKeyUp";
    case effSetEditKnobMode:
        return "effSetEditKnobMode";
    case effGetMidiProgramName:
        return "effGetMidiProgramName";
    case effGetCurrentMidiProgram:
        return "effGetCurrentMidiProgram";
    case effGetMidiProgramCategory:
        return "effGetMidiProgramCategory";
    case effHasMidiProgramsChanged:
        return "effHasMidiProgramsChanged";
    case effGetMidiKeyName:
        return "effGetMidiKeyName";
    case effBeginSetProgram:
        return "effBeginSetProgram";
    case effEndSetProgram:
        return "effEndSetProgram";
    case effGetSpeakerArrangement:
        return "effGetSpeakerArrangement";
    case effShellGetNextPlugin:
        return "effShellGetNextPlugin";
    case effStartProcess:
        return "effStartProcess";
    case effStopProcess:
        return "effStopProcess";
    case effSetTotalSampleToProcess:
        return "effSetTotalSampleToProcess";
    case effSetPanLaw:
        return "effSetPanLaw";
    case effBeginLoadBank:
        return "effBeginLoadBank";
    case effBeginLoadProgram:
        return "effBeginLoadProgram";
    case effSetProcessPrecision:
        return "effSetProcessPrecision";
    case effGetNumMidiInputChannels:
        return "effGetNumMidiInputChannels";
    case effGetNumMidiOutputChannels:
        return "effGetNumMidiOutputChannels";
    default:
        return "unknown";
    }
}

// -----------------------------------------------------------------------
// Convert Host/Master opcode to string

static inline
const char* vstMasterOpcode2str(const int32_t opcode) noexcept
{
    switch (opcode)
    {
    case audioMasterAutomate:
        return "audioMasterAutomate";
    case audioMasterVersion:
        return "audioMasterVersion";
    case audioMasterCurrentId:
        return "audioMasterCurrentId";
    case audioMasterIdle:
        return "audioMasterIdle";
    case DECLARE_VST_DEPRECATED(audioMasterPinConnected):
        return "audioMasterPinConnected";
    case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
        return "audioMasterWantMidi";
    case audioMasterGetTime:
        return "audioMasterGetTime";
    case audioMasterProcessEvents:
        return "audioMasterProcessEvents";
    case DECLARE_VST_DEPRECATED(audioMasterSetTime):
        return "audioMasterSetTime";
    case DECLARE_VST_DEPRECATED(audioMasterTempoAt):
        return "audioMasterTempoAt";
    case DECLARE_VST_DEPRECATED(audioMasterGetNumAutomatableParameters):
        return "audioMasterGetNumAutomatableParameters";
    case DECLARE_VST_DEPRECATED(audioMasterGetParameterQuantization):
        return "audioMasterGetParameterQuantization";
    case audioMasterIOChanged:
        return "audioMasterIOChanged";
    case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
        return "audioMasterNeedIdle";
    case audioMasterSizeWindow:
        return "audioMasterSizeWindow";
    case audioMasterGetSampleRate:
        return "audioMasterGetSampleRate";
    case audioMasterGetBlockSize:
        return "audioMasterGetBlockSize";
    case audioMasterGetInputLatency:
        return "audioMasterGetInputLatency";
    case audioMasterGetOutputLatency:
        return "audioMasterGetOutputLatency";
    case DECLARE_VST_DEPRECATED(audioMasterGetPreviousPlug):
        return "audioMasterGetPreviousPlug";
    case DECLARE_VST_DEPRECATED(audioMasterGetNextPlug):
        return "audioMasterGetNextPlug";
    case DECLARE_VST_DEPRECATED(audioMasterWillReplaceOrAccumulate):
        return "audioMasterWillReplaceOrAccumulate";
    case audioMasterGetCurrentProcessLevel:
        return "audioMasterGetCurrentProcessLevel";
    case audioMasterGetAutomationState:
        return "audioMasterGetAutomationState";
    case audioMasterOfflineStart:
        return "audioMasterOfflineStart";
    case audioMasterOfflineRead:
        return "audioMasterOfflineRead";
    case audioMasterOfflineWrite:
        return "audioMasterOfflineWrite";
    case audioMasterOfflineGetCurrentPass:
        return "audioMasterOfflineGetCurrentPass";
    case audioMasterOfflineGetCurrentMetaPass:
        return "audioMasterOfflineGetCurrentMetaPass";
    case DECLARE_VST_DEPRECATED(audioMasterSetOutputSampleRate):
        return "audioMasterSetOutputSampleRate";
    case DECLARE_VST_DEPRECATED(audioMasterGetOutputSpeakerArrangement):
        return "audioMasterGetOutputSpeakerArrangement";
    case audioMasterGetVendorString:
        return "audioMasterGetVendorString";
    case audioMasterGetProductString:
        return "audioMasterGetProductString";
    case audioMasterGetVendorVersion:
        return "audioMasterGetVendorVersion";
    case audioMasterVendorSpecific:
        return "audioMasterVendorSpecific";
    case DECLARE_VST_DEPRECATED(audioMasterSetIcon):
        return "audioMasterSetIcon";
    case audioMasterCanDo:
        return "audioMasterCanDo";
    case audioMasterGetLanguage:
        return "audioMasterGetLanguage";
    case DECLARE_VST_DEPRECATED(audioMasterOpenWindow):
        return "audioMasterOpenWindow";
    case DECLARE_VST_DEPRECATED(audioMasterCloseWindow):
        return "audioMasterCloseWindow";
    case audioMasterGetDirectory:
        return "audioMasterGetDirectory";
    case audioMasterUpdateDisplay:
        return "audioMasterUpdateDisplay";
    case audioMasterBeginEdit:
        return "audioMasterBeginEdit";
    case audioMasterEndEdit:
        return "audioMasterEndEdit";
    case audioMasterOpenFileSelector:
        return "audioMasterOpenFileSelector";
    case audioMasterCloseFileSelector:
        return "audioMasterCloseFileSelector";
    case DECLARE_VST_DEPRECATED(audioMasterEditFile):
        return "audioMasterEditFile";
    case DECLARE_VST_DEPRECATED(audioMasterGetChunkFile):
        return "audioMasterGetChunkFile";
    case DECLARE_VST_DEPRECATED(audioMasterGetInputSpeakerArrangement):
        return "audioMasterGetInputSpeakerArrangement";
    default:
        return "unknown";
    }
}

// -----------------------------------------------------------------------

#endif // CARLA_VST_UTILS_HPP_INCLUDED
