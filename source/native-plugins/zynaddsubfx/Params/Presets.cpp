/*
  ZynAddSubFX - a software synthesizer

  Presets.cpp - Presets and Clipboard management
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "Presets.h"
#include "../Misc/XMLwrapper.h"
#include "PresetsStore.h"
#include <string.h>


Presets::Presets()
{
    type[0] = 0;
}

Presets::~Presets()
{}

void Presets::setpresettype(const char *type)
{
    strcpy(this->type, type);
}

void Presets::copy(PresetsStore &ps, const char *name)
{
    XMLwrapper xml;

    //used only for the clipboard
    if(name == NULL)
        xml.minimal = false;

    char type[MAX_PRESETTYPE_SIZE];
    strcpy(type, this->type);
    //strcat(type, "n");
    if(name == NULL)
        if(strstr(type, "Plfo") != NULL)
            strcpy(type, "Plfo");

    xml.beginbranch(type);
    add2XML(&xml);
    xml.endbranch();

    if(name == NULL)
        ps.copyclipboard(xml, type);
    else
        ps.copypreset(xml, type, name);
}

#if 0
void Presets::paste(PresetsStore &ps, int npreset)
{
    char type[MAX_PRESETTYPE_SIZE];
    strcpy(type, this->type);
    //strcat(type, "n");

    if(npreset == 0)
        if(strstr(type, "Plfo") != NULL)
            strcpy(type, "Plfo");

    XMLwrapper xml;
    if(npreset == 0) {
        if(!checkclipboardtype(ps))
            return;
        if(!ps.pasteclipboard(xml))
            return;
    } else if(!ps.pastepreset(xml, npreset))
        return;

    if(xml.enterbranch(type) == 0)
        return;

    defaults();
    getfromXML(&xml);

    xml.exitbranch();
}
#endif

bool Presets::checkclipboardtype(PresetsStore &ps)
{
    return ps.checkclipboardtype(type);
}


void Presets::deletepreset(PresetsStore &ps, int npreset)
{
    ps.deletepreset(npreset);
}
