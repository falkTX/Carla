/*
  ZynAddSubFX - a software synthesizer

  WaveShapeSmps.h - Sample distortions
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef WAVESHAPESMPS_H
#define WAVESHAPESMPS_H

//Waveshaping(called by Distorsion effect and waveshape from OscilGen)
void waveShapeSmps(int n,
                   float *smps,
                   unsigned char type,
                   unsigned char drive);

#endif
