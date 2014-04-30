//wdf.cpp

#include <stdio.h>
#include <inttypes.h>
#include <cmath>
#include "wdf.h"
using std::abs;

WDF::WDF() {}

void WDF::setWD(T val) {
	WD = val;
	state = val;
	DUMP(printf("DOWN\tWDF\t%c\tWD=%f\tWU=%f\tV=%f\n",type,WD,WU,(WD+WU)/2.0));	
}

void OnePort::setWD(T val) {
	WD = val;
	state = val;
	DUMP(printf("DOWN\tOneport\t%c\tWD=%f\tWU=%f\tV=%f\n",type,WD,WU,(WD+WU)/2.0));	
}

T WDF::Voltage() {
	T Volts = (WU + WD) / 2.0;
	return Volts;
}

T WDF::Current() {
	T Amps = (WU - WD) / (2.0*PortRes);
	return Amps;
}

template <class Port1, class Port2>ser::ser(Port1 *l, Port2 *r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
	type = 'S';
}

ser::ser(R* l, par* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
	type = 'S';
}

ser::ser(C* l, R* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
	type = 'S';
}

ser::ser(C* l, V* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
	type = 'S';
}

template <class Port>inv::inv(Port *l) : Adaptor(PASSTHROUGH) {
	left = l;
	PortRes = l->PortRes;
	type = 'I';
}

inv::inv(ser *l) : Adaptor(PASSTHROUGH) {
	left = l;
	PortRes = l->PortRes;
	type = 'I';
}

T ser::waveUp() {
	//Adaptor::WU = -left->waveUp() - right->waveUp();
	WDF::WU = -left->waveUp() - right->waveUp();
	DUMP(printf("UP\tser\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

template <class Port1, class Port2>par::par(Port1 *l, Port2 *r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
	type = 'P';
}

par::par(inv* l, R* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
	type = 'P';
}

par::par(inv* l, V* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
	type = 'P';
}

par::par(C* l, R* r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
	type = 'P';
}

T par::waveUp() {
	T G23 = 1.0 / left->PortRes + 1.0 / right->PortRes;
	WDF::WU = (1.0 / left->PortRes)/G23*left->waveUp() + (1.0 / right->PortRes)/G23*right->waveUp();
	DUMP(printf("UP\tpar\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

Adaptor::Adaptor(int flag) {
	WU = 0.0;
	WD = 0.0;
	switch (flag) {
		case ONEPORT:
			left = NULL;
			right = NULL;
			break;
		case PASSTHROUGH:
			right = NULL;
			break;
		default:
		case THREEPORT:
			break;
	}
}

void ser::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	DUMP(printf("SER WP=%f\n",waveparent));
	left->setWD(left->WU-(2.0*left->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
	right->setWD(right->WU-(2.0*right->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
}

void par::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	DUMP(printf("PAR WP=%f\n",waveparent));
	T p = 2.0*(waveparent/PortRes + left->WU/left->PortRes + right->WU/right->PortRes)/(1.0/PortRes + 1.0/left->PortRes + 1.0/right->PortRes);

	left->setWD((p - left->WU));
	right->setWD((p - right->WU));
}

T inv::waveUp() {
	///////////WD = -left->WD;
	WU = -left->waveUp(); 	//-
	DUMP(printf("UP\tinv\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

void inv::setWD(T waveparent) {
	WDF::setWD(waveparent);
	DUMP(printf("INV WP=%f\n",waveparent));
	//left->WD = -waveparent;		//-
	///////////left->WU = -WU;
	left->setWD(-waveparent);	//-
	
}

R::R(T res) : Adaptor(ONEPORT) {
	PortRes = res;
	type = 'R';
}

T R::waveUp() {
	WU = 0.0;
	DUMP(printf("UP\tR\tWU=%f\tWD=%f\tV=%f\n",WU, WD,(WD+WU)/2.0));
	return WU;
}

C::C(T c, T fs) : Adaptor(ONEPORT) {
	PortRes = 1.0/(2.0*c*fs);
	state = 0.0;
	type = 'C';
}

T C::waveUp() {
	WU = state;
	DUMP(printf("UP\tC\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

V::V(T ee, T r) : Adaptor(ONEPORT) {
	e = ee;
	PortRes = r;
	WD = 0.0;  //always?
	type = 'V';
}

T V::waveUp() {
	T watts = 100.0;
	WU = 2.0*e - WD;
	if (Voltage()*Current() > watts) WU *= 0.999;//0.9955;
	DUMP(printf("UP\tV\tWU=%f\tWD=%f\tV=%f\n",WU, WD,(WD+WU)/2.0));
	return WU;
}

inline T _exp(const T x)
{
    if(x < 10.0 && x > 10.0)
        return 1.0 + x + x*x/2.0 + x*x*x/6.0 + x*x*x*x/24.0 + x*x*x*x*x/120.0
            + x*x*x*x*x*x/720.0 + x*x*x*x*x*x*x/5040.0;
    else
        return exp(x);
}

inline T _log(const T x)
{
    if(x > 10)
        return log(x);
    const T a=(x-1)/(x+1);
    return 2.0*(a+a*a*a/3.0+a*a*a*a*a/5.0+a*a*a*a*a*a*a/7.0+a*a*a*a*a*a*a*a*a/9.0); 
}

inline T _pow(const T a, const T b)
{
    return pow(a,b);
}

T Triode::ffg(T VG) {
        return (G.WD-G.PortRes*(gg*_pow(_log(1.0+_exp(cg*VG))/cg,e)+ig0)-VG);
}

T Triode::fgdash(T VG) {
        T a1 = exp(cg*VG);
        T b1 = -e*pow(log(a1+1.0)/cg,e-1.0);
        T c1 = a1/(a1+1.0)*gg*G.PortRes;
        return (b1*c1);
}

T Triode::ffp(T VP) { 
    static bool prepared = false;
    static double scale;
    static double coeff[4];
    if(!prepared) {
        //go go series expansion
        const double L2 = log(2.0);

        const double scale = pow(L2,gamma-2)/(8.0*pow(c,gamma));
        coeff[0] = 8.0*L2*L2*scale;
        coeff[1] = gamma*c*L2*4*scale;
        coeff[2] = (c*c*gamma*gamma+L2*c*c*gamma-c*c*gamma)*scale;
        coeff[3] = 0.0;
        prepared = true;
    }

    double A = VP/mu+vg;
    return (P.WD+P.PortRes*((g*(coeff[0]+coeff[1]*A+coeff[2]*A*A))+(G.WD-vg)/G.PortRes)-VP);

    printf("%f\n", VP/mu+vg);
	return (P.WD+P.PortRes*((g*_pow(_log(1.0+_exp(c*(VP/mu+vg)))/c,gamma))+(G.WD-vg)/G.PortRes)-VP);
}	//	    ^

T Triode::fpdash(T VP) {
        T a1 = exp(c*(vg+VP/mu));
        T b1 = a1/(mu*(a1+1.0));
        T c1 = g*gamma*P.PortRes*pow(log(a1+1.0)/c,gamma-1.0);
        return (c1*b1);
}

T Triode::ffk() {
        return (K.WD - K.PortRes*(g*pow(log(1.0+exp(c*(vp/mu+vg)))/c,gamma)));
}
/*
T Triode::secantfg(T *i1, T *i2) {
        T vgn = 0.0;
        T init = *i1;
        for (int i = 0; i<ITER; ++i) {
                vgn = *i1 - fg(*i1)*(*i1-*i2)/(fg(*i1)-fg(*i2));
                *i2 = *i1;
                *i1 = vgn;
                if ((fabs(fg(vgn))) < EPSILON) break;
        }
        if ((fabs(fg(vgn)) >= EPSILON)) {
                DUMP(fprintf(stderr,"Vg did not converge\n"));
		return init;
	}
        return vgn; 
}               
        
T Triode::newtonfg(T *i1) {
        T init = *i1;
        if (fabs(fg(*i1)) < EPSILON || fgdash(*i1)==0.0) return *i1;
        T vgn = 0.0;
        for (int i = 0; i<ITER; ++i) {
                vgn = *i1 - fg(*i1)/fgdash(*i1);
                *i1 = vgn;
                if (fabs(fg(vgn)) < EPSILON) break;
        } 
        if ((fabs(fg(vgn)) >= EPSILON)) {
//                vgn = init;
                DUMP(fprintf(stderr,"Vg did not converge\n"));
        }       
        return vgn;
}

T Triode::newtonfp(T *i1) {
        T init = *i1;
        if (fabs(fp(*i1)) < EPSILON || fpdash(*i1)==0.0) return *i1;
        T vpn = 0.0;
        for (int i = 0; i<ITER; ++i) {
                vpn = *i1 - fp(*i1)/fpdash(*i1);
                *i1 = vpn;
                if (fabs(fp(vpn)) < EPSILON) break;
        }
        if ((fabs(fp(vpn)) >= EPSILON)) {
//                vpn = init;
                DUMP(fprintf(stderr,"Vp did not converge\n"));
        }
        return vpn;
}

T Triode::secantfp(T *i1, T *i2) {
        T vpn = 0.0;
        for (int i = 0; i<ITER; ++i) {
                vpn = *i1 - fp(*i1)*(*i1-*i2)/(fp(*i1)-fp(*i2));
                *i2 = *i1;
                *i1 = vpn;
                if ((fabs(fp(vpn))) < EPSILON) break;
        }

        if ((fabs(fp(vpn)) >= EPSILON))
                DUMP(fprintf(stderr,"Vp did not converge\n"));
        return vpn;
}
*/

//****************************************************************************80
//	Purpose:
//
//		Brent's method root finder.
//
//	Licensing:
//
//		This code below is distributed under the GNU LGPL license.
//
//	Author:
//
//		Original FORTRAN77 version by Richard Brent.
//		C++ version by John Burkardt.
//		Adapted for zamvalve by Damien Zammit.

T Triode::r8_abs ( T x )
{
	T value;

	if ( 0.0 <= x )
	{
		value = x;
	}
	else
	{
		value = - x;
	}
	return value;
}

Triode::Triode()
{
	T r = 1.0;

	while ( 1.0 < ( T ) ( 1.0 + r )	)
	{
		r = r / 2.0;
	}

	r *= 2.0;
	r8_epsilon = r;
}

T Triode::r8_max ( T x, T y )
{
	T value;

	if ( y < x )
	{
		value = x;
	}
	else
	{
		value = y;
	}
	return value;
}

T Triode::r8_sign ( T x )
{
	T value;

	if ( x < 0.0 )
	{
		value = -1.0;
	}
	else
	{
		value = 1.0;
	}
	return value;
}


T Triode::zeroffp ( T a, T b, T t )
{
	T c;
	T d;
	T e;
	T fa;
	T fb;
	T fc;
	T m;
	T macheps;
	T p;
	T q;
	T r;
	T s;
	T sa;
	T sb;
	T tol;
//
//	Make local copies of A and B.
//
	sa = a;
	sb = b;
	fa = ffp ( sa );
	fb = ffp ( sb );

	c = sa;
	fc = fa;
	e = sb - sa;
	d = e;

	macheps = r8_epsilon;

	for ( ; ; )
	{
		if ( abs ( fc ) < abs ( fb ) )
		{
			sa = sb;
			sb = c;
			c = sa;
			fa = fb;
			fb = fc;
			fc = fa;
		}

		tol = 2.0 * macheps * abs ( sb ) + t;
		m = 0.5 * ( c - sb );

		if ( abs ( m ) <= tol || fb == 0.0 )
		{
			break;
		}

		if ( abs ( e ) < tol || abs ( fa ) <= abs ( fb ) )
		{
			e = m;
			d = e;
		}
		else
		{
			s = fb / fa;

			if ( sa == c )
			{
				p = 2.0 * m * s;
				q = 1.0 - s;
			}
			else
			{
				q = fa / fc;
				r = fb / fc;
				p = s * ( 2.0 * m * q * ( q - r ) - ( sb - sa ) * ( r - 1.0 ) );
				q = ( q - 1.0 ) * ( r - 1.0 ) * ( s - 1.0 );
			}

			if ( 0.0 < p )
			{
				q = - q;
			}
			else
			{
				p = - p;
			}

			s = e;
			e = d;

			if ( 2.0 * p < 3.0 * m * q - abs ( tol * q ) &&
				p < abs ( 0.5 * s * q ) )
			{
				d = p / q;
			}
			else
			{
				e = m;
				d = e;
			}
		}
		sa = sb;
		fa = fb;

		if ( tol < abs ( d ) )
		{
			sb = sb + d;
		}
		else if ( 0.0 < m )
		{
			sb = sb + tol;
		}
		else
		{
			sb = sb - tol;
		}

		fb = ffp ( sb );

		if ( ( 0.0 < fb && 0.0 < fc ) || ( fb <= 0.0 && fc <= 0.0 ) )
		{
			c = sa;
			fc = fa;
			e = sb - sa;
			d = e;
		}
	}
	return sb;
}

T Triode::zeroffg ( T a, T b, T t )
{
	T c;
	T d;
	T e;
	T fa;
	T fb;
	T fc;
	T m;
	T macheps;
	T p;
	T q;
	T r;
	T s;
	T sa;
	T sb;
	T tol;
//
//	Make local copies of A and B.
//
	sa = a;
	sb = b;
	fa = ffg ( sa );
	fb = ffg ( sb );

	c = sa;
	fc = fa;
	e = sb - sa;
	d = e;

	macheps = r8_epsilon;

	for ( ; ; )
	{
		if ( abs ( fc ) < abs ( fb ) )
		{
			sa = sb;
			sb = c;
			c = sa;
			fa = fb;
			fb = fc;
			fc = fa;
		}

		tol = 2.0 * macheps * abs ( sb ) + t;
		m = 0.5 * ( c - sb );

		if ( abs ( m ) <= tol || fb == 0.0 )
		{
			break;
		}

		if ( abs ( e ) < tol || abs ( fa ) <= abs ( fb ) )
		{
			e = m;
			d = e;
		}
		else
		{
			s = fb / fa;

			if ( sa == c )
			{
				p = 2.0 * m * s;
				q = 1.0 - s;
			}
			else
			{
				q = fa / fc;
				r = fb / fc;
				p = s * ( 2.0 * m * q * ( q - r ) - ( sb - sa ) * ( r - 1.0 ) );
				q = ( q - 1.0 ) * ( r - 1.0 ) * ( s - 1.0 );
			}

			if ( 0.0 < p )
			{
				q = - q;
			}
			else
			{
				p = - p;
			}

			s = e;
			e = d;

			if ( 2.0 * p < 3.0 * m * q - abs ( tol * q ) &&
				p < abs ( 0.5 * s * q ) )
			{
				d = p / q;
			}
			else
			{
				e = m;
				d = e;
			}
		}
		sa = sb;
		fa = fb;

		if ( tol < abs ( d ) )
		{
			sb = sb + d;
		}
		else if ( 0.0 < m )
		{
			sb = sb + tol;
		}
		else
		{
			sb = sb - tol;
		}

		fb = ffg ( sb );

		if ( ( 0.0 < fb && 0.0 < fc ) || ( fb <= 0.0 && fc <= 0.0 ) )
		{
			c = sa;
			fc = fa;
			e = sb - sa;
			d = e;
		}
	}
	return sb;
}

