/*
 * Carla Native Plugins
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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

#include "carla-vst.hpp"
#include "ui_launcher.cpp"
#include "ui_launcher_res.cpp"

#include "CarlaDefines.h"

#ifdef __WINE__
__cdecl static intptr_t cvst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    return vst_dispatcherCallback(effect, opcode, index, value, ptr, opt);
}

__cdecl static float cvst_getParameterCallback(AEffect* effect, int32_t index)
{
    return vst_getParameterCallback(effect, index);
}

__cdecl static void cvst_setParameterCallback(AEffect* effect, int32_t index, float value)
{
    return vst_setParameterCallback(effect, index, value);
}

__cdecl static void cvst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    return vst_processCallback(effect, inputs, outputs, sampleFrames);
}

__cdecl static void cvst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    return vst_processReplacingCallback(effect, inputs, outputs, sampleFrames);
}
#endif

CARLA_EXPORT __cdecl
#if defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
const AEffect* VSTPluginMain(audioMasterCallback audioMaster);
#else
const AEffect* VSTPluginMain(audioMasterCallback audioMaster) asm ("main");
#endif

CARLA_EXPORT __cdecl
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    // old version
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0f) == 0)
        return nullptr;

    AEffect* const effect(new AEffect);
    std::memset(effect, 0, sizeof(AEffect));

    // vst fields
    effect->magic   = kEffectMagic;
    effect->version = CARLA_VERSION_HEX;

    // pointers
    VstObject* const obj(new VstObject());
    obj->audioMaster = (void*)audioMaster;
    obj->plugin      = nullptr;
    effect->object   = obj;

    // static calls
#ifdef __WINE__
    effect->dispatcher       = cvst_dispatcherCallback;
    effect->getParameter     = cvst_getParameterCallback;
    effect->setParameter     = cvst_setParameterCallback;
    effect->process          = cvst_processCallback;
    effect->processReplacing = cvst_processReplacingCallback;
#else
    effect->dispatcher       = vst_dispatcherCallback;
    effect->process          = vst_processCallback;
    effect->getParameter     = vst_getParameterCallback;
    effect->setParameter     = vst_setParameterCallback;
    effect->processReplacing = vst_processReplacingCallback;
#endif

    return VSTPluginMainInit(effect);
}

intptr_t VSTAudioMaster(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    const audioMasterCallback audioMaster = (audioMasterCallback)((VstObject*)effect->object)->audioMaster;
    return audioMaster(effect, opcode, index, value, ptr, opt);
}
