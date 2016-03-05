/*
  ZynAddSubFX - a software synthesizer

  CallbackRepeater.cpp - Timed Callback Container
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "CallbackRepeater.h"
CallbackRepeater::CallbackRepeater(int interval, cb_t cb_)
    :last(time(0)), dt(interval), cb(cb_)
{}

void CallbackRepeater::tick(void)
{
    auto now = time(0);
    if(now-last > dt && dt >= 0) {
        cb();
        last = now;
    }
}
