/*
  ZynAddSubFX - a software synthesizer

  PresetsArray.h - PresetsArray and Clipboard management
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PRESETSARRAY_H
#define PRESETSARRAY_H

#include "../Misc/XMLwrapper.h"

#include "Presets.h"

/**PresetsArray and Clipboard management*/
class PresetsArray:public Presets
{
    public:
        PresetsArray();
        virtual ~PresetsArray();

        void copy(PresetsStore &ps, const char *name); /**<if name==NULL, the clipboard is used*/
        void copy(PresetsStore &ps, int elm, const char *name); /**<if name==NULL, the clipboard is used*/
        //void paste(PresetsStore &ps, int npreset); //npreset==0 for clipboard
        //bool checkclipboardtype(PresetsStore &ps);
        //void setelement(int n);
    protected:
        void setpresettype(const char *type);
    private:
        virtual void add2XMLsection(XMLwrapper& xml, int n)    = 0;
        //virtual void getfromXMLsection(XMLwrapper *xml, int n) = 0;
        //virtual void defaults() = 0;
        //virtual void defaults(int n) = 0;
};

#endif
