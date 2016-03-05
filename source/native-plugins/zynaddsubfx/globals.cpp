/*
  ZynAddSubFX - a software synthesizer

  globals.h - it contains program settings and the program capabilities
              like number of parts, of effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Misc/Util.h"
#include "globals.h"

void SYNTH_T::alias(bool randomize)
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
        if(randomize)
            denormalkillbuf[i] = (RND - 0.5f) * 1e-16;
        else
            denormalkillbuf[i] = 0;
}
