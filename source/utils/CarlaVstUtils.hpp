/*
 * Carla VST utils
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

#ifndef CARLA_VST_UTILS_HPP_INCLUDED
#define CARLA_VST_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------
// Include fixes

// Define __cdecl if needed
#ifndef CARLA_OS_WIN
# ifndef __cdecl
#  define __cdecl
# endif
#endif

// Disable deprecated VST features (NOT)
#define VST_2_4_EXTENSIONS 1
#define VST_FORCE_DEPRECATED 0

#ifdef VESTIGE_HEADER
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
#define effGetPlugCategory 35
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
#define effIdle 53
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
#define effBeginSetProgram 67
#define effEndSetProgram 68
#define effGetSpeakerArrangement 69
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
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
#define VSTCALLBACK __cdecl
struct ERect {
    int16_t top, left, bottom, right;
};
struct VstTimeInfo_R {
    double samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos;
    int32_t timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags;
};
#else
#ifndef CARLA_OS_MAC
# undef TARGET_API_MAC_CARBON
# define TARGET_API_MAC_CARBON 0
#endif
#undef VST_64BIT_PLATFORM
#define VST_64BIT_PLATFORM (defined(_WIN64) || defined(__LP64__) || defined (_LP64))
#include "vst/pluginterfaces/vst2.x/aeffectx.h"
typedef VstTimeInfo VstTimeInfo_R;
#endif

// -----------------------------------------------------------------------
// Plugin callback

typedef AEffect* (*VST_Function)(audioMasterCallback);

// -----------------------------------------------------------------------
// Check if feature is supported by the plugin

static inline
bool vstPluginCanDo(AEffect* const effect, const char* const feature) noexcept
{
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
#if ! VST_FORCE_DEPRECATED
    case effGetVu:
        return "effGetVu";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case effEditDraw:
        return "effEditDraw";
    case effEditMouse:
        return "effEditMouse";
    case effEditKey:
        return "effEditKey";
#endif
    case effEditIdle:
        return "effEditIdle";
#if ! VST_FORCE_DEPRECATED
    case effEditTop:
        return "effEditTop";
    case effEditSleep:
        return "effEditSleep";
    case effIdentify:
        return "effIdentify";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case effGetNumProgramCategories:
        return "effGetNumProgramCategories";
#endif
    case effGetProgramNameIndexed:
        return "effGetProgramNameIndexed";
#if ! VST_FORCE_DEPRECATED
    case effCopyProgram:
        return "effCopyProgram";
    case effConnectInput:
        return "effConnectInput";
    case effConnectOutput:
        return "effConnectOutput";
#endif
    case effGetInputProperties:
        return "effGetInputProperties";
    case effGetOutputProperties:
        return "effGetOutputProperties";
    case effGetPlugCategory:
        return "effGetPlugCategory";
#if ! VST_FORCE_DEPRECATED
    case effGetCurrentPosition:
        return "effGetCurrentPosition";
    case effGetDestinationBuffer:
        return "effGetDestinationBuffer";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case effSetBlockSizeAndSampleRate:
        return "effSetBlockSizeAndSampleRate";
#endif
    case effSetBypass:
        return "effSetBypass";
    case effGetEffectName:
        return "effGetEffectName";
#if ! VST_FORCE_DEPRECATED
    case effGetErrorText:
        return "effGetErrorText";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case effIdle:
        return "effIdle";
    case effGetIcon:
        return "effGetIcon";
    case effSetViewPosition:
        return "effSetViewPosition";
#endif
    case effGetParameterProperties:
        return "effGetParameterProperties";
#if ! VST_FORCE_DEPRECATED
    case effKeysRequired:
        return "effKeysRequired";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case audioMasterPinConnected:
        return "audioMasterPinConnected";
    case audioMasterWantMidi:
        return "audioMasterWantMidi";
#endif
    case audioMasterGetTime:
        return "audioMasterGetTime";
    case audioMasterProcessEvents:
        return "audioMasterProcessEvents";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetTime:
        return "audioMasterSetTime";
    case audioMasterTempoAt:
        return "audioMasterTempoAt";
    case audioMasterGetNumAutomatableParameters:
        return "audioMasterGetNumAutomatableParameters";
    case audioMasterGetParameterQuantization:
        return "audioMasterGetParameterQuantization";
#endif
    case audioMasterIOChanged:
        return "audioMasterIOChanged";
#if ! VST_FORCE_DEPRECATED
    case audioMasterNeedIdle:
        return "audioMasterNeedIdle";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case audioMasterGetPreviousPlug:
        return "audioMasterGetPreviousPlug";
    case audioMasterGetNextPlug:
        return "audioMasterGetNextPlug";
    case audioMasterWillReplaceOrAccumulate:
        return "audioMasterWillReplaceOrAccumulate";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetOutputSampleRate:
        return "audioMasterSetOutputSampleRate";
    case audioMasterGetOutputSpeakerArrangement:
        return "audioMasterGetOutputSpeakerArrangement";
#endif
    case audioMasterGetVendorString:
        return "audioMasterGetVendorString";
    case audioMasterGetProductString:
        return "audioMasterGetProductString";
    case audioMasterGetVendorVersion:
        return "audioMasterGetVendorVersion";
    case audioMasterVendorSpecific:
        return "audioMasterVendorSpecific";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetIcon:
        return "audioMasterSetIcon";
#endif
    case audioMasterCanDo:
        return "audioMasterCanDo";
    case audioMasterGetLanguage:
        return "audioMasterGetLanguage";
#if ! VST_FORCE_DEPRECATED
    case audioMasterOpenWindow:
        return "audioMasterOpenWindow";
    case audioMasterCloseWindow:
        return "audioMasterCloseWindow";
#endif
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
#if ! VST_FORCE_DEPRECATED
    case audioMasterEditFile:
        return "audioMasterEditFile";
    case audioMasterGetChunkFile:
        return "audioMasterGetChunkFile";
    case audioMasterGetInputSpeakerArrangement:
        return "audioMasterGetInputSpeakerArrangement";
#endif
    default:
        return "unknown";
    }
}

// -----------------------------------------------------------------------

#endif // CARLA_VST_UTILS_HPP_INCLUDED
