// ----------------------------------------------------------------------------
//
//  Copyright (C) 2006-2023 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "resampler-table.h"


#undef ENABLE_VEC4
#if (defined(__SSE2_MATH__) || defined(__ARM_NEON) || defined(__ARM_NEON__)) && !defined(CARLA_OS_WIN)
# define ENABLE_VEC4
#endif


static double sinc (double x)
{
    x = fabs (x);
    if (x < 1e-6) return 1.0;
    x *= M_PI;
    return sin (x) / x;
}


static double wind (double x)
{
    x = fabs (x);
    if (x >= 1.0) return 0.0f;
    x *= M_PI;
    return 0.384 + 0.500 * cos (x) + 0.116 * cos (2 * x);
}


Resampler_table  *Resampler_table::_list = 0;
Resampler_mutex   Resampler_table::_mutex;


Resampler_table::Resampler_table (double fr, unsigned int hl, unsigned int np) :
    _next (0),
    _refc (0),
    _fr (fr),
    _hl (hl),
    _np (np)
{
    unsigned int  i, j, n;
    double        t;
    float         *p;

    n = hl * (np + 1);
#ifdef ENABLE_VEC4
    posix_memalign ((void **) &_ctab, 16, n * sizeof (float));
#else
    _ctab = new float [n];
#endif
    p = _ctab;
    for (j = 0; j <= np; j++)
    {
        t = (double) j / (double) np;
        for (i = 0; i < hl; i++)
        {
            p [hl - i - 1] = (float)(fr * sinc (t * fr) * wind (t / hl));
            t += 1;
        }
        p += hl;
    }
}


Resampler_table::~Resampler_table (void)
{
#ifdef ENABLE_VEC4
    free (_ctab);
#else
    delete[] _ctab;
#endif
}


Resampler_table *Resampler_table::create (double fr, unsigned int hl, unsigned int np)
{
    Resampler_table *P;

    _mutex.lock ();
    P = _list;
    while (P)
    {
        if ((fr >= P->_fr * 0.999) && (fr <= P->_fr * 1.001) && (hl == P->_hl) && (np == P->_np))
        {
            P->_refc++;
            _mutex.unlock ();
            return P;
        }
        P = P->_next;
    }
    P = new Resampler_table (fr, hl, np);
    P->_refc = 1;
    P->_next = _list;
    _list = P;
    _mutex.unlock ();
    return P;
}


void Resampler_table::destroy (Resampler_table *T)
{
    Resampler_table *P, *Q;

    _mutex.lock ();
    if (T)
    {
        T->_refc--;
        if (T->_refc == 0)
        {
            P = _list;
            Q = 0;
            while (P)
            {
                if (P == T)
                {
                    if (Q) Q->_next = T->_next;
                    else      _list = T->_next;
                    break;
                }
                Q = P;
                P = P->_next;
            }
            delete T;
        }
    }
    _mutex.unlock ();
}
