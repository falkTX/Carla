/*
  ZynAddSubFX - a software synthesizer

  PresetExtractor.h - RT Safe Copy/Paste
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <string>
#include <rtosc/ports.h>

extern const rtosc::Ports real_preset_ports;
extern const rtosc::Ports preset_ports;
struct Clipboard {
    std::string data;
    std::string type;
};

Clipboard clipboardCopy(class MiddleWare &mw, std::string url);

void presetCopy(MiddleWare &mw, std::string url, std::string name);
void presetPaste(MiddleWare &mw, std::string url, std::string name);
void presetCopyArray(MiddleWare &mw, std::string url,  int field, std::string name);
void presetPasteArray(MiddleWare &mw, std::string url, int field, std::string name);
void presetPaste(std::string url, int);
void presetDelete(int);
void presetRescan();
std::string presetClipboardType();
bool presetCheckClipboardType();
