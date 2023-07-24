/*
 * Carla Native Plugins
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_VST_HPP_INCLUDED
#define CARLA_VST_HPP_INCLUDED

#include "CarlaDefines.h"
#include "CarlaNative.h"
#include "vestige/vestige.h"

#include "ui_launcher_res.hpp"
struct CarlaUILauncher;

class NativePlugin;

struct VstObject {
    void* audioMaster;
    NativePlugin* plugin;
};

struct VstRect {
    int16_t top, left, bottom, right;
};

CarlaUILauncher* createUILauncher(uintptr_t winId, const NativePluginDescriptor* d, NativePluginHandle h);
void getUILauncherSize(CarlaUILauncher* ui, VstRect* rect);
void idleUILauncher(CarlaUILauncher* ui);
void destoryUILauncher(CarlaUILauncher* ui);

const AEffect* VSTPluginMainInit(AEffect* effect);
intptr_t VSTAudioMaster(AEffect*, int32_t, int32_t, intptr_t, void*, float);
bool isUsingUILauncher();

intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
float vst_getParameterCallback(AEffect* effect, int32_t index);
void vst_setParameterCallback(AEffect* effect, int32_t index, float value);
void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);
void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);

#endif // CARLA_VST_HPP_INCLUDED
