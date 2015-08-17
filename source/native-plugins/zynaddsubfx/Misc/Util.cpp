/*
  ZynAddSubFX - a software synthesizer

  Util.cpp - Miscellaneous functions
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

#include "globals.h"
#include "Util.h"
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_SCHEDULER
#include <sched.h>
#endif

#ifndef errx
#include <err.h>
#endif

#include <rtosc/rtosc.h>

prng_t prng_state = 0x1234;

/*
 * Transform the velocity according the scaling parameter (velocity sensing)
 */
float VelF(float velocity, unsigned char scaling)
{
    float x;
    x = powf(VELOCITY_MAX_SCALE, (64.0f - scaling) / 64.0f);
    if((scaling == 127) || (velocity > 0.99f))
        return 1.0f;
    else
        return powf(velocity, x);
}

/*
 * Get the detune in cents
 */
float getdetune(unsigned char type,
                unsigned short int coarsedetune,
                unsigned short int finedetune)
{
    float det = 0.0f, octdet = 0.0f, cdet = 0.0f, findet = 0.0f;
    //Get Octave
    int octave = coarsedetune / 1024;
    if(octave >= 8)
        octave -= 16;
    octdet = octave * 1200.0f;

    //Coarse and fine detune
    int cdetune = coarsedetune % 1024;
    if(cdetune > 512)
        cdetune -= 1024;

    int fdetune = finedetune - 8192;

    switch(type) {
//	case 1: is used for the default (see below)
        case 2:
            cdet   = fabs(cdetune * 10.0f);
            findet = fabs(fdetune / 8192.0f) * 10.0f;
            break;
        case 3:
            cdet   = fabs(cdetune * 100.0f);
            findet = powf(10, fabs(fdetune / 8192.0f) * 3.0f) / 10.0f - 0.1f;
            break;
        case 4:
            cdet   = fabs(cdetune * 701.95500087f); //perfect fifth
            findet =
                (powf(2, fabs(fdetune / 8192.0f) * 12.0f) - 1.0f) / 4095 * 1200;
            break;
        //case ...: need to update N_DETUNE_TYPES, if you'll add more
        default:
            cdet   = fabs(cdetune * 50.0f);
            findet = fabs(fdetune / 8192.0f) * 35.0f; //almost like "Paul's Sound Designer 2"
            break;
    }
    if(finedetune < 8192)
        findet = -findet;
    if(cdetune < 0)
        cdet = -cdet;

    det = octdet + cdet + findet;
    return det;
}


bool fileexists(const char *filename)
{
    struct stat tmp;
    int result = stat(filename, &tmp);
    if(result >= 0)
        return true;

    return false;
}

void set_realtime()
{
#ifdef HAVE_SCHEDULER
    sched_param sc;
    sc.sched_priority = 60;
    //if you want get "sched_setscheduler undeclared" from compilation,
    //you can safely remove the folowing line:
    sched_setscheduler(0, SCHED_FIFO, &sc);
    //if (err==0) printf("Real-time");
#endif
}

void os_sleep(long length)
{
    usleep(length);
}

//!< maximum lenght a pid has on any POSIX system
//!< this is an estimation, but more than 12 looks insane
constexpr std::size_t max_pid_len = 12;

//!< safe pid lenght guess, posix conform
std::size_t os_guess_pid_length()
{
    const char* pid_max_file = "/proc/sys/kernel/pid_max";
    if(-1 == access(pid_max_file, R_OK)) {
        return max_pid_len;
    }
    else {
        std::ifstream is(pid_max_file);
        if(!is.good())
            return max_pid_len;
        else {
            std::string s;
            is >> s;
            for(const auto& c : s)
                if(c < '0' || c > '9')
                    return max_pid_len;
            return std::min(s.length(), max_pid_len);
        }
    }
}

//!< returns pid padded, posix conform
std::string os_pid_as_padded_string()
{
    char result_str[max_pid_len << 1];
    std::fill_n(result_str, max_pid_len, '0');
    std::size_t written = snprintf(result_str + max_pid_len, max_pid_len,
        "%d", (int)getpid());
    // the below pointer should never cause segfaults:
    return result_str + max_pid_len + written - os_guess_pid_length();
}

std::string legalizeFilename(std::string filename)
{
    for(int i = 0; i < (int) filename.size(); ++i) {
        char c = filename[i];
        if(!(isdigit(c) || isalpha(c) || (c == '-') || (c == ' ')))
            filename[i] = '_';
    }
    return filename;
}

void invSignal(float *sig, size_t len)
{
    for(size_t i = 0; i < len; ++i)
        sig[i] *= -1.0f;
}

float SYNTH_T::numRandom()
{
    return RND;
}

float interpolate(const float *data, size_t len, float pos)
{
    assert(len > (size_t)pos + 1);
    const int l_pos      = (int)pos,
              r_pos      = l_pos + 1;
    const float leftness = pos - l_pos;
    return data[l_pos] * leftness + data[r_pos] * (1.0f - leftness);
}

float cinterpolate(const float *data, size_t len, float pos)
{
    const int l_pos      = ((int)pos) % len,
              r_pos      = (l_pos + 1) % len;
    const float leftness = pos - l_pos;
    return data[l_pos] * leftness + data[r_pos] * (1.0f - leftness);
}

char *rtosc_splat(const char *path, std::set<std::string> v)
{
    char argT[v.size()+1];
    rtosc_arg_t arg[v.size()];
    unsigned i=0;
    for(auto vv : v) {
        argT[i]  = 's';
        arg[i].s = vv.c_str();
        i++;
    }
    argT[v.size()] = 0;

    size_t len = rtosc_amessage(0, 0, path, argT, arg);
    char *buf = new char[len];
    rtosc_amessage(buf, len, path, argT, arg);
    return buf;
}
