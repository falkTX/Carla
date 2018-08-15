/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#ifndef SFZDEBUG_H_INCLUDED
#define SFZDEBUG_H_INCLUDED

#include "SFZCommon.h"

#ifdef DEBUG

namespace sfzero
{
void dbgprintf(const char *msg, ...);
}

#endif

#endif // SFZDEBUG_H_INCLUDED
