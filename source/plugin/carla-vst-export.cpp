/*
 * Carla Native Plugins
 * Copyright (C) 2013-2022 Filipe Coelho <falktx@falktx.com>
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

#include <cstring>
#include <vector>

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

static struct CarlaVSTCleanup {
    std::vector<AEffect*> effects;
    std::vector<VstObject*> objects;

    CarlaVSTCleanup()
        : effects(),
          objects() {}

    ~CarlaVSTCleanup()
    {
        for (std::vector<VstObject*>::iterator it=objects.begin(), end=objects.end(); it != end; ++it)
            delete (*it);

        for (std::vector<AEffect*>::iterator it=effects.begin(), end=effects.end(); it != end; ++it)
            delete (*it);
    }
} gCarlaVSTCleanup;

CARLA_PLUGIN_EXPORT __cdecl
const AEffect* VSTPluginMain(audioMasterCallback audioMaster);

CARLA_PLUGIN_EXPORT __cdecl
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

    gCarlaVSTCleanup.effects.push_back(effect);
    gCarlaVSTCleanup.objects.push_back(obj);

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

#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
CARLA_PLUGIN_EXPORT __cdecl
const AEffect* VSTPluginMain_asm(audioMasterCallback audioMaster) asm ("main");

CARLA_PLUGIN_EXPORT __cdecl
const AEffect* VSTPluginMain_asm(audioMasterCallback audioMaster)
{
    return VSTPluginMain(audioMaster);
}
#endif

intptr_t VSTAudioMaster(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    const audioMasterCallback audioMaster = (audioMasterCallback)((VstObject*)effect->object)->audioMaster;
    return audioMaster(effect, opcode, index, value, ptr, opt);
}

bool isUsingUILauncher()
{
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
    return true;
#else
    return false;
#endif
}
