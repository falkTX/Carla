/*
  ZynAddSubFX - a software synthesizer

  Util.h - Miscellaneous functions
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

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <stdint.h>
#include <algorithm>
#include <set>

using std::min;
using std::max;

//Velocity Sensing function
extern float VelF(float velocity, unsigned char scaling);

bool fileexists(const char *filename);

#define N_DETUNE_TYPES 4 //the number of detune types
extern float getdetune(unsigned char type,
                       unsigned short int coarsedetune,
                       unsigned short int finedetune);

/**Try to set current thread to realtime priority program priority
 * \todo see if the right pid is being sent
 * \todo see if this is having desired effect, if not then look at
 * pthread_attr_t*/
void set_realtime();

/**Os independent sleep in microsecond*/
void os_sleep(long length);

//! returns pid padded to maximum pid lenght, posix conform
std::string os_pid_as_padded_string();

std::string legalizeFilename(std::string filename);

void invSignal(float *sig, size_t len);

template<class T>
std::string stringFrom(T x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template<class T>
std::string to_s(T x)
{
    return stringFrom(x);
}

template<class T>
T stringTo(const char *x)
{
    std::string str = x != NULL ? x : "0"; //should work for the basic float/int
    std::stringstream ss(str);
    T ans;
    ss >> ans;
    return ans;
}



template<class T>
T limit(T val, T min, T max)
{
    return val < min ? min : (val > max ? max : val);
}

template<class T>
bool inRange(T val, T min, T max)
{
    return val >= min && val <= max;
}

template<class T>
T array_max(const T *data, size_t len)
{
    T max = 0;

    for(unsigned i = 0; i < len; ++i)
        if(max < data[i])
            max = data[i];
    return max;
}

//Random number generator

typedef uint32_t prng_t;
extern prng_t prng_state;

// Portable Pseudo-Random Number Generator
inline prng_t prng_r(prng_t &p)
{
    return p = p * 1103515245 + 12345;
}

inline prng_t prng(void)
{
    return prng_r(prng_state) & 0x7fffffff;
}

inline void sprng(prng_t p)
{
    prng_state = p;
}

/*
 * The random generator (0.0f..1.0f)
 */
#ifndef INT32_MAX
#define INT32_MAX      (2147483647)
#endif
#define RND (prng() / (INT32_MAX * 1.0f))

//Linear Interpolation
float interpolate(const float *data, size_t len, float pos);

//Linear circular interpolation
float cinterpolate(const float *data, size_t len, float pos);

template<class T>
static inline void nullify(T &t) {delete t; t = NULL; }
template<class T>
static inline void arrayNullify(T &t) {delete [] t; t = NULL; }

char *rtosc_splat(const char *path, std::set<std::string>);

/**
 * Port macros - these produce easy and regular port definitions for common
 * types
 */
#define rParamZyn(name, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) rMap(min, 0) rMap(max, 127) DOC(__VA_ARGS__), NULL, rParamICb(name)}

#define rSelf(type) \
{"self", rProp(internal) rMap(class, type) rDoc("port metadata"), 0, \
    [](const char *, rtosc::RtData &d){ \
        d.reply(d.loc, "b", sizeof(d.obj), &d.obj);}}\

#define rPaste \
{"preset-type", rProp(internal), 0, \
    [](const char *, rtosc::RtData &d){ \
        rObject *obj = (rObject*)d.obj; \
        d.reply(d.loc, "s", obj->type);}},\
{"paste:b", rProp(internal) rDoc("paste port"), 0, \
    [](const char *m, rtosc::RtData &d){ \
        printf("rPaste...\n"); \
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data; \
        rObject &o = *(rObject*)d.obj;\
        o.paste(paste);}}

#define rArrayPaste \
{"paste-array:bi", rProp(internal) rDoc("array paste port"), 0, \
    [](const char *m, rtosc::RtData &d){ \
        printf("rArrayPaste...\n"); \
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data; \
        int field = rtosc_argument(m,1).i; \
        rObject &o = *(rObject*)d.obj;\
        o.pasteArray(paste,field);}}

#endif
