/*
  ZynAddSubFX - a software synthesizer

  Stereo.cpp - Object for storing a pair of objects
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

template<class T>
Stereo<T>::Stereo(const T &left, const T &right)
    :l(left), r(right)
{}

template<class T>
Stereo<T>::Stereo(const T &val)
    :l(val), r(val)
{}

template<class T>
Stereo<T> &Stereo<T>::operator=(const Stereo<T> &nstr)
{
    l = nstr.l;
    r = nstr.r;
    return *this;
}
