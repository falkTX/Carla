/*************************************************************************************
 * Original code copyright (C) 2012 Steve Folta
 * Converted to Juce module (C) 2016 Leo Olivers
 * Forked from https://github.com/stevefolta/SFZero
 * For license info please see the LICENSE file distributed with this source code
 *************************************************************************************/
#include "SFZDebug.h"
#include <stdarg.h>

namespace sfzero
{

#ifdef DEBUG

void dbgprintf(const char *msg, ...)
{
  va_list args;

  va_start(args, msg);

  char output[256];
  vsnprintf(output, 256, msg, args);

  va_end(args);
}

#endif // DEBUG

}
