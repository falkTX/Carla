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

constexpr int version_type::v_strcmp(const version_type& v2, int i) const
{
    return (i == sizeof(version))
        ? 0
        : ((version[i] == v2.version[i])
            ? v_strcmp(v2, i+1)
            : (version[i] - v2.version[i]));
}

constexpr bool version_type::operator<(const version_type& other) const
{
    return v_strcmp(other, 0) < 0;
}

std::ostream& operator<< (std::ostream& os,
    const version_type& v)
{
    return os << v.major() << '.'
        << v.minor() << '.'
        << v.revision();
}

static_assert(!(version_type(3,1,1) < version_type(1,3,3)),
    "version operator failed");
static_assert(version_type(2,9,9) < version_type(3,4,3),
    "version operator failed");
static_assert(!(version_type(2,4,3) < version_type(2,4,3)),
    "version operator failed");

