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

#include "CarlaNative.h"
#include "vestige/vestige.h"
#include "ui_launcher_res.hpp"

struct CarlaUILauncher;
class NativePlugin;

struct VstObject {
    void* audioMaster;
    NativePlugin* plugin;
};

CarlaUILauncher* createUILauncher(const intptr_t winId,
                                  const NativePluginDescriptor* const d,
                                  const NativePluginHandle h);
void idleUILauncher(CarlaUILauncher* const ui);
void destoryUILauncher(CarlaUILauncher* const ui);

const AEffect* VSTPluginMainInit(AEffect* const effect);
intptr_t VSTAudioMaster(AEffect*, int32_t, int32_t, intptr_t, void*, float);

intptr_t vst_dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
float vst_getParameterCallback(AEffect* effect, int32_t index);
void vst_setParameterCallback(AEffect* effect, int32_t index, float value);
void vst_processCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);
void vst_processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);
