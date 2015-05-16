/*
  ZynAddSubFX - a software synthesizer

  PresetsArray.cpp - PresetsArray and Clipboard management
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

#include "PresetsStore.h"
#include "PresetsArray.h"
#include <string.h>


PresetsArray::PresetsArray()
{
    type[0]  = 0;
}

PresetsArray::~PresetsArray()
{}

void PresetsArray::setpresettype(const char *type)
{
    strcpy(this->type, type);
}

void PresetsArray::copy(PresetsStore &ps, const char *name)
{
    copy(ps, -1, name);
}

void PresetsArray::copy(PresetsStore &ps, int nelement, const char *name)
{
    XMLwrapper xml;

    //used only for the clipboard
    if(name == NULL)
        xml.minimal = false;

    char type[MAX_PRESETTYPE_SIZE];
    strcpy(type, this->type);
    if(nelement != -1)
        strcat(type, "n");
    if(name == NULL)
        if(strstr(type, "Plfo") != NULL)
            strcpy(type, "Plfo");

    xml.beginbranch(type);
    if(nelement == -1)
        add2XML(&xml);
    else
        add2XMLsection(&xml, nelement);
    xml.endbranch();

    if(name == NULL)
        ps.copyclipboard(xml, type);
    else
        ps.copypreset(xml, type, name);
}

#if 0
void PresetsArray::paste(PresetsStore &ps, int npreset)
{
    char type[MAX_PRESETTYPE_SIZE];
    strcpy(type, this->type);
    if(nelement != -1)
        strcat(type, "n");
    if(npreset == 0)
        if(strstr(type, "Plfo") != NULL)
            strcpy(type, "Plfo");

    XMLwrapper xml;
    if(npreset == 0) {
        if(!checkclipboardtype(ps)) {
            nelement = -1;
            return;
        }
        if(!ps.pasteclipboard(xml)) {
            nelement = -1;
            return;
        }
    } else if(!ps.pastepreset(xml, npreset)) {
        nelement = -1;
        return;
    }

    if(xml.enterbranch(type) == 0) {
        nelement = -1;
        return;
    }
    if(nelement == -1) {
        defaults();
        getfromXML(&xml);
    } else {
        defaults(nelement);
        getfromXMLsection(&xml, nelement);
    }
    xml.exitbranch();

    nelement = -1;
}


bool PresetsArray::checkclipboardtype(PresetsStore &ps)
{
    char type[MAX_PRESETTYPE_SIZE];
    strcpy(type, this->type);
    if(nelement != -1)
        strcat(type, "n");

    return ps.checkclipboardtype(type);
}
#endif

//void PresetsArray::rescanforpresets()
//{
//    char type[MAX_PRESETTYPE_SIZE];
//    strcpy(type, this->type);
//    if(nelement != -1)
//        strcat(type, "n");
//
//    presetsstore.rescanforpresets(type);
//}
