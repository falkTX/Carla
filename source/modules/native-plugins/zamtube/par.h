//par.h
#ifndef __PAR_H__
#define __PAR_H__

class inv;
class ser;

#include "wdf.h"
#include "inv.h"
#include "ser.h"

class par : public Adaptor {
public:
	template <class Port1, class Port2>par(Port1 *left, Port2 *right);
	par(inv *l, R* r);
	par(inv *l, V* r);
	par(C *l, R* r);
	T waveUp();
	void setWD(T waveparent);
};

#else
#endif
