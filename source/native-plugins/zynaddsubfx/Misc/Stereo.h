/*
  ZynAddSubFX - a software synthesizer

  Stereo.h - Object for storing a pair of objects
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef STEREO_H
#define STEREO_H

namespace zyncarla {

template<class T>
struct Stereo {
    public:
        Stereo(const T &left, const T &right);

        /**Initializes Stereo with left and right set to val
         * @param val the value for both channels*/
        Stereo(const T &val);
        ~Stereo() {}

        Stereo<T> &operator=(const Stereo<T> &smp);

        //data
        T l, r;
};

}

#include "Stereo.cpp"
#endif
