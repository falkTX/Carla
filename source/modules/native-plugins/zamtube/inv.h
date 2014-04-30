//inv.h
#ifndef __INV_H__
#define __INV_H__

class ser;
class par;

#include "wdf.h"
#include "ser.h"
#include "par.h"

class inv : public Adaptor { 
public:
	template <class Port>inv(Port *left);
	inv(ser *l);
	T waveUp();
	void setWD(T waveparent);
};

#else
#endif
