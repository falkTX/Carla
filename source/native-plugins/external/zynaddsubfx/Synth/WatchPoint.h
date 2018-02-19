/*
  ZynAddSubFX - a software synthesizer

  WatchPoint.h - Synthesis State Watcher
  Copyright (C) 2015-2015 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once

namespace rtosc {class ThreadLink;}

namespace zyncarla {

struct WatchManager;

struct WatchPoint
{
    bool          active;
    int           samples_left;
    WatchManager *reference;
    char          identity[128];

    WatchPoint(WatchManager *ref, const char *prefix, const char *id);
    bool is_active(void);
};

#define MAX_WATCH 16
#define MAX_WATCH_PATH 128
#define MAX_SAMPLE 128
struct WatchManager
{
    typedef rtosc::ThreadLink thrlnk;
    thrlnk *write_back;
    bool    new_active;
    char    active_list[MAX_WATCH][MAX_WATCH_PATH];
    float   data_list[MAX_SAMPLE][MAX_WATCH];
    int     sample_list[MAX_WATCH];
    bool    deactivate[MAX_WATCH];

    //External API
    WatchManager(thrlnk *link=0);
    void add_watch(const char *);
    void del_watch(const char *);
    void tick(void);

    //Watch Point Query API
    bool active(const char *) const;
    int  samples(const char *) const;

    //Watch Point Response API
    void satisfy(const char *, float);
    void satisfy(const char *, float*, int);
};

struct FloatWatchPoint:public WatchPoint
{
    FloatWatchPoint(WatchManager *ref, const char *prefix, const char *id);
    inline void operator()(float f)
    {
        if(is_active() && reference) {
            reference->satisfy(identity, f);
            active = false;
        }
    }
};

//basically the same as the float watch point, only it consumes tuples
struct VecWatchPoint : public WatchPoint
{
    VecWatchPoint(WatchManager *ref, const char *prefix, const char *id);
    inline void operator()(float *f, int n)
    {
        if(is_active() && reference) {
            reference->satisfy(identity, f, n);
            active = false;
        }
    }
};

}
