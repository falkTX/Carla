/*
  ZynAddSubFX - a software synthesizer

  PresetsStore.cpp - Presets and Clipboard store
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PRESETSTORE_H
#define PRESETSTORE_H

#include <string>
#include <vector>

class XMLwrapper;
class PresetsStore
{
        const class Config& config;
    public:
        PresetsStore(const class Config &config);
        ~PresetsStore();

        //Clipboard stuff
        void copyclipboard(XMLwrapper &xml, char *type);
        bool pasteclipboard(XMLwrapper &xml);
        bool checkclipboardtype(const char *type);

        //presets stuff
        void copypreset(XMLwrapper &xml, char *type, std::string name);
        bool pastepreset(XMLwrapper &xml, unsigned int npreset);
        void deletepreset(unsigned int npreset);
        void deletepreset(std::string file);

        struct presetstruct {
            bool operator<(const presetstruct &b) const;
            std::string file;
            std::string name;
            std::string type;
        };
        std::vector<presetstruct> presets;

        void scanforpresets();

        struct {
            std::string data;
            std::string type;
        } clipboard;

        void clearpresets();
};

//extern PresetsStore presetsstore;
#endif
