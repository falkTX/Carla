/*
wdf.h
Copyright (C) 2013  Damien Zammit

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __WDF_H__
#define __WDF_H__

#include <math.h>

#define ONEPORT 0
#define PASSTHROUGH 1
#define THREEPORT 2

#define max(x,y) (( (x) > (y) ) ? x : y )
#define min(x,y) (( (x) < (y) ) ? x : y )
#define sign(x) ( (x) >= 0.0 ? 1.0 : -1.0 )
#define BIG 1e12
#define SMALL 1e-14
#define EPSILON 1e-9
#define ITER 50
#define SWAP_PP(x,y) {T tmp=y; y=x; x=tmp;}
#define SWAP_PN(x,y) {T tmp=y; y=-x; x=tmp;}
#define SWAP_NP(x,y) {T tmp=y; y=x; x=-tmp;}
#define SWAP_NN(x,y) {T tmp=y; y=-x; x=-tmp;}

#if 0
#include <stdio.h>
#define DUMP(x) x
#else
#define DUMP(x)
#endif

typedef double T;

class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage();
	T Current();
	T state;
	char type;

	virtual T waveUp() { return 0.0; }
	virtual void setWD(T waveparent);
};

class OnePort : public WDF {
public:
	void setWD(T waveparent);
};

class Triode {
public:
	WDF G, K, P;

        T vg, vk, vp;
        T g, mu, gamma, c, gg, e, cg, ig0;
        T g1, mu1, gamma1, c1, gg1, e1, cg1, ig01;
        T g2, mu2, gamma2, c2, gg2, e2, cg2, ig02;

	T ffg(T VG);
	T fgdash(T VG);
	T ffp(T VP);
	T fpdash(T VP);
	T ffk();
	T secantfg(T *i1, T *i2);
	T newtonfg(T *i1);
	T secantfp(T *i1, T *i2);
	T newtonfp(T *i1);
	
	Triode();

	//Brent's method
	T r8_abs ( T x );
	T r8_epsilon;
	T r8_max ( T x, T y );
	T r8_sign ( T x );
	T zeroffp ( T a, T b, T t );
	T zeroffg ( T a, T b, T t );

};

class Adaptor : public OnePort {
public:
	WDF *left;
	WDF *right;
	Adaptor(int flag);
};

class R : public Adaptor {
public:
	R(T res);
	T waveUp();
};

class C : public Adaptor {
public:
	C(T c, T fs);
	T waveUp();
};

class V : public Adaptor {
public:
	T e;
	V(T ee, T r);
	T waveUp();
};

class inv;
class ser;
class par;
#include "inv.h"
#include "ser.h"
#include "par.h"

#else
#endif
