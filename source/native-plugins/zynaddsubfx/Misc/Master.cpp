/*
  ZynAddSubFX - a software synthesizer

  Master.cpp - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Master.h"

#include "Part.h"

#include "zyn-version.h"
#include "../Misc/Stereo.h"
#include "../Misc/Util.h"
#include "../Params/LFOParams.h"
#include "../Effects/EffectMgr.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Allocator.h"
#include "../Containers/ScratchString.h"
#include "../Nio/Nio.h"
#include "PresetExtractor.h"

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <rtosc/thread-link.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <unistd.h>

using namespace std;
using namespace rtosc;

namespace zyncarla {

#define rObject Master

static const Ports sysefxPort =
{
    {"part#" STRINGIFY(NUM_MIDI_PARTS) "::i", rProp(parameter)
        rDoc("gain on part to sysefx routing"), 0,
        [](const char *m, RtData&d)
        {
            //we know that if we are here the location must
            //be ...Psysefxvol#N/part#M
            //and the number "N" is one or two digits at most

            // go backto the '/'
            const char* m_findslash = m + strlen(m),
                      * loc_findslash = d.loc + strlen(d.loc);
            for(;*loc_findslash != '/'; --m_findslash, --loc_findslash)
                assert(*loc_findslash == *m_findslash);
            assert(m_findslash + 1 == m);

            const char *index_1 = loc_findslash-1;
            assert(isdigit(*index_1));
            if(isdigit(index_1[-1]))
                index_1--;
            int ind1 = atoi(index_1);

            //Now get the second index like normal
            while(!isdigit(*m)) m++;
            int ind2 = atoi(m);
            Master &mast = *(Master*)d.obj;

            if(rtosc_narguments(m)) {
                mast.setPsysefxvol(ind2, ind1, rtosc_argument(m,0).i);
                d.broadcast(d.loc, "i", mast.Psysefxvol[ind1][ind2]);
            } else
                d.reply(d.loc, "i", mast.Psysefxvol[ind1][ind2]);
        }}
};

static const Ports sysefsendto =
{
    {"to#" STRINGIFY(NUM_SYS_EFX) "::i",
        rProp(parameter) rDoc("sysefx to sysefx routing gain"), 0, [](const char *m, RtData&d)
        {
            //same workaround as before
            //go backto the '/'
            const char* m_findslash = m + strlen(m),
                      * loc_findslash = d.loc + strlen(d.loc);
            for(;*loc_findslash != '/'; --m_findslash, --loc_findslash)
                assert(*loc_findslash == *m_findslash);
            assert(m_findslash + 1 == m);

            const char *index_1 = loc_findslash-1;
            assert(isdigit(*index_1));
            if(isdigit(index_1[-1]))
                index_1--;
            int ind1 = atoi(index_1);

            //Now get the second index like normal
            while(!isdigit(*m)) m++;
            int ind2 = atoi(m);
            Master &master = *(Master*)d.obj;

            if(rtosc_narguments(m))
                master.setPsysefxsend(ind1, ind2, rtosc_argument(m,0).i);
            else
                d.reply(d.loc, "i", master.Psysefxsend[ind1][ind2]);
        }}
};

#define rBegin [](const char *msg, RtData &d) { rtosc::AutomationMgr &a = *(AutomationMgr*)d.obj
#define rEnd }

static int extract_num(const char *&msg)
{
    while(*msg && !isdigit(*msg)) msg++;
    int num = atoi(msg);
    while(isdigit(*msg)) msg++;
    return num;
}

static int get_next_int(const char *msg)
{
    return extract_num(msg);
}

static const Ports mapping_ports = {
    {"offset::f", rProp(parameter) rDefault(0) rShort("off") rLinear(-50, 50) rMap(unit, percent), 0,
        rBegin;
        int slot = d.idx[1];
        int param = d.idx[0];
        if(!strcmp("f",rtosc_argument_string(msg))) {
            a.setSlotSubOffset(slot, param, rtosc_argument(msg, 0).f);
            a.updateMapping(slot, param);
            d.broadcast(d.loc, "f", a.getSlotSubOffset(slot, param));
        } else
            d.reply(d.loc, "f", a.getSlotSubOffset(slot, param));
        rEnd},
    {"gain::f", rProp(parameter) rDefault(100) rShort("gain") rLinear(-200, 200) rMap(unit, percent), 0,
        rBegin;
        int slot = d.idx[1];
        int param = d.idx[0];
        if(!strcmp("f",rtosc_argument_string(msg))) {
            a.setSlotSubGain(slot, param, rtosc_argument(msg, 0).f);
            a.updateMapping(slot, param);
            d.broadcast(d.loc, "f", a.getSlotSubGain(slot, param));
        } else
            d.reply(d.loc, "f", a.getSlotSubGain(slot, param));
        rEnd},
};

static const Ports auto_param_ports = {
    {"used::T:F", rProp(parameter) rProp(read-only) rDoc("If automation is assigned to anything"), 0,
        rBegin;
        int slot  = d.idx[1];
        int param = d.idx[0];

        d.reply(d.loc, a.slots[slot].automations[param].used ? "T" : "F");
        rEnd},
    {"active::T:F", rProp(parameter) rDoc("If automation is being actively used"), 0,
        rBegin;
        int slot  = d.idx[1];
        int param = d.idx[0];
        if(rtosc_narguments(msg))
            a.slots[slot].automations[param].active = rtosc_argument(msg, 0).T;
        else
            d.reply(d.loc, a.slots[slot].automations[param].active ? "T" : "F");
        rEnd},
    {"path::s", rProp(parameter) rProp(read-only) rDoc("Path of parameter"), 0,
        rBegin;
        int slot  = d.idx[1];
        int param = d.idx[0];
        d.reply(d.loc, "s", a.slots[slot].automations[param].param_path);
        rEnd},
    {"clear:", rDoc("Clear automation param"), 0,
        rBegin;
        int slot = d.idx[1];
        int param = d.idx[0];
        a.clearSlotSub(slot, param);
        rEnd},
    {"mapping/", 0, &mapping_ports,
        rBegin;
        SNIP;
        mapping_ports.dispatch(msg, d);
        rEnd},

    //{"mapping", rDoc("Parameter mapping control"), 0,
    //    rBegin;
    //    int slot  = d.idx[1];
    //    int param = d.idx[0];
    //    if(!strcmp("b", rtosc_argument_string(msg))) {
    //        int    len  = rtosc_argument(msg, 0).b.len / sizeof(float);
    //        float *data = (float*)rtosc_argument(msg, 0).b.data;
    //    } else {
    //        d.reply(d.loc, "b",
    //                a.slots[slot].automations[param].map.npoints*sizeof(float),
    //                a.slots[slot].automations[param].map.control_points);
    //    }
    //    rEnd},
};

static const Ports slot_ports = {
    //{"learn-binding:s", rDoc("Create binding for automation path with midi-learn"), 0,
    //    rBegin;
    //    (void) m;
    //    //m->automate.createBinding(rtosc_argument(msg, 0).i,
    //    //                          rtosc_argument(msg, 1).s,
    //    //                          rtosc_argument(msg, 2).T);
    //    rEnd},
    //{"create-binding:s", rDoc("Create binding for automation path"), 0,
    //    rBegin;
    //    m->automate.createBinding(rtosc_argument(msg, 0).i,
    //                              rtosc_argument(msg, 1).s,
    //                              rtosc_argument(msg, 2).T);
    //    rEnd},
    {"value::f", rProp(no learn) rProp(parameter) rMap(default, 0.5) rLinear(0, 1) rDoc("Access current value in slot 'i' (0..1)"), 0,
        rBegin;
        int num = d.idx[0];
        if(!strcmp("f",rtosc_argument_string(msg))) {
            a.setSlot(num, rtosc_argument(msg, 0).f);
            d.broadcast(d.loc, "f", a.getSlot(num));
        } else
            d.reply(d.loc, "f", a.getSlot(num));
        rEnd},

    {"name::s", rProp(parameter) rDoc("Access name of automation slot"), 0,
        rBegin;
        int num = d.idx[0];
        if(!strcmp("s",rtosc_argument_string(msg))) {
            a.setName(num, rtosc_argument(msg, 0).s);
            d.broadcast(d.loc, "s", a.getName(num));
        } else
            d.reply(d.loc, "s", a.getName(num));
        rEnd},
    {"midi-cc::i", rProp(parameter) rMap(default, -1) rDoc("Access assigned midi CC slot") , 0,
        rBegin;
        int slot = d.idx[0];
        if(rtosc_narguments(msg))
            a.slots[slot].midi_cc = rtosc_argument(msg, 0).i;
        else
            d.reply(d.loc, "i", a.slots[slot].midi_cc);

        rEnd},
    {"active::T:F",  rProp(parameter) rMap(default, F) rDoc("If Slot is enabled"), 0,
        rBegin;
        int slot = d.idx[0];
        if(rtosc_narguments(msg))
            a.slots[slot].active = rtosc_argument(msg, 0).T;
        else
            d.reply(d.loc, a.slots[slot].active ? "T" : "F");
        rEnd},
    {"learning::i", rProp(parameter) rMap(default, -1) rDoc("If slot is trying to find a midi learn binding"), 0,
        rBegin;
        int slot = d.idx[0];
        d.reply(d.loc, "i", a.slots[slot].learning);
        rEnd},
    {"clear:", rDoc("Clear automation slot"), 0,
        rBegin;
        int slot = d.idx[0];
        a.clearSlot(slot);
        rEnd},
    {"param#4/", rDoc("Info on individual param mappings"), &auto_param_ports,
        rBegin;
        (void)a;
        d.push_index(get_next_int(msg));
        SNIP;
        auto_param_ports.dispatch(msg, d);
        d.pop_index();
        rEnd},
};

static const Ports automate_ports = {
    {"active-slot::i", rProp(parameter) rMap(min, -1) rMap(max, 16) rDoc("Active Slot for macro learning"), 0,
        rBegin;
        if(!strcmp("i",rtosc_argument_string(msg))) {
            a.active_slot = rtosc_argument(msg, 0).i;
            d.broadcast(d.loc, "i", a.active_slot);
        } else
            d.reply(d.loc, "i", a.active_slot);
        rEnd},
    {"learn-binding-new-slot:s", rDoc("Learn a parameter assigned to a new slot"), 0,
        rBegin;
        int free_slot = a.free_slot();
        if(free_slot >= 0) {
            a.createBinding(free_slot, rtosc_argument(msg, 0).s, true);
            a.active_slot = free_slot;
        }
        rEnd},
    {"learn-binding-same-slot:s", rDoc("Learn a parameter appending to the active-slot"), 0,
        rBegin;
        if(a.active_slot >= 0)
            a.createBinding(a.active_slot, rtosc_argument(msg, 0).s, true);
        rEnd},
    // TODO: remove rNoWalk
    {"slot#16/", rNoWalk rDoc("Parameters of individual automation slots"), &slot_ports,
        rBegin;
        (void)a;
        d.push_index(get_next_int(msg));
        SNIP;
        slot_ports.dispatch(msg, d);
        d.pop_index();
        rEnd},
    {"clear", rDoc("Clear all automation slots"), 0,
        rBegin;
        for(int i=0; i<a.nslots; ++i)
            a.clearSlot(i);
        rEnd},
    {"load-blob:b", rProp(internal) rDoc("Load blob from middleware"), 0,
        rBegin;
        auto &b = **(rtosc::AutomationMgr **)rtosc_argument(msg, 0).b.data;
        //XXX this code should likely be in rtosc
        for(int i=0; i<a.nslots; ++i) {
            auto &slota = a.slots[i];
            auto &slotb = b.slots[i];
            std::swap(slota.learning, slotb.learning);
            std::swap(slota.midi_cc,  slotb.midi_cc);
            std::swap(slota.used,     slotb.used);
            std::swap(slota.active,   slotb.active);
            for(int j=0; j<a.per_slot; ++j) {
                auto &aa = slota.automations[j];
                auto &ab = slotb.automations[j];
                std::swap(aa.used, ab.used);
                std::swap(aa.active, ab.active);
                std::swap(aa.param_path, ab.param_path);
                std::swap(aa.param_min, ab.param_min);
                std::swap(aa.param_max, ab.param_max);
                std::swap(aa.param_step, ab.param_step);
                std::swap(aa.param_type, ab.param_type);
                std::swap(aa.map.offset, ab.map.offset);
                std::swap(aa.map.gain, ab.map.gain);
                std::swap(aa.map.upoints, ab.map.upoints);
                for(int k=0; k<aa.map.npoints; ++k)
                    std::swap(aa.map.control_points[k], ab.map.control_points[k]);
            }
        }
        rEnd},
};

#undef  rBegin
#undef  rEnd
#define rBegin [](const char *msg, RtData &d) { Master *m = (Master*)d.obj
#define rEnd }

static const Ports watchPorts = {
    {"add:s", rDoc("Add synthesis state to watch"), 0,
        rBegin;
        m->watcher.add_watch(rtosc_argument(msg,0).s);
        rEnd},
};


extern const Ports bankPorts;
static const Ports master_ports = {
    rString(last_xmz, XMZ_PATH_MAX, "File name for last name loaded if any."),
    rRecursp(part, 16, "Part"),//NUM_MIDI_PARTS
    rRecursp(sysefx, 4, "System Effect"),//NUM_SYS_EFX
    rRecursp(insefx, 8, "Insertion Effect"),//NUM_INS_EFX
    rRecur(microtonal, "Microtonal Mapping Functionality"),
    rRecur(ctl, "Controller"),
    rArrayOption(Pinsparts, NUM_INS_EFX, rOpt(-2, Master), rOpt(-1, Off),
                 rOptions(Part1, Part2, Part3, Part4, Part5, Part6,
                          Part7, Part8, Part9, Part10, Part11, Part12,
                          Part13, Part14, Part15, Part16) rDefault(Off),
                 "Part to insert part onto"),
    {"Pkeyshift::i", rShort("key shift") rProp(parameter) rLinear(0,127)
        rDefault(64) rDoc("Global Key Shift"), 0, [](const char *m, RtData&d) {
        if(rtosc_narguments(m)==0) {
            d.reply(d.loc, "i", ((Master*)d.obj)->Pkeyshift);
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='i') {
            ((Master*)d.obj)->setPkeyshift(limit<char>(rtosc_argument(m,0).i,0,127));
            d.broadcast(d.loc, "i", ((Master*)d.obj)->Pkeyshift);}}},
    {"echo", rDoc("Hidden port to echo messages"), 0, [](const char *m, RtData&d) {
       d.reply(m-1);}},
    {"get-vu:", rDoc("Grab VU Data"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       d.reply("/vu-meter", "bb", sizeof(m->vu), &m->vu, sizeof(float)*NUM_MIDI_PARTS, m->vuoutpeakpart);}},
    {"vu-meter:", rDoc("Grab VU Data"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       char        types[6+NUM_MIDI_PARTS+1] = {0};
       rtosc_arg_t  args[6+NUM_MIDI_PARTS+1];
       for(int i=0; i<6+NUM_MIDI_PARTS; ++i)
           types[i] = 'f';
       args[0].f = m->vu.outpeakl;
       args[1].f = m->vu.outpeakr;
       args[2].f = m->vu.maxoutpeakl;
       args[3].f = m->vu.maxoutpeakr;
       args[4].f = m->vu.rmspeakl;
       args[5].f = m->vu.rmspeakr;
       for(int i=0; i<NUM_MIDI_PARTS; ++i)
           args[6+i].f = m->vuoutpeakpart[i];

       d.replyArray("/vu-meter", types, args);}},
    {"reset-vu:", rDoc("Grab VU Data"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       m->vuresetpeaks();}},
    {"load-part:ib", rProp(internal) rDoc("Load Part From Middleware"), 0, [](const char *msg, RtData &d) {
       Master *m =  (Master*)d.obj;
       Part   *p = *(Part**)rtosc_argument(msg, 1).b.data;
       int     i = rtosc_argument(msg, 0).i;
       m->part[i]->cloneTraits(*p);
       m->part[i]->kill_rt();
       d.reply("/free", "sb", "Part", sizeof(void*), &m->part[i]);
       m->part[i] = p;
       p->initialize_rt();
       for(int i=0; i<128; ++i)
           m->activeNotes[i] = 0;
       }},
    {"active_keys:", rProp("Obtain a list of active notes"), 0,
        rBegin;
        char keys[129] = {0};
        for(int i=0; i<128; ++i)
            keys[i] = m->activeNotes[i] ? 'T' : 'F';
        d.broadcast(d.loc, keys);
        rEnd},
    {"Pvolume::i", rShort("volume") rProp(parameter) rLinear(0,127)
        rDefault(80) rDoc("Master Volume"), 0,
        [](const char *m, rtosc::RtData &d) {
        if(rtosc_narguments(m)==0) {
            d.reply(d.loc, "i", ((Master*)d.obj)->Pvolume);
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='i') {
            ((Master*)d.obj)->setPvolume(limit<char>(rtosc_argument(m,0).i,0,127));
            d.broadcast(d.loc, "i", ((Master*)d.obj)->Pvolume);}}},
    {"volume::i", rShort("volume") rProp(parameter) rLinear(0,127)
        rDefault(80) rDoc("Master Volume"), 0,
        [](const char *m, rtosc::RtData &d) {
        if(rtosc_narguments(m)==0) {
            d.reply(d.loc, "i", ((Master*)d.obj)->Pvolume);
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='i') {
            ((Master*)d.obj)->setPvolume(limit<char>(rtosc_argument(m,0).i,0,127));
            d.broadcast(d.loc, "i", ((Master*)d.obj)->Pvolume);}}},
    {"Psysefxvol#" STRINGIFY(NUM_SYS_EFX) "/::i", 0, &sysefxPort,
        [](const char *msg, rtosc::RtData &d) {
            SNIP;
            sysefxPort.dispatch(msg, d);
        }},
    {"sysefxfrom#" STRINGIFY(NUM_SYS_EFX) "/", rDoc("Routing Between System Effects"), &sysefsendto,
        [](const char *msg, RtData&d) {
            SNIP;
            sysefsendto.dispatch(msg, d);
        }},

    {"noteOn:iii", rDoc("Noteon Event"), 0,
        [](const char *m,RtData &d){
            Master *M =  (Master*)d.obj;
            M->noteOn(rtosc_argument(m,0).i,rtosc_argument(m,1).i,rtosc_argument(m,2).i);}},

    {"noteOff:ii", rDoc("Noteoff Event"), 0,
        [](const char *m,RtData &d){
            Master *M =  (Master*)d.obj;
            M->noteOff(rtosc_argument(m,0).i,rtosc_argument(m,1).i);}},
    {"virtual_midi_cc:iii", rDoc("MIDI CC Event"), 0,
        [](const char *m,RtData &d){
            Master *M =  (Master*)d.obj;
            M->setController(rtosc_argument(m,0).i,rtosc_argument(m,1).i,rtosc_argument(m,2).i);}},

    {"setController:iii", rDoc("MIDI CC Event"), 0,
        [](const char *m,RtData &d){
            Master *M =  (Master*)d.obj;
            M->setController(rtosc_argument(m,0).i,rtosc_argument(m,1).i,rtosc_argument(m,2).i);}},
    {"Panic:", rDoc("Stop all sound"), 0,
        [](const char *, RtData &d) {
            Master &M =  *(Master*)d.obj;
            M.ShutUp();
        }},
    {"freeze_state:", rProp(internal) rDoc("Disable OSC event handling\n"
            "This sets up a read-only mode from which it's safe for another"
            " thread to save parameters"), 0,
        [](const char *,RtData &d) {
            Master *M =  (Master*)d.obj;
            std::atomic_thread_fence(std::memory_order_release);
            M->frozenState = true;
            d.reply("/state_frozen", "");}},
    {"thaw_state:", rProp(internal) rDoc("Resume handling OSC messages\n"
            "See /freeze_state for more information"), 0,
        [](const char *,RtData &d) {
            Master *M =  (Master*)d.obj;
            M->frozenState = false;}},
    {"automate/", rDoc("MIDI Learn/Plugin Automation support"), &automate_ports,
        [](const char *msg, RtData &d) {
            SNIP;
            d.obj = (void*)&((Master*)d.obj)->automate;
            automate_ports.dispatch(msg, d);
            }},
    {"close-ui:", rDoc("Request to close any connection named \"GUI\""), 0,
        [](const char *, RtData &d) {
       d.reply("/close-ui", "");}},
    {"add-rt-memory:bi", rProp(internal) rDoc("Add Additional Memory To RT MemPool"), 0,
        [](const char *msg, RtData &d)
        {
            Master &m = *(Master*)d.obj;
            char   *mem = *(char**)rtosc_argument(msg, 0).b.data;
            int     i = rtosc_argument(msg, 1).i;
            m.memory->addMemory(mem, i);
            m.pendingMemory = false;
        }},
    {"samplerate:", rMap(unit, Hz) rDoc("Get synthesizer sample rate"), 0, [](const char *, RtData &d) {
            Master &m = *(Master*)d.obj;
            d.reply("/samplerate", "f", m.synth.samplerate_f);
        }},
    {"oscilsize:", rDoc("Get synthesizer oscillator size"), 0, [](const char *, RtData &d) {
            Master &m = *(Master*)d.obj;
            d.reply("/oscilsize", "f", m.synth.oscilsize_f);
            d.reply("/oscilsize", "i", m.synth.oscilsize);
        }},
    {"undo_pause:",rProp(internal) rDoc("pause undo event recording"),0,
        [](const char *, rtosc::RtData &d) {d.reply("/undo_pause", "");}},
    {"undo_resume:",rProp(internal) rDoc("resume undo event recording"),0,
        [](const char *, rtosc::RtData &d) {d.reply("/undo_resume", "");}},
    {"config/", rNoWalk rDoc("Top Level Application Configuration Parameters"),
        &Config::ports, [](const char *, rtosc::RtData &d){d.forward();}},
    {"presets/", rDoc("Parameter Presets"), &preset_ports, rBOIL_BEGIN
        SNIP
            preset_ports.dispatch(msg, data);
        rBOIL_END},
    {"HDDRecorder/preparefile:s", rDoc("Init WAV file"), 0, [](const char *msg, RtData &d) {
       Master *m = (Master*)d.obj;
       m->HDDRecorder.preparefile(rtosc_argument(msg, 0).s, 1);}},
    {"HDDRecorder/start:", rDoc("Start recording"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       m->HDDRecorder.start();}},
    {"HDDRecorder/stop:", rDoc("Stop recording"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       m->HDDRecorder.stop();}},
    {"HDDRecorder/pause:", rDoc("Pause recording"), 0, [](const char *, RtData &d) {
       Master *m = (Master*)d.obj;
       m->HDDRecorder.pause();}},
    {"watch/", rDoc("Interface to grab out live synthesis state"), &watchPorts,
        rBOIL_BEGIN;
        SNIP;
        watchPorts.dispatch(msg, data);
        rBOIL_END},
    {"bank/", rDoc("Controls for instrument banks"), &bankPorts,
            [](const char*,RtData&) {}},
    {"learn:s", rProp(depricated) rDoc("MIDI Learn"), 0,
        rBegin;
        int free_slot = m->automate.free_slot();
        if(free_slot >= 0) {
            m->automate.createBinding(free_slot, rtosc_argument(msg, 0).s, true);
            m->automate.active_slot = free_slot;
        }
        rEnd},
};

#undef rBegin
#undef rEnd

const Ports &Master::ports = master_ports;

class DataObj:public rtosc::RtData
{
    public:
        DataObj(char *loc_, size_t loc_size_, void *obj_, rtosc::ThreadLink *bToU_)
        {
            memset(loc_, 0, loc_size_);
            loc      = loc_;
            loc_size = loc_size_;
            obj      = obj_;
            bToU     = bToU_;
            forwarded = false;
        }

        virtual void replyArray(const char *path, const char *args, rtosc_arg_t *vals) override
        {
            char *buffer = bToU->buffer();
            rtosc_amessage(buffer,bToU->buffer_size(),path,args,vals);
            reply(buffer);
        }
        virtual void reply(const char *path, const char *args, ...) override
        {
            va_list va;
            va_start(va,args);
            char *buffer = bToU->buffer();
            rtosc_vmessage(buffer,bToU->buffer_size(),path,args,va);
            reply(buffer);
            va_end(va);
        }
        virtual void reply(const char *msg) override
        {
            if(rtosc_message_length(msg, -1) == 0)
                fprintf(stderr, "Warning: Invalid Rtosc message '%s'\n", msg);
            bToU->raw_write(msg);
        }
        virtual void broadcast(const char *path, const char *args, ...) override{
            va_list va;
            va_start(va,args);
            reply("/broadcast", "");
            char *buffer = bToU->buffer();
            rtosc_vmessage(buffer,bToU->buffer_size(),path,args,va);
            reply(buffer);
            va_end(va);
        }
        virtual void broadcast(const char *msg) override
        {
            reply("/broadcast", "");
            reply(msg);
        }

        virtual void forward(const char *reason) override
        {
            assert(message);
            reply("/forward", "");
            printf("forwarding '%s'\n", message);
            forwarded = true;
        }
        bool forwarded;
    private:
        rtosc::ThreadLink *bToU;
};

vuData::vuData(void)
    :outpeakl(0.0f), outpeakr(0.0f), maxoutpeakl(0.0f), maxoutpeakr(0.0f),
      rmspeakl(0.0f), rmspeakr(0.0f), clipped(0)
{}

void Master::saveAutomation(XMLwrapper &xml, const rtosc::AutomationMgr &midi)
{
    xml.beginbranch("automation");
    {
        XmlNode metadata("mgr-info");
        metadata["nslots"]       = to_s(midi.nslots);
        metadata["nautomations"] = to_s(midi.per_slot);
        metadata["ncontrol"]     = to_s(midi.slots[0].automations[0].map.npoints);
        xml.add(metadata);

        for(int i=0; i<midi.nslots; ++i) {
            const auto &slot = midi.slots[i];
            if(!slot.used)
                continue;
            xml.beginbranch("slot", i);
            XmlNode params("params");
            params["midi-cc"] = to_s(slot.midi_cc);
            xml.add(params);
            for(int j=0; j<midi.per_slot; ++j) {
                const auto &au = slot.automations[j];
                if(!au.used)
                    continue;
                xml.beginbranch("automation", j);
                XmlNode automation("params");
                automation["path"] = au.param_path;
                XmlNode mapping("mapping");
                mapping["gain"]   = to_s(au.map.gain);
                mapping["offset"] = to_s(au.map.offset);
                xml.add(automation);
                xml.add(mapping);
                xml.endbranch();
            }

            xml.endbranch();
        }
    }
    xml.endbranch();
}

void Master::loadAutomation(XMLwrapper &xml, rtosc::AutomationMgr &midi)
{
    if(xml.enterbranch("automation")) {
        for(int i=0; i<midi.nslots; ++i) {
            auto &slot = midi.slots[i];
            if(xml.enterbranch("slot", i)) {
                for(int j=0; j<midi.per_slot; ++j) {
                    auto &au = slot.automations[j];
                    if(xml.enterbranch("automation", j)) {
                        float gain       = 1.0;
                        float offset     = 0.0;
                        std::string path = "";
                        for(auto node:xml.getBranch()) {
                            if(node.name == "params")
                                path = node["path"];
                            else if(node.name == "mapping") {
                                gain   = atof(node["gain"].c_str());
                                offset = atof(node["offset"].c_str());
                            }
                        }
                        printf("createBinding(%d, %s, false)\n", i, path.c_str());
                        midi.createBinding(i, path.c_str(), false);
                        midi.setSlotSubGain(i, j, gain);
                        midi.setSlotSubOffset(i, j, offset);
                        xml.exitbranch();
                    }
                }
                for(auto node:xml.getBranch())
                    if(node.name == "params")
                        slot.midi_cc = atoi(node["midi-cc"].c_str());
                xml.exitbranch();
            }
        }
        xml.exitbranch();
    }
}

Master::Master(const SYNTH_T &synth_, Config* config)
    :HDDRecorder(synth_), time(synth_), ctl(synth_, &time),
    microtonal(config->cfg.GzipCompression), bank(config),
    automate(16,4,8),
    frozenState(false), pendingMemory(false),
    synth(synth_), gzip_compression(config->cfg.GzipCompression)
{
    bToU = NULL;
    uToB = NULL;

    //Setup MIDI Learn
    automate.set_ports(master_ports);
    automate.set_instance(this);
    //midi.frontend = [this](const char *msg) {bToU->raw_write(msg);};
    automate.backend  = [this](const char *msg) {applyOscEvent(msg);};

    memory = new AllocatorClass();
    swaplr = 0;
    off  = 0;
    smps = 0;
    bufl = new float[synth.buffersize];
    bufr = new float[synth.buffersize];

    last_xmz[0] = 0;
    fft = new FFTwrapper(synth.oscilsize);

    shutup = 0;
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        vuoutpeakpart[npart] = 1e-9;
        fakepeakpart[npart]  = 0;
    }

    ScratchString ss;
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        part[npart] = new Part(*memory, synth, time, config->cfg.GzipCompression,
                               config->cfg.Interpolation, &microtonal, fft, &watcher,
                               (ss+"/part"+npart+"/").c_str);

    //Insertion Effects init
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        insefx[nefx] = new EffectMgr(*memory, synth, 1, &time);

    //System Effects init
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        sysefx[nefx] = new EffectMgr(*memory, synth, 0, &time);

    //Note Visualization
    for(int i=0; i<128; ++i)
        activeNotes[i] = 0;

    defaults();

    mastercb = 0;
    mastercb_ptr = 0;
}

void Master::applyOscEvent(const char *msg)
{
    char loc_buf[1024];
    DataObj d{loc_buf, 1024, this, bToU};
    memset(loc_buf, 0, sizeof(loc_buf));
    d.matches = 0;

    if(strcmp(msg, "/get-vu") && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 5 + 30, 0 + 40);
        fprintf(stdout, "backend[*]: '%s'<%s>\n", msg,
                rtosc_argument_string(msg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    ports.dispatch(msg, d, true);
    if(d.matches == 0 && !d.forwarded)
        fprintf(stderr, "Unknown path '%s:%s'\n", msg, rtosc_argument_string(msg));
    if(d.forwarded)
        bToU->raw_write(msg);
}

void Master::defaults()
{
    volume = 1.0f;
    setPvolume(80);
    setPkeyshift(64);

    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        part[npart]->defaults();
        part[npart]->partno  = npart % NUM_MIDI_CHANNELS;
        part[npart]->Prcvchn = npart % NUM_MIDI_CHANNELS;
    }

    partonoff(0, 1); //enable the first part

    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx) {
        insefx[nefx]->defaults();
        Pinsparts[nefx] = -1;
    }

    //System Effects init
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx) {
        sysefx[nefx]->defaults();
        for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
            setPsysefxvol(npart, nefx, 0);

        for(int nefxto = 0; nefxto < NUM_SYS_EFX; ++nefxto)
            setPsysefxsend(nefx, nefxto, 0);
    }

    microtonal.defaults();
    ShutUp();
}

/*
 * Note On Messages (velocity=0 for NoteOff)
 */
void Master::noteOn(char chan, char note, char velocity)
{
    if(velocity) {
        for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
            if(chan == part[npart]->Prcvchn) {
                fakepeakpart[npart] = velocity * 2;
                if(part[npart]->Penabled)
                    part[npart]->NoteOn(note, velocity, keyshift);
            }
        }
        activeNotes[(int)note] = 1;
    }
    else
        this->noteOff(chan, note);
    HDDRecorder.triggernow();
}

/*
 * Note Off Messages
 */
void Master::noteOff(char chan, char note)
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if((chan == part[npart]->Prcvchn) && part[npart]->Penabled)
            part[npart]->NoteOff(note);
    activeNotes[(int)note] = 0;
}

/*
 * Pressure Messages (velocity=0 for NoteOff)
 */
void Master::polyphonicAftertouch(char chan, char note, char velocity)
{
    if(velocity) {
        for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
            if(chan == part[npart]->Prcvchn)
                if(part[npart]->Penabled)
                    part[npart]->PolyphonicAftertouch(note, velocity, keyshift);

    }
    else
        this->noteOff(chan, note);
}

/*
 * Controllers
 */
void Master::setController(char chan, int type, int par)
{
    if(frozenState)
        return;
    automate.handleMidi(chan, type, par);
    if((type == C_dataentryhi) || (type == C_dataentrylo)
       || (type == C_nrpnhi) || (type == C_nrpnlo)) { //Process RPN and NRPN by the Master (ignore the chan)
        ctl.setparameternumber(type, par);

        int parhi = -1, parlo = -1, valhi = -1, vallo = -1;
        if(ctl.getnrpn(&parhi, &parlo, &valhi, &vallo) == 0) { //this is NRPN
            switch(parhi) {
                case 0x04: //System Effects
                    if(parlo < NUM_SYS_EFX)
                        sysefx[parlo]->seteffectparrt(valhi, vallo);
                    break;
                case 0x08: //Insertion Effects
                    if(parlo < NUM_INS_EFX)
                        insefx[parlo]->seteffectparrt(valhi, vallo);
                    break;
            }
        }
    } else {  //other controllers
        for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) //Send the controller to all part assigned to the channel
            if((chan == part[npart]->Prcvchn) && (part[npart]->Penabled != 0))
                part[npart]->SetController(type, par);

        if(type == C_allsoundsoff) { //cleanup insertion/system FX
            for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
                sysefx[nefx]->cleanup();
            for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
                insefx[nefx]->cleanup();
        }
    }
}

void Master::vuUpdate(const float *outl, const float *outr)
{
    //Peak computation (for vumeters)
    vu.outpeakl = 1e-12;
    vu.outpeakr = 1e-12;
    for(int i = 0; i < synth.buffersize; ++i) {
        if(fabs(outl[i]) > vu.outpeakl)
            vu.outpeakl = fabs(outl[i]);
        if(fabs(outr[i]) > vu.outpeakr)
            vu.outpeakr = fabs(outr[i]);
    }
    if((vu.outpeakl > 1.0f) || (vu.outpeakr > 1.0f))
        vu.clipped = 1;
    if(vu.maxoutpeakl < vu.outpeakl)
        vu.maxoutpeakl = vu.outpeakl;
    if(vu.maxoutpeakr < vu.outpeakr)
        vu.maxoutpeakr = vu.outpeakr;

    //RMS Peak computation (for vumeters)
    vu.rmspeakl = 1e-12;
    vu.rmspeakr = 1e-12;
    for(int i = 0; i < synth.buffersize; ++i) {
        vu.rmspeakl += outl[i] * outl[i];
        vu.rmspeakr += outr[i] * outr[i];
    }
    vu.rmspeakl = sqrt(vu.rmspeakl / synth.buffersize_f);
    vu.rmspeakr = sqrt(vu.rmspeakr / synth.buffersize_f);

    //Part Peak computation (for Part vumeters or fake part vumeters)
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        vuoutpeakpart[npart] = 1.0e-12f;
        if(part[npart]->Penabled != 0) {
            float *outl = part[npart]->partoutl,
            *outr = part[npart]->partoutr;
            for(int i = 0; i < synth.buffersize; ++i) {
                float tmp = fabs(outl[i] + outr[i]);
                if(tmp > vuoutpeakpart[npart])
                    vuoutpeakpart[npart] = tmp;
            }
            vuoutpeakpart[npart] *= volume;
        }
        else
        if(fakepeakpart[npart] > 1)
            fakepeakpart[npart]--;
    }
}

/*
 * Enable/Disable a part
 */
void Master::partonoff(int npart, int what)
{
    if(npart >= NUM_MIDI_PARTS)
        return;
    if(what == 0) { //disable part
        fakepeakpart[npart]   = 0;
        part[npart]->Penabled = 0;
        part[npart]->cleanup();
        for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx) {
            if(Pinsparts[nefx] == npart)
                insefx[nefx]->cleanup();
        }
    }
    else {  //enabled
        part[npart]->Penabled = 1;
        fakepeakpart[npart]   = 0;
    }
}

void Master::setMasterChangedCallback(void(*cb)(void*,Master*), void *ptr)
{
    mastercb     = cb;
    mastercb_ptr = ptr;
}

#if 0
template <class T>
struct def_skip
{
	static void skip(const char*& argptr) { argptr += sizeof(T); }
};

template <class T>
struct str_skip
{
	static void skip(const char*& argptr) { while(argptr++); /*TODO: 4 padding */ }
};

template<class T, class Display = T, template<class TMP> class SkipsizeFunc = def_skip>
void _dump_prim_arg(const char*& argptr, std::ostream& os)
{
	os << ' ' << (Display)*(const T*)argptr;
	SkipsizeFunc<T>::skip(argptr);
}

void dump_msg(const char* ptr, std::ostream& os = std::cerr)
{
	assert(*ptr == '/');
	os << ptr;

	while(*++ptr) ; // skip address
	while(!*++ptr) ; // skip 0s

	assert(*ptr == ',');
	os << ' ' << (ptr + 1);

	const char* argptr = ptr;
	while(*++argptr) ; // skip type string
	while(!*++argptr) ; // skip 0s

	char c;
	while((c = *++ptr))
	{
		switch(c)
		{
			case 'i':
				_dump_prim_arg<int32_t>(argptr, os); break;
			case 'c':
				_dump_prim_arg<int32_t, char>(argptr, os); break;
		//	case 's':
		//		_dump_prim_arg<char, const char*>(argptr, os); break;
			default:
				exit(1);
		}
	}

}
#endif
int msg_id=0;

bool Master::runOSC(float *outl, float *outr, bool offline)
{
    //Handle user events TODO move me to a proper location
    char loc_buf[1024];
    DataObj d{loc_buf, 1024, this, bToU};
    memset(loc_buf, 0, sizeof(loc_buf));
    int events = 0;
    while(uToB && uToB->hasNext() && events < 100) {
        const char *msg = uToB->read();

        if(!strcmp(msg, "/load-master")) {
            Master *this_master = this;
            Master *new_master  = *(Master**)rtosc_argument(msg, 0).b.data;
            if(!offline)
                new_master->AudioOut(outl, outr);
            Nio::masterSwap(new_master);
            if (mastercb)
                mastercb(mastercb_ptr, new_master);
            bToU->write("/free", "sb", "Master", sizeof(Master*), &this_master);
            return false;
        }

        //XXX yes, this is not realtime safe, but it is useful...
        if(strcmp(msg, "/get-vu") && false) {
            fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 5 + 30, 0 + 40);
            fprintf(stdout, "backend[%d]: '%s'<%s>\n", msg_id++, msg,
                    rtosc_argument_string(msg));
            fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }
        ports.dispatch(msg, d, true);
        events++;
        if(!d.matches) {
            //workaround for requesting voice status
            int a=0, b=0, c=0;
            char e=0;
            if(4 == sscanf(msg, "/part%d/kit%d/adpars/VoicePar%d/Enable%c", &a, &b, &c, &e)) {
                d.reply(msg, "F");
                d.matches++;
            }
        }
        if(!d.matches) {// && !ports.apropos(msg)) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown address<BACKEND:%s> '%s:%s'\n",
                    offline ? "offline" : "online",
                    uToB->peak(),
                    rtosc_argument_string(uToB->peak()));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }
    }

    if(automate.damaged) {
        d.broadcast("/damage", "s", "/automate/");
        automate.damaged = 0;
    }

    if(events>1 && false)
        fprintf(stderr, "backend: %d events per cycle\n",events);

    return true;
}

/*
 * Master audio out (the final sound)
 */
bool Master::AudioOut(float *outr, float *outl)
{
    //Danger Limits
    if(memory->lowMemory(2,1024*1024))
        printf("QUITE LOW MEMORY IN THE RT POOL BE PREPARED FOR WEIRD BEHAVIOR!!\n");
    //Normal Limits
    if(!pendingMemory && memory->lowMemory(4,1024*1024)) {
        printf("Requesting more memory\n");
        bToU->write("/request-memory", "");
        pendingMemory = true;
    }

    //work through events
    if(!runOSC(outl, outr, false))
        return false;


    //Handle watch points
    if(bToU)
        watcher.write_back = bToU;
    watcher.tick();


    //Swaps the Left channel with Right Channel
    if(swaplr)
        swap(outl, outr);

    //clean up the output samples (should not be needed?)
    memset(outl, 0, synth.bufferbytes);
    memset(outr, 0, synth.bufferbytes);

    //Compute part samples and store them part[npart]->partoutl,partoutr
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if(part[npart]->Penabled)
            part[npart]->ComputePartSmps();

    //Insertion effects
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        if(Pinsparts[nefx] >= 0) {
            int efxpart = Pinsparts[nefx];
            if(part[efxpart]->Penabled)
                insefx[nefx]->out(part[efxpart]->partoutl,
                                  part[efxpart]->partoutr);
        }


    //Apply the part volumes and pannings (after insertion effects)
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        if(!part[npart]->Penabled)
            continue;

        Stereo<float> newvol(part[npart]->volume),
        oldvol(part[npart]->oldvolumel,
               part[npart]->oldvolumer);

        float pan = part[npart]->panning;
        if(pan < 0.5f)
            newvol.l *= pan * 2.0f;
        else
            newvol.r *= (1.0f - pan) * 2.0f;
        //if(npart==0)
        //printf("[%d]vol = %f->%f\n", npart, oldvol.l, newvol.l);

        //the volume or the panning has changed and needs interpolation
        if(ABOVE_AMPLITUDE_THRESHOLD(oldvol.l, newvol.l)
           || ABOVE_AMPLITUDE_THRESHOLD(oldvol.r, newvol.r)) {
            for(int i = 0; i < synth.buffersize; ++i) {
                Stereo<float> vol(INTERPOLATE_AMPLITUDE(oldvol.l, newvol.l,
                                                        i, synth.buffersize),
                                  INTERPOLATE_AMPLITUDE(oldvol.r, newvol.r,
                                                        i, synth.buffersize));
                part[npart]->partoutl[i] *= vol.l;
                part[npart]->partoutr[i] *= vol.r;
            }
            part[npart]->oldvolumel = newvol.l;
            part[npart]->oldvolumer = newvol.r;
        }
        else {
            for(int i = 0; i < synth.buffersize; ++i) { //the volume did not changed
                part[npart]->partoutl[i] *= newvol.l;
                part[npart]->partoutr[i] *= newvol.r;
            }
        }
    }


    //System effects
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx) {
        if(sysefx[nefx]->geteffect() == 0)
            continue;  //the effect is disabled

        float tmpmixl[synth.buffersize];
        float tmpmixr[synth.buffersize];
        //Clean up the samples used by the system effects
        memset(tmpmixl, 0, synth.bufferbytes);
        memset(tmpmixr, 0, synth.bufferbytes);

        //Mix the channels according to the part settings about System Effect
        for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
            //skip if the part has no output to effect
            if(Psysefxvol[nefx][npart] == 0)
                continue;

            //skip if the part is disabled
            if(part[npart]->Penabled == 0)
                continue;

            //the output volume of each part to system effect
            const float vol = sysefxvol[nefx][npart];
            for(int i = 0; i < synth.buffersize; ++i) {
                tmpmixl[i] += part[npart]->partoutl[i] * vol;
                tmpmixr[i] += part[npart]->partoutr[i] * vol;
            }
        }

        // system effect send to next ones
        for(int nefxfrom = 0; nefxfrom < nefx; ++nefxfrom)
            if(Psysefxsend[nefxfrom][nefx] != 0) {
                const float vol = sysefxsend[nefxfrom][nefx];
                for(int i = 0; i < synth.buffersize; ++i) {
                    tmpmixl[i] += sysefx[nefxfrom]->efxoutl[i] * vol;
                    tmpmixr[i] += sysefx[nefxfrom]->efxoutr[i] * vol;
                }
            }

        sysefx[nefx]->out(tmpmixl, tmpmixr);

        //Add the System Effect to sound output
        const float outvol = sysefx[nefx]->sysefxgetvolume();
        for(int i = 0; i < synth.buffersize; ++i) {
            outl[i] += tmpmixl[i] * outvol;
            outr[i] += tmpmixr[i] * outvol;
        }
    }

    //Mix all parts
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        if(part[npart]->Penabled)   //only mix active parts
            for(int i = 0; i < synth.buffersize; ++i) { //the volume did not changed
                outl[i] += part[npart]->partoutl[i];
                outr[i] += part[npart]->partoutr[i];
            }

    //Insertion effects for Master Out
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        if(Pinsparts[nefx] == -2)
            insefx[nefx]->out(outl, outr);


    //Master Volume
    for(int i = 0; i < synth.buffersize; ++i) {
        outl[i] *= volume;
        outr[i] *= volume;
    }

    vuUpdate(outl, outr);

    //Shutup if it is asked (with fade-out)
    if(shutup) {
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmp = (synth.buffersize_f - i) / synth.buffersize_f;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        ShutUp();
    }

    //update the global frame timer
    time++;

#ifdef DEMO_VERSION
    double seconds = time.time()*synth.buffersize_f/synth.samplerate_f;
    if(seconds > 10*60) {//10 minute trial
        shutup = true;
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmp = (synth.buffersize_f - i) / synth.buffersize_f;
            outl[i] *= 0.0f;
            outr[i] *= 0.0f;
        }
    }
#endif

    //Update pulse
    last_ack = last_beat;


    return true;
}

//TODO review the respective code from yoshimi for this
//If memory serves correctly, libsamplerate was used
void Master::GetAudioOutSamples(size_t nsamples,
                                unsigned samplerate,
                                float *outl,
                                float *outr)
{
    off_t out_off = 0;

    //Fail when resampling rather than doing a poor job
    if(synth.samplerate != samplerate) {
        printf("darn it: %d vs %d\n", synth.samplerate, samplerate);
        return;
    }

    while(nsamples) {
        //use all available samples
        if(nsamples >= smps) {
            memcpy(outl + out_off, bufl + off, sizeof(float) * smps);
            memcpy(outr + out_off, bufr + off, sizeof(float) * smps);
            nsamples -= smps;

            //generate samples
            if (! AudioOut(bufl, bufr))
                return;

            off  = 0;
            out_off  += smps;
            smps = synth.buffersize;
        }
        else {   //use some samples
            memcpy(outl + out_off, bufl + off, sizeof(float) * nsamples);
            memcpy(outr + out_off, bufr + off, sizeof(float) * nsamples);
            smps    -= nsamples;
            off     += nsamples;
            nsamples = 0;
        }
    }
}

Master::~Master()
{
    delete []bufl;
    delete []bufr;

    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        delete part[npart];
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        delete insefx[nefx];
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        delete sysefx[nefx];

    delete fft;
    delete memory;
}


/*
 * Parameter control
 */
void Master::setPvolume(char Pvolume_)
{
    Pvolume = Pvolume_;
    volume  = dB2rap((Pvolume - 96.0f) / 96.0f * 40.0f);
}

void Master::setPkeyshift(char Pkeyshift_)
{
    Pkeyshift = Pkeyshift_;
    keyshift  = (int)Pkeyshift - 64;
}


void Master::setPsysefxvol(int Ppart, int Pefx, char Pvol)
{
    Psysefxvol[Pefx][Ppart] = Pvol;
    sysefxvol[Pefx][Ppart]  = powf(0.1f, (1.0f - Pvol / 96.0f) * 2.0f);
}

void Master::setPsysefxsend(int Pefxfrom, int Pefxto, char Pvol)
{
    Psysefxsend[Pefxfrom][Pefxto] = Pvol;
    sysefxsend[Pefxfrom][Pefxto]  = powf(0.1f, (1.0f - Pvol / 96.0f) * 2.0f);
}


/*
 * Panic! (Clean up all parts and effects)
 */
void Master::ShutUp()
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        part[npart]->cleanup();
        fakepeakpart[npart] = 0;
    }
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
        insefx[nefx]->cleanup();
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
        sysefx[nefx]->cleanup();
    for(int i = 0; i < int(sizeof(activeNotes)/sizeof(activeNotes[0])); ++i)
        activeNotes[i] = 0;
    vuresetpeaks();
    shutup = 0;
}


/*
 * Reset peaks and clear the "cliped" flag (for VU-meter)
 */
void Master::vuresetpeaks()
{
    vu.outpeakl    = 1e-9;
    vu.outpeakr    = 1e-9;
    vu.maxoutpeakl = 1e-9;
    vu.maxoutpeakr = 1e-9;
    vu.clipped     = 0;
}

void Master::applyparameters(void)
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
        part[npart]->applyparameters();
}

void Master::initialize_rt(void)
{
    for(int i=0; i<NUM_SYS_EFX; ++i)
        sysefx[i]->init();
    for(int i=0; i<NUM_INS_EFX; ++i)
        insefx[i]->init();

    for(int i=0; i<NUM_MIDI_PARTS; ++i)
        part[i]->initialize_rt();
}

void Master::add2XML(XMLwrapper& xml)
{
    xml.addpar("volume", Pvolume);
    xml.addpar("key_shift", Pkeyshift);
    xml.addparbool("nrpn_receive", ctl.NRPN.receive);

    xml.beginbranch("MICROTONAL");
    microtonal.add2XML(xml);
    xml.endbranch();

    saveAutomation(xml, automate);

    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        xml.beginbranch("PART", npart);
        part[npart]->add2XML(xml);
        xml.endbranch();
    }

    xml.beginbranch("SYSTEM_EFFECTS");
    for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx) {
        xml.beginbranch("SYSTEM_EFFECT", nefx);
        xml.beginbranch("EFFECT");
        sysefx[nefx]->add2XML(xml);
        xml.endbranch();

        for(int pefx = 0; pefx < NUM_MIDI_PARTS; ++pefx) {
            xml.beginbranch("VOLUME", pefx);
            xml.addpar("vol", Psysefxvol[nefx][pefx]);
            xml.endbranch();
        }

        for(int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; ++tonefx) {
            xml.beginbranch("SENDTO", tonefx);
            xml.addpar("send_vol", Psysefxsend[nefx][tonefx]);
            xml.endbranch();
        }


        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("INSERTION_EFFECTS");
    for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx) {
        xml.beginbranch("INSERTION_EFFECT", nefx);
        xml.addpar("part", Pinsparts[nefx]);

        xml.beginbranch("EFFECT");
        insefx[nefx]->add2XML(xml);
        xml.endbranch();
        xml.endbranch();
    }

    xml.endbranch();
}


int Master::getalldata(char **data)
{
    XMLwrapper xml;

    xml.beginbranch("MASTER");

    add2XML(xml);

    xml.endbranch();

    *data = xml.getXMLdata();
    return strlen(*data) + 1;
}

void Master::putalldata(const char *data)
{
    XMLwrapper xml;
    if(!xml.putXMLdata(data)) {
        return;
    }

    if(xml.enterbranch("MASTER") == 0)
        return;

    getfromXML(xml);

    xml.exitbranch();
}

int Master::saveXML(const char *filename)
{
    XMLwrapper xml;

    xml.beginbranch("MASTER");
    add2XML(xml);
    xml.endbranch();

    return xml.saveXMLfile(filename, gzip_compression);
}


int Master::loadXML(const char *filename)
{
    XMLwrapper xml;

    if(xml.loadXMLfile(filename) < 0) {
        return -1;
    }

    if(xml.enterbranch("MASTER") == 0)
        return -10;

    getfromXML(xml);
    xml.exitbranch();

    initialize_rt();
    return 0;
}

void Master::getfromXML(XMLwrapper& xml)
{
    setPvolume(xml.getpar127("volume", Pvolume));
    setPkeyshift(xml.getpar127("key_shift", Pkeyshift));
    ctl.NRPN.receive = xml.getparbool("nrpn_receive", ctl.NRPN.receive);


    part[0]->Penabled = 0;
    for(int npart = 0; npart < NUM_MIDI_PARTS; ++npart) {
        if(xml.enterbranch("PART", npart) == 0)
            continue;
        part[npart]->getfromXML(xml);
        xml.exitbranch();
    }

    if(xml.enterbranch("MICROTONAL")) {
        microtonal.getfromXML(xml);
        xml.exitbranch();
    }

    loadAutomation(xml, automate);

    sysefx[0]->changeeffect(0);
    if(xml.enterbranch("SYSTEM_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx) {
            if(xml.enterbranch("SYSTEM_EFFECT", nefx) == 0)
                continue;
            if(xml.enterbranch("EFFECT")) {
                sysefx[nefx]->getfromXML(xml);
                xml.exitbranch();
            }

            for(int partefx = 0; partefx < NUM_MIDI_PARTS; ++partefx) {
                if(xml.enterbranch("VOLUME", partefx) == 0)
                    continue;
                setPsysefxvol(partefx, nefx,
                              xml.getpar127("vol", Psysefxvol[partefx][nefx]));
                xml.exitbranch();
            }

            for(int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; ++tonefx) {
                if(xml.enterbranch("SENDTO", tonefx) == 0)
                    continue;
                setPsysefxsend(nefx, tonefx,
                               xml.getpar127("send_vol",
                                              Psysefxsend[nefx][tonefx]));
                xml.exitbranch();
            }
            xml.exitbranch();
        }
        xml.exitbranch();
    }


    if(xml.enterbranch("INSERTION_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx) {
            if(xml.enterbranch("INSERTION_EFFECT", nefx) == 0)
                continue;
            Pinsparts[nefx] = xml.getpar("part",
                                          Pinsparts[nefx],
                                          -2,
                                          NUM_MIDI_PARTS);
            if(xml.enterbranch("EFFECT")) {
                insefx[nefx]->getfromXML(xml);
                xml.exitbranch();
            }
            xml.exitbranch();
        }

        xml.exitbranch();
    }
}

static rtosc_version version_in_rtosc_fmt()
{
    return rtosc_version
    {
        (unsigned char) version.get_major(),
        (unsigned char) version.get_minor(),
        (unsigned char) version.get_revision()
    };
}

char* Master::getXMLData()
{
    XMLwrapper xml;

    xml.beginbranch("MASTER");
    add2XML(xml);
    xml.endbranch();

    return xml.getXMLdata();
}

int Master::saveOSC(const char *filename)
{
    std::string savefile = rtosc::save_to_file(ports, this,
                                               "ZynAddSubFX",
                                               version_in_rtosc_fmt());

    zyncarla::Config config;
    zyncarla::SYNTH_T* synth = new zyncarla::SYNTH_T;
    synth->buffersize = 256;
    synth->samplerate = 48000;
    synth->alias();

    zyncarla::Master master2(*synth, &config);
    int rval = master2.loadOSCFromStr(savefile.c_str());


    if(rval < 0)
    {
        std::cerr << "invalid savefile!" << std::endl;
        std::cerr << "complete savefile:" << std::endl;
        std::cerr << savefile << std::endl;
        std::cerr << "first entry that could not be parsed:" << std::endl;

        for(int i = -rval + 1; savefile[i]; ++i)
        if(savefile[i] == '\n')
        {
            savefile.resize(i);
            break;
        }
        std::cerr << (savefile.c_str() - rval) << std::endl;

        rval = -1;
    }
    else
    {
        char* xml = getXMLData(),
            * xml2 = master2.getXMLData();

        rval = strcmp(xml, xml2) ? -1 : 0;

        if(rval == 0)
        {
            if(filename)
            {
                std::ofstream ofs(filename);
                ofs << savefile;
            }
            else if(!filename)
                std::cout << savefile << std::endl;
        }
        else
        {
            std::cout << savefile << std::endl;
            std::cerr << "Can not write OSC savefile!! (see tmp1.txt and tmp2.txt)"
                      << std::endl;
            std::ofstream tmp1("tmp1.txt"), tmp2("tmp2.txt");
            tmp1 << xml;
            tmp2 << xml2;
        }

        free(xml);
        free(xml2);
    }
    return rval;
}

int Master::loadOSCFromStr(const char *filename)
{
    return rtosc::load_from_file(filename,
                                 ports, this,
                                 "ZynAddSubFX", version_in_rtosc_fmt());
}

string loadfile(string fname)
{
    std::ifstream t(fname.c_str());
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    return str;
}

int Master::loadOSC(const char *filename)
{
    int rval = loadOSCFromStr(loadfile(filename).c_str());
    return rval < 0 ? rval : 0;
}


}
