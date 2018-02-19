/*
  ZynAddSubFX - a software synthesizer

  ConnectionDummy.cpp - Out-Of-Process GUI Stubs
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Connection.h"
#include <unistd.h>
namespace GUI {
ui_handle_t createUi(Fl_Osc_Interface*, void *)
{
    return 0;
}
void destroyUi(ui_handle_t)
{
}
void raiseUi(ui_handle_t, const char *)
{
}
void raiseUi(ui_handle_t, const char *, const char *, ...)
{
}
void tickUi(ui_handle_t)
{
    usleep(1000);
}
Fl_Osc_Interface *genOscInterface(zyncarla::MiddleWare*)
{
    return NULL;
}
};
