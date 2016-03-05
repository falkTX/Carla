/*
  ZynAddSubFX - a software synthesizer

  CallbackRepeater.h - Timed Callback Container
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <functional>
#include <ctime>

struct CallbackRepeater
{
    typedef std::function<void(void)> cb_t ;

    //Call interval in seconds and callback
    CallbackRepeater(int interval, cb_t cb_);

    //Time Check
    void tick(void);

    std::time_t last;
    std::time_t dt;
    cb_t        cb;
};
