/*
  ZynAddSubFX - a software synthesizer

  globals.h - it contains program settings and the program capabilities
              like number of parts, of effects
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

#include "Misc/Util.h"
#include "globals.h"

void SYNTH_T::alias()
{
    halfsamplerate_f = (samplerate_f = samplerate) / 2.0f;
    buffersize_f     = buffersize;
    bufferbytes      = buffersize * sizeof(float);
    oscilsize_f      = oscilsize;

    //produce denormal buf
    // note: once there will be more buffers, use a cleanup function
    // for deleting the buffers and also call it in the dtor
    denormalkillbuf.resize(buffersize);
    for(int i = 0; i < buffersize; ++i)
        denormalkillbuf[i] = (RND - 0.5f) * 1e-16;
}
