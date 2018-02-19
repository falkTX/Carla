/*
  ZynAddSubFX - a software synthesizer

  Presets.h - Presets and Clipboard management
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PRESETS_H
#define PRESETS_H

#include "../globals.h"

namespace zyncarla {

class PresetsStore;

/**Presets and Clipboard management*/
class Presets
{
    friend class PresetsArray;
    public:
        Presets();
        virtual ~Presets();

        virtual void copy(PresetsStore &ps, const char *name); /**<if name==NULL, the clipboard is used*/
        //virtual void paste(PresetsStore &ps, int npreset); //npreset==0 for clipboard
        virtual bool checkclipboardtype(PresetsStore &ps);
        void deletepreset(PresetsStore &ps, int npreset);

        char type[MAX_PRESETTYPE_SIZE];
        //void setelement(int n);
    protected:
        void setpresettype(const char *type);
    private:
        virtual void add2XML(XMLwrapper& xml)    = 0;
        //virtual void getfromXML(XMLwrapper *xml) = 0;
        //virtual void defaults() = 0;
};

}

#endif
