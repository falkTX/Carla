/*
  ZynAddSubFX - a software synthesizer

  version.h - declaration of version_type class
              contains the current zynaddsubfx version
  Copyright (C) 2016 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef VERSION_H
#define VERSION_H

#include <iosfwd>

namespace zyncarla {

//! class containing a zynaddsubfx version
class version_type
{
    char version[3];

    // strcmp-like comparison against another version_type
    constexpr int v_strcmp(const version_type& v2, int i) const
    {
        return (i == sizeof(version))
            ? 0
            : ((version[i] == v2.version[i])
                ? v_strcmp(v2, i+1)
                : (version[i] - v2.version[i]));
    }
    
public:
    constexpr version_type(char maj, char min, char rev) :
        version{maj, min, rev}
    {
    }

    //! constructs the current zynaddsubfx version
    constexpr version_type() :
        version_type(3,
                     0,
                     2)
    {
    }

    void set_major(int maj) { version[0] = maj; }
    void set_minor(int min) { version[1] = min; }
    void set_revision(int rev) { version[2] = rev; }

    int get_major() const { return version[0]; }
    int get_minor() const { return version[1]; }
    int get_revision() const { return version[2]; }

    constexpr bool operator<(const version_type& other) const
    {
        return v_strcmp(other, 0) < 0;
    }

    //! prints version as <major>.<minor>.<revision>
    friend std::ostream& operator<< (std::ostream& os,
                                     const version_type& v);
};

//! the current zynaddsubfx version
constexpr version_type version;

}

#endif

