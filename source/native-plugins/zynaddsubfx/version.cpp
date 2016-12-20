/*
  ZynAddSubFX - a software synthesizer

  version.cpp - implementation of version_type class
  Copyright (C) 2016 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <iostream>

#include "zyn-version.h"

std::ostream& operator<< (std::ostream& os,
    const version_type& v)
{
    return os << v.get_major() << '.'
        << v.get_minor() << '.'
        << v.get_revision();
}

static_assert(!(version_type(3,1,1) < version_type(1,3,3)),
    "version operator failed");
static_assert(version_type(2,9,9) < version_type(3,4,3),
    "version operator failed");
static_assert(!(version_type(2,4,3) < version_type(2,4,3)),
    "version operator failed");

