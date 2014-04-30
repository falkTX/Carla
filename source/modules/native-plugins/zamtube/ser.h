//ser.h
#ifndef __SER_H__
#define __SER_H__

class inv;
class par;

#include "inv.h"
#include "par.h"

class ser : public Adaptor {
public:
	template <class Port1, class Port2>ser(Port1 *left, Port2 *right);
	ser(R* l, par* r);
	ser(C* l, R* r);
	ser(C* l, V* r);
	T waveUp();
	void setWD(T waveparent);	
};

#else
#endif
