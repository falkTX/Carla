/*
   ZynAddSubFX - a software synthesizer

   Compressor.h - simple audio compressor macros
   Copyright (C) 2016 Hans Petter Selasky

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef _COMPRESSOR_H_
#define	_COMPRESSOR_H_

#define	floatIsValid(x) ({			\
      float __r = (x) * 0.0;			\
      __r == 0.0 || __r == -0.0;		\
})

#define	stereoCompressor(div,pv,l,r) do {	\
  /*						\
   * Don't max the output range to avoid	\
   * overflowing sample rate conversion and	\
   * equalizer filters in the DSP's output	\
   * path. Keep one 10th, 1dB, reserved.	\
   */						\
  const float __limit = 1.0 - (1.0 / 10.0);	\
  float __peak;					\
						\
  /* sanity checks */				\
  __peak = (pv);				\
  if (!floatIsValid(__peak))			\
	__peak = 0.0;				\
  if (!floatIsValid(l))				\
	(l) = 0.0;				\
  if (!floatIsValid(r))				\
	(r) = 0.0;				\
  /* compute maximum */				\
  if ((l) < -__peak)				\
    __peak = -(l);				\
  else if ((l) > __peak)			\
    __peak = (l);				\
  if ((r) < -__peak)				\
    __peak = -(r);				\
  else if ((r) > __peak)			\
    __peak = (r);				\
  /* compressor */				\
  if (__peak > __limit) {			\
    (l) /= __peak;				\
    (r) /= __peak;				\
    (l) *= __limit;				\
    (r) *= __limit;				\
    __peak -= __peak / (div);			\
  }						\
  (pv) = __peak;				\
} while (0)

#endif			/* _COMPRESSOR_H_ */
