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
