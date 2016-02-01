/*
  ZynAddSubFX - a software synthesizer

  Part.cpp - Part implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "Part.h"
#include "Microtonal.h"
#include "Util.h"
#include "XMLwrapper.h"
#include "Allocator.h"
#include "../Effects/EffectMgr.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../Synth/Resonance.h"
#include "../Synth/SynthNote.h"
#include "../Synth/ADnote.h"
#include "../Synth/SUBnote.h"
#include "../Synth/PADnote.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Util.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <iostream>

using rtosc::Ports;
using rtosc::RtData;

#define rObject Part
static const Ports partPorts = {
    rRecurs(kit, 16, "Kit"),//NUM_KIT_ITEMS
    rRecursp(partefx, 3, "Part Effect"),
    rRecur(ctl,       "Controller"),
    rToggle(Penabled, "Part enable"),
#undef rChangeCb
#define rChangeCb obj->setPvolume(obj->Pvolume);
    rParamZyn(Pvolume, "Part Volume"),
#undef rChangeCb
#define rChangeCb obj->setPpanning(obj->Ppanning);
    rParamZyn(Ppanning, "Set Panning"),
#undef rChangeCb
#define rChangeCb obj->setkeylimit(obj->Pkeylimit);
    rParamI(Pkeylimit, rProp(parameter), rMap(min,0), rMap(max, POLYPHONY), "Key limit per part"),
#undef rChangeCb
#define rChangeCb
    rParamZyn(Pminkey, "Min Used Key"),
    rParamZyn(Pmaxkey, "Max Used Key"),
    rParamZyn(Pkeyshift, "Part keyshift"),
    rParamZyn(Prcvchn,  "Active MIDI channel"),
    rParamZyn(Pvelsns,   "Velocity sensing"),
    rParamZyn(Pveloffs,  "Velocity offset"),
    rToggle(Pnoteon,  "If the channel accepts note on events"),
    rOption(Pkitmode, rOptions(Off, Multi-Kit, Single-Kit), "Kit mode/enable\n"
            "Off        - Only the first kit is ever utilized\n"
            "Multi-kit  - Every applicable kit is run for a note\n"
            "Single-kit - The first applicable kit is run for a given note"),
    rToggle(Pdrummode, "Drum mode enable\n"
            "When drum mode is enabled all keys are mapped to 12tET and legato is disabled"),
    rToggle(Ppolymode,  "Polyphony mode"),
    rToggle(Plegatomode, "Legato mode"),
    rParamZyn(info.Ptype, "Class of Instrument"),
    rString(info.Pauthor, MAX_INFO_TEXT_SIZE, "Instrument author"),
    rString(info.Pcomments, MAX_INFO_TEXT_SIZE, "Instrument comments"),
    rString(Pname, PART_MAX_NAME_LEN, "User specified label"),
    rArray(Pefxroute, NUM_PART_EFX,  "Effect Routing"),
    rArrayT(Pefxbypass, NUM_PART_EFX, "If an effect is bypassed"),
    {"captureMin:", rDoc("Capture minimum valid note"), NULL,
        [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pminkey = p->lastnote;}},
    {"captureMax:", rDoc("Capture maximum valid note"), NULL,
        [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pmaxkey = p->lastnote;}},
    {"polyType::c:i", rProp(parameter) rOptions(Polyphonic, Monophonic, Legato)
        rDoc("Synthesis polyphony type\n"
                "Polyphonic - Each note is played independently\n"
                "Monophonic - A single note is played at a time with"
                " envelopes resetting between notes\n"
                "Legato     - A single note is played at a time without"
                " envelopes resetting between notes\n"
            ), NULL,
        [](const char *msg, RtData &d)
        {
            Part *p = (Part*)d.obj;
            if(!rtosc_narguments(msg)) {
                int res = 0;
                if(!p->Ppolymode)
                    res = p->Plegatomode ? 2 : 1;
                d.reply(d.loc, "c", res);
                return;
            }

            int i = rtosc_argument(msg, 0).i;
            if(i == 0) {
                p->Ppolymode = 1;
                p->Plegatomode = 0;
            } else if(i==1) {
                p->Ppolymode = 0;
                p->Plegatomode = 0;
            } else {
                p->Ppolymode = 0;
                p->Plegatomode = 1;
            }}},
    {"clear:", rProp(internal) rDoc("Reset Part To Defaults"), 0, 
        [](const char *, RtData &d)
        {
            //XXX todo forward this event for middleware to handle
            //Part *p = (Part*)d.obj;
            //p->defaults();
            //char part_loc[128];
            //strcpy(part_loc, d.loc);
            //char *end = strrchr(part_loc, '/');
            //if(end)
            //    end[1] = 0;

            //d.broadcast("/damage", "s", part_loc);
        }},


    //{"kit#16::T:F", "::Enables or disables kit item", 0,
    //    [](const char *m, RtData &d) {
    //        auto loc = d.loc;
    //        Part *p = (Part*)d.obj;
    //        unsigned kitid = -1;
    //        //Note that this event will be captured before transmitted, thus
    //        //reply/broadcast don't matter
    //        for(int i=0; i<NUM_KIT_ITEMS; ++i) {
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i,
    //                    "oscil", sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->OscilSmp);
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i, "oscil-mod"
    //                    sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->somethingelse);
    //        }
    //        d.reply("/middleware/pad", "sib", loc, kitid,
    //                sizeof(PADnoteParameters*),
    //                p->kit[kitid]->padpars)
    //    }}
};

#undef  rObject
#define rObject Part::Kit
static const Ports kitPorts = {
    rRecurp(padpars, "Padnote parameters"),
    rRecurp(adpars, "Adnote parameters"),
    rRecurp(subpars, "Adnote parameters"),
    rToggle(Penabled, "Kit item enable"),
    rToggle(Pmuted,   "Kit item mute"),
    rParamZyn(Pminkey,   "Kit item min key"),
    rParamZyn(Pmaxkey,   "Kit item max key"),
    rToggle(Padenabled, "ADsynth enable"),
    rToggle(Psubenabled, "SUBsynth enable"),
    rToggle(Ppadenabled, "PADsynth enable"),
    rParamZyn(Psendtoparteffect, "Effect Levels"),
    rString(Pname, PART_MAX_NAME_LEN, "Kit User Specified Label"),
    {"captureMin:", rDoc("Capture minimum valid note"), NULL,
        [](const char *, RtData &r)
        {Part::Kit *p = (Part::Kit*)r.obj; p->Pminkey = p->parent->lastnote;}},
    {"captureMax:", rDoc("Capture maximum valid note"), NULL, [](const char *, RtData &r)
        {Part::Kit *p = (Part::Kit*)r.obj; p->Pmaxkey = p->parent->lastnote;}},
    {"padpars-data:b", rProp(internal) rDoc("Set PADsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.padpars == NULL);
            o.padpars = *(decltype(o.padpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"adpars-data:b", rProp(internal) rDoc("Set ADsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.adpars == NULL);
            o.adpars = *(decltype(o.adpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"subpars-data:b", rProp(internal) rDoc("Set SUBsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.subpars == NULL);
            o.subpars = *(decltype(o.subpars)*)rtosc_argument(msg, 0).b.data;
        }},
};

const Ports &Part::Kit::ports = kitPorts;
const Ports &Part::ports = partPorts;

Part::Part(Allocator &alloc, const SYNTH_T &synth_, const AbsTime &time_,
    const int &gzip_compression, const int &interpolation,
    Microtonal *microtonal_, FFTwrapper *fft_)
    :Pdrummode(false),
    Ppolymode(true),
    Plegatomode(false),
    partoutl(new float[synth_.buffersize]),
    partoutr(new float[synth_.buffersize]),
    ctl(synth_, &time_),
    microtonal(microtonal_),
    fft(fft_),
    memory(alloc),
    synth(synth_),
    time(time_),
    gzip_compression(gzip_compression),
    interpolation(interpolation)
{
    monomemClear();

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        kit[n].parent  = this;
        kit[n].Pname   = new char [PART_MAX_NAME_LEN];
        kit[n].adpars  = nullptr;
        kit[n].subpars = nullptr;
        kit[n].padpars = nullptr;
    }

    kit[0].adpars  = new ADnoteParameters(synth, fft, &time);

    //Part's Insertion Effects init
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]    = new EffectMgr(memory, synth, 1, &time);
        Pefxbypass[nefx] = false;
    }
    assert(partefx[0]);

    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        partfxinputl[n] = new float [synth.buffersize];
        partfxinputr[n] = new float [synth.buffersize];
    }

    killallnotes = false;
    oldfreq      = -1.0f;

    cleanup();

    Pname = new char[PART_MAX_NAME_LEN];

    oldvolumel = oldvolumer = 0.5f;
    lastnote   = -1;

    defaults();
    assert(partefx[0]);
}

void Part::cloneTraits(Part &p) const
{
#define CLONE(x) p.x = this->x
    CLONE(Penabled);

    p.setPvolume(this->Pvolume);
    p.setPpanning(this->Ppanning);

    CLONE(Pminkey);
    CLONE(Pmaxkey);
    CLONE(Pkeyshift);
    CLONE(Prcvchn);

    CLONE(Pvelsns);
    CLONE(Pveloffs);

    CLONE(Pnoteon);
    CLONE(Ppolymode);
    CLONE(Plegatomode);
    CLONE(Pkeylimit);

    CLONE(ctl);
}

void Part::defaults()
{
    Penabled    = 0;
    Pminkey     = 0;
    Pmaxkey     = 127;
    Pnoteon     = 1;
    Ppolymode   = 1;
    Plegatomode = 0;
    setPvolume(96);
    Pkeyshift = 64;
    Prcvchn   = 0;
    setPpanning(64);
    Pvelsns   = 64;
    Pveloffs  = 64;
    Pkeylimit = 15;
    defaultsinstrument();
    ctl.defaults();
}

void Part::defaultsinstrument()
{
    ZERO(Pname, PART_MAX_NAME_LEN);

    info.Ptype = 0;
    ZERO(info.Pauthor, MAX_INFO_TEXT_SIZE + 1);
    ZERO(info.Pcomments, MAX_INFO_TEXT_SIZE + 1);

    Pkitmode  = 0;
    Pdrummode = 0;

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        //kit[n].Penabled    = false;
        kit[n].Pmuted      = false;
        kit[n].Pminkey     = 0;
        kit[n].Pmaxkey     = 127;
        kit[n].Padenabled  = false;
        kit[n].Psubenabled = false;
        kit[n].Ppadenabled = false;
        ZERO(kit[n].Pname, PART_MAX_NAME_LEN);
        kit[n].Psendtoparteffect = 0;
        if(n != 0)
            setkititemstatus(n, 0);
    }
    kit[0].Penabled   = 1;
    kit[0].Padenabled = 1;
    kit[0].adpars->defaults();

    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]->defaults();
        Pefxroute[nefx] = 0; //route to next effect
    }
}



/*
 * Cleanup the part
 */
void Part::cleanup(bool final_)
{
    notePool.killAllNotes();
    for(int i = 0; i < synth.buffersize; ++i) {
        partoutl[i] = final_ ? 0.0f : synth.denormalkillbuf[i];
        partoutr[i] = final_ ? 0.0f : synth.denormalkillbuf[i];
    }
    ctl.resetall();
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        partefx[nefx]->cleanup();
    for(int n = 0; n < NUM_PART_EFX + 1; ++n)
        for(int i = 0; i < synth.buffersize; ++i) {
            partfxinputl[n][i] = final_ ? 0.0f : synth.denormalkillbuf[i];
            partfxinputr[n][i] = final_ ? 0.0f : synth.denormalkillbuf[i];
        }
}

Part::~Part()
{
    cleanup(true);
    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        delete kit[n].adpars;
        delete kit[n].subpars;
        delete kit[n].padpars;
        delete [] kit[n].Pname;
    }

    delete [] Pname;
    delete [] partoutl;
    delete [] partoutr;
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        delete partefx[nefx];
    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        delete [] partfxinputl[n];
        delete [] partfxinputr[n];
    }
}

static void assert_kit_sanity(const Part::Kit *kits)
{
    for(int i=0; i<NUM_KIT_ITEMS; ++i) {
        //an enabled kit must have a corresponding parameter object
        assert(!kits[i].Padenabled  || kits[i].adpars);
        assert(!kits[i].Ppadenabled || kits[i].padpars);
        assert(!kits[i].Psubenabled || kits[i].subpars);
    }
}

static int kit_usage(const Part::Kit *kits, int note, int mode)
{
    const bool non_kit   = mode == 0;
    const bool singl_kit = mode == 2;
    int synth_usage = 0;

    for(uint8_t i = 0; i < NUM_KIT_ITEMS; ++i) {
        const auto &item = kits[i];
        if(!non_kit && !item.validNote(note))
            continue;

        synth_usage += item.Padenabled;
        synth_usage += item.Psubenabled;
        synth_usage += item.Ppadenabled;

        //Partial Kit Use
        if(non_kit || (singl_kit && item.active()))
            break;
    }

    return synth_usage;
}

/*
 * Note On Messages
 */
bool Part::NoteOn(unsigned char note,
                  unsigned char velocity,
                  int masterkeyshift)
{
    //Verify Basic Mode and sanity
    const bool isRunningNote   = notePool.existsRunningNote();
    const bool doingLegato     = isRunningNote && isLegatoMode() &&
                                 lastlegatomodevalid;

    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || notePool.full() ||
            notePool.synthFull(kit_usage(kit, note, Pkitmode)))
        return false;

    verifyKeyMode();
    assert_kit_sanity(kit);

    //Preserve Note Stack
    if(isMonoMode() || isLegatoMode()) {
        monomemPush(note);
        monomem[note].velocity  = velocity;
        monomem[note].mkeyshift = masterkeyshift;

    } else if(!monomemEmpty())
        monomemClear();

    //Mono/Legato Release old notes
    if(isMonoMode() || (isLegatoMode() && !doingLegato))
        notePool.releasePlayingNotes();

    lastlegatomodevalid = isLegatoMode();

    //Compute Note Parameters
    const float vel          = getVelocity(velocity, Pvelsns, Pveloffs);
    const int   partkeyshift = (int)Pkeyshift - 64;
    const int   keyshift     = masterkeyshift + partkeyshift;
    const float notebasefreq = getBaseFreq(note, keyshift);

    if(notebasefreq < 0)
        return false;

    //Portamento
    lastnote = note;
    if(oldfreq < 1.0f)
        oldfreq = notebasefreq;//this is only the first note is played

    // For Mono/Legato: Force Portamento Off on first
    // notes. That means it is required that the previous note is
    // still held down or sustained for the Portamento to activate
    // (that's like Legato).
    bool portamento = false;
    if(Ppolymode || isRunningNote)
        portamento = ctl.initportamento(oldfreq, notebasefreq, doingLegato);

    oldfreq = notebasefreq;

    //Adjust Existing Notes
    if(doingLegato) {
        LegatoParams pars = {notebasefreq, vel, portamento, note, true};
        notePool.applyLegato(pars);
        return true;
    }

    if(Ppolymode)
        notePool.makeUnsustainable(note);

    //Create New Notes
    for(uint8_t i = 0; i < NUM_KIT_ITEMS; ++i) {
        auto &item = kit[i];
        if(Pkitmode != 0 && !item.validNote(note))
            continue;

        SynthParams pars{memory, ctl, synth, time, notebasefreq, vel,
            portamento, note, false};
        const int sendto = Pkitmode ? item.sendto() : 0;

        try {
            if(item.Padenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<ADnote>(kit[i].adpars, pars), 0, i});
            if(item.Psubenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<SUBnote>(kit[i].subpars, pars), 1, i});
            if(item.Ppadenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<PADnote>(kit[i].padpars, pars, interpolation), 2, i});
        } catch (std::bad_alloc & ba) {
            std::cerr << "dropped new note: " << ba.what() << std::endl;
        }

        //Partial Kit Use
        if(isNonKit() || (isSingleKit() && item.active()))
            break;
    }

    if(isLegatoMode())
        notePool.upgradeToLegato();

    //Enforce the key limit
    setkeylimit(Pkeylimit);
    return true;
}

/*
 * Note Off Messages
 */
void Part::NoteOff(unsigned char note) //release the key
{
    // This note is released, so we remove it from the list.
    if(!monomemEmpty())
        monomemPop(note);

    for(auto &desc:notePool.activeDesc()) {
        if(desc.note != note || !desc.playing())
            continue;
        if(!ctl.sustain.sustain) { //the sustain pedal is not pushed
            if((isMonoMode() || isLegatoMode()) && !monomemEmpty())
                MonoMemRenote();//Play most recent still active note
            else 
                notePool.release(desc);
        }
        else {   //the sustain pedal is pushed
            if(desc.canSustain())
                desc.doSustain();
            else
                notePool.release(desc);
        }
    }
}

void Part::PolyphonicAftertouch(unsigned char note,
                                unsigned char velocity,
                                int masterkeyshift)
{
    (void) masterkeyshift;

    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || Pdrummode)
        return;

    // MonoMem stuff:
    if(!Ppolymode)   // if Poly is off
        monomem[note].velocity = velocity;       // Store this note's velocity.

    const float vel = getVelocity(velocity, Pvelsns, Pveloffs);
    for(auto &d:notePool.activeDesc()) {
        if(d.note == note && d.playing())
            for(auto &s:notePool.activeNotes(d))
                s.note->setVelocity(vel);
    }
}

/*
 * Controllers
 */
void Part::SetController(unsigned int type, int par)
{
    switch(type) {
        case C_pitchwheel:
            ctl.setpitchwheel(par);
            break;
        case C_expression:
            ctl.setexpression(par);
            setPvolume(Pvolume); //update the volume
            break;
        case C_portamento:
            ctl.setportamento(par);
            break;
        case C_panning:
            ctl.setpanning(par);
            setPpanning(Ppanning); //update the panning
            break;
        case C_filtercutoff:
            ctl.setfiltercutoff(par);
            break;
        case C_filterq:
            ctl.setfilterq(par);
            break;
        case C_bandwidth:
            ctl.setbandwidth(par);
            break;
        case C_modwheel:
            ctl.setmodwheel(par);
            break;
        case C_fmamp:
            ctl.setfmamp(par);
            break;
        case C_volume:
            ctl.setvolume(par);
            if(ctl.volume.receive != 0)
                volume = ctl.volume.volume;
            else
                setPvolume(Pvolume);
            break;
        case C_sustain:
            ctl.setsustain(par);
            if(ctl.sustain.sustain == 0)
                ReleaseSustainedKeys();
            break;
        case C_allsoundsoff:
            AllNotesOff(); //Panic
            break;
        case C_resetallcontrollers:
            ctl.resetall();
            ReleaseSustainedKeys();
            if(ctl.volume.receive != 0)
                volume = ctl.volume.volume;
            else
                setPvolume(Pvolume);
            setPvolume(Pvolume); //update the volume
            setPpanning(Ppanning); //update the panning

            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center, 1.0f);

                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_bandwidth, 1.0f);
            }
            //more update to add here if I add controllers
            break;
        case C_allnotesoff:
            ReleaseAllKeys();
            break;
        case C_resonance_center:
            ctl.setresonancecenter(par);
            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center,
                               ctl.resonancecenter.relcenter);
            }
            break;
        case C_resonance_bandwidth:
            ctl.setresonancebw(par);
            kit[0].adpars->GlobalPar.Reson->
            sendcontroller(C_resonance_bandwidth, ctl.resonancebandwidth.relbw);
            break;
    }
}
/*
 * Release the sustained keys
 */

void Part::ReleaseSustainedKeys()
{
    // Let's call MonoMemRenote() on some conditions:
    if((isMonoMode() || isLegatoMode()) && !monomemEmpty())
        if(monomemBack() != lastnote) // Sustain controller manipulation would cause repeated same note respawn without this check.
            MonoMemRenote();  // To play most recent still held note.

    for(auto &d:notePool.activeDesc())
        if(d.sustained())
            for(auto &s:notePool.activeNotes(d))
                s.note->releasekey();
}

/*
 * Release all keys
 */

void Part::ReleaseAllKeys()
{
    for(auto &d:notePool.activeDesc())
        if(!d.released())
            for(auto &s:notePool.activeNotes(d))
                s.note->releasekey();
}

// Call NoteOn(...) with the most recent still held key as new note
// (Made for Mono/Legato).
void Part::MonoMemRenote()
{
    unsigned char mmrtempnote = monomemBack(); // Last list element.
    monomemPop(mmrtempnote); // We remove it, will be added again in NoteOn(...).
    NoteOn(mmrtempnote, monomem[mmrtempnote].velocity,
            monomem[mmrtempnote].mkeyshift);
}

float Part::getBaseFreq(int note, int keyshift) const
{
    if(Pdrummode)
        return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
    else
        return microtonal->getnotefreq(note, keyshift);
}

float Part::getVelocity(uint8_t velocity, uint8_t velocity_sense,
        uint8_t velocity_offset) const
{
    //compute sense function
    const float vel = VelF(velocity / 127.0f, velocity_sense);

    //compute the velocity offset
    return limit(vel + (velocity_offset - 64.0f) / 64.0f, 0.0f, 1.0f);
}

void Part::verifyKeyMode(void)
{
    if(Plegatomode && !Pdrummode && Ppolymode) {
            fprintf(stderr,
                "WARNING: Poly & Legato modes are On, that shouldn't happen\n"
                "Disabling Legato mode...\n"
                "(Part.cpp::NoteOn(..))\n");
            Plegatomode = 0;
    }
}


/*
 * Set Part's key limit
 */
void Part::setkeylimit(unsigned char Pkeylimit_)
{
    Pkeylimit = Pkeylimit_;
    int keylimit = Pkeylimit;
    if(keylimit == 0)
        keylimit = POLYPHONY - 5;

    if(notePool.getRunningNotes() >= keylimit)
        notePool.enforceKeyLimit(keylimit);
}


/*
 * Prepare all notes to be turned off
 */
void Part::AllNotesOff()
{
    killallnotes = true;
}

/*
 * Compute Part samples and store them in the partoutl[] and partoutr[]
 */
void Part::ComputePartSmps()
{
    assert(partefx[0]);
    for(unsigned nefx = 0; nefx < NUM_PART_EFX + 1; ++nefx) {
        memset(partfxinputl[nefx], 0, synth.bufferbytes);
        memset(partfxinputr[nefx], 0, synth.bufferbytes);
    }

    for(auto &d:notePool.activeDesc()) {
        d.age++;
        for(auto &s:notePool.activeNotes(d)) {
            float tmpoutr[synth.buffersize];
            float tmpoutl[synth.buffersize];
            auto &note = *s.note;
            note.noteout(&tmpoutl[0], &tmpoutr[0]);

            for(int i = 0; i < synth.buffersize; ++i) { //add the note to part(mix)
                partfxinputl[d.sendto][i] += tmpoutl[i];
                partfxinputr[d.sendto][i] += tmpoutr[i];
            }

            if(note.finished())
                notePool.kill(s);
        }
    }

    //Apply part's effects and mix them
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        if(!Pefxbypass[nefx]) {
            partefx[nefx]->out(partfxinputl[nefx], partfxinputr[nefx]);
            if(Pefxroute[nefx] == 2)
                for(int i = 0; i < synth.buffersize; ++i) {
                    partfxinputl[nefx + 1][i] += partefx[nefx]->efxoutl[i];
                    partfxinputr[nefx + 1][i] += partefx[nefx]->efxoutr[i];
                }
        }
        int routeto = ((Pefxroute[nefx] == 0) ? nefx + 1 : NUM_PART_EFX);
        for(int i = 0; i < synth.buffersize; ++i) {
            partfxinputl[routeto][i] += partfxinputl[nefx][i];
            partfxinputr[routeto][i] += partfxinputr[nefx][i];
        }
    }
    for(int i = 0; i < synth.buffersize; ++i) {
        partoutl[i] = partfxinputl[NUM_PART_EFX][i];
        partoutr[i] = partfxinputr[NUM_PART_EFX][i];
    }

    if(killallnotes) {
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmp = (synth.buffersize_f - i) / synth.buffersize_f;
            partoutl[i] *= tmp;
            partoutr[i] *= tmp;
        }
        notePool.killAllNotes();
        monomemClear();
        killallnotes = false;
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
            partefx[nefx]->cleanup();
    }
    ctl.updateportamento();
}

/*
 * Parameter control
 */
void Part::setPvolume(char Pvolume_)
{
    Pvolume = Pvolume_;
    volume  =
        dB2rap((Pvolume - 96.0f) / 96.0f * 40.0f) * ctl.expression.relvolume;
}

void Part::setPpanning(char Ppanning_)
{
    Ppanning = Ppanning_;
    panning  = limit(Ppanning / 127.0f + ctl.panning.pan, 0.0f, 1.0f);
}

/*
 * Enable or disable a kit item
 */
void Part::setkititemstatus(unsigned kititem, bool Penabled_)
{
    //nonexistent kit item and the first kit item is always enabled
    if((kititem == 0) || (kititem >= NUM_KIT_ITEMS))
        return;

    Kit &kkit = kit[kititem];

    //no need to update if
    if(kkit.Penabled == Penabled_)
        return;
    kkit.Penabled = Penabled_;

    if(!Penabled_) {
        delete kkit.adpars;
        delete kkit.subpars;
        delete kkit.padpars;
        kkit.adpars  = nullptr;
        kkit.subpars = nullptr;
        kkit.padpars = nullptr;
        kkit.Pname[0] = '\0';

        notePool.killAllNotes();
    }
    else {
        //All parameters must be NULL in this case
        assert(!(kkit.adpars || kkit.subpars || kkit.padpars));
        kkit.adpars  = new ADnoteParameters(synth, fft, &time);
        kkit.subpars = new SUBnoteParameters(&time);
        kkit.padpars = new PADnoteParameters(synth, fft, &time);
    }
}

void Part::add2XMLinstrument(XMLwrapper& xml)
{
    xml.beginbranch("INFO");
    xml.addparstr("name", (char *)Pname);
    xml.addparstr("author", (char *)info.Pauthor);
    xml.addparstr("comments", (char *)info.Pcomments);
    xml.addpar("type", info.Ptype);
    xml.endbranch();


    xml.beginbranch("INSTRUMENT_KIT");
    xml.addpar("kit_mode", Pkitmode);
    xml.addparbool("drum_mode", Pdrummode);

    for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
        xml.beginbranch("INSTRUMENT_KIT_ITEM", i);
        xml.addparbool("enabled", kit[i].Penabled);
        if(kit[i].Penabled != 0) {
            xml.addparstr("name", (char *)kit[i].Pname);

            xml.addparbool("muted", kit[i].Pmuted);
            xml.addpar("min_key", kit[i].Pminkey);
            xml.addpar("max_key", kit[i].Pmaxkey);

            xml.addpar("send_to_instrument_effect", kit[i].Psendtoparteffect);

            xml.addparbool("add_enabled", kit[i].Padenabled);
            if(kit[i].Padenabled && kit[i].adpars) {
                xml.beginbranch("ADD_SYNTH_PARAMETERS");
                kit[i].adpars->add2XML(xml);
                xml.endbranch();
            }

            xml.addparbool("sub_enabled", kit[i].Psubenabled);
            if(kit[i].Psubenabled && kit[i].subpars) {
                xml.beginbranch("SUB_SYNTH_PARAMETERS");
                kit[i].subpars->add2XML(xml);
                xml.endbranch();
            }

            xml.addparbool("pad_enabled", kit[i].Ppadenabled);
            if(kit[i].Ppadenabled && kit[i].padpars) {
                xml.beginbranch("PAD_SYNTH_PARAMETERS");
                kit[i].padpars->add2XML(xml);
                xml.endbranch();
            }
        }
        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("INSTRUMENT_EFFECTS");
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        xml.beginbranch("INSTRUMENT_EFFECT", nefx);
        xml.beginbranch("EFFECT");
        partefx[nefx]->add2XML(xml);
        xml.endbranch();

        xml.addpar("route", Pefxroute[nefx]);
        partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
        xml.addparbool("bypass", Pefxbypass[nefx]);
        xml.endbranch();
    }
    xml.endbranch();
}

void Part::add2XML(XMLwrapper& xml)
{
    //parameters
    xml.addparbool("enabled", Penabled);
    if((Penabled == 0) && (xml.minimal))
        return;

    xml.addpar("volume", Pvolume);
    xml.addpar("panning", Ppanning);

    xml.addpar("min_key", Pminkey);
    xml.addpar("max_key", Pmaxkey);
    xml.addpar("key_shift", Pkeyshift);
    xml.addpar("rcv_chn", Prcvchn);

    xml.addpar("velocity_sensing", Pvelsns);
    xml.addpar("velocity_offset", Pveloffs);

    xml.addparbool("note_on", Pnoteon);
    xml.addparbool("poly_mode", Ppolymode);
    xml.addpar("legato_mode", Plegatomode);
    xml.addpar("key_limit", Pkeylimit);

    xml.beginbranch("INSTRUMENT");
    add2XMLinstrument(xml);
    xml.endbranch();

    xml.beginbranch("CONTROLLER");
    ctl.add2XML(xml);
    xml.endbranch();
}

int Part::saveXML(const char *filename)
{
    XMLwrapper xml;

    xml.beginbranch("INSTRUMENT");
    add2XMLinstrument(xml);
    xml.endbranch();

    int result = xml.saveXMLfile(filename, gzip_compression);
    return result;
}

int Part::loadXMLinstrument(const char *filename)
{
    XMLwrapper xml;
    if(xml.loadXMLfile(filename) < 0) {
        return -1;
    }

    if(xml.enterbranch("INSTRUMENT") == 0)
        return -10;
    getfromXMLinstrument(xml);
    xml.exitbranch();

    return 0;
}

void Part::applyparameters(void)
{
    applyparameters([]{return false;});
}

void Part::applyparameters(std::function<bool()> do_abort)
{
    for(int n = 0; n < NUM_KIT_ITEMS; ++n)
        if(kit[n].Ppadenabled && kit[n].padpars)
            kit[n].padpars->applyparameters(do_abort);
}

void Part::initialize_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->init();
}

void Part::kill_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->kill();
    notePool.killAllNotes();
}

void Part::monomemPush(char note)
{
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            return;

    for(int i=254;i>=0; --i)
        monomemnotes[i+1] = monomemnotes[i];
    monomemnotes[0] = note;
}

void Part::monomemPop(char note)
{
    int note_pos=-1;
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            note_pos = i;
    if(note_pos != -1) {
        for(int i=note_pos; i<256; ++i)
            monomemnotes[i] = monomemnotes[i+1];
        monomemnotes[255] = -1;
    }
}

char Part::monomemBack(void) const
{
    return monomemnotes[0];
}

bool Part::monomemEmpty(void) const
{
    return monomemnotes[0] == -1;
}

void Part::monomemClear(void)
{
    for(int i=0; i<256; ++i)
        monomemnotes[i] = -1;
}

void Part::getfromXMLinstrument(XMLwrapper& xml)
{
    if(xml.enterbranch("INFO")) {
        xml.getparstr("name", (char *)Pname, PART_MAX_NAME_LEN);
        xml.getparstr("author", (char *)info.Pauthor, MAX_INFO_TEXT_SIZE);
        xml.getparstr("comments", (char *)info.Pcomments, MAX_INFO_TEXT_SIZE);
        info.Ptype = xml.getpar("type", info.Ptype, 0, 16);

        xml.exitbranch();
    }

    if(xml.enterbranch("INSTRUMENT_KIT")) {
        Pkitmode  = xml.getpar127("kit_mode", Pkitmode);
        Pdrummode = xml.getparbool("drum_mode", Pdrummode);

        setkititemstatus(0, 0);
        for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
            if(xml.enterbranch("INSTRUMENT_KIT_ITEM", i) == 0)
                continue;
            setkititemstatus(i, xml.getparbool("enabled", kit[i].Penabled));
            if(kit[i].Penabled == 0) {
                xml.exitbranch();
                continue;
            }

            xml.getparstr("name", (char *)kit[i].Pname, PART_MAX_NAME_LEN);

            kit[i].Pmuted  = xml.getparbool("muted", kit[i].Pmuted);
            kit[i].Pminkey = xml.getpar127("min_key", kit[i].Pminkey);
            kit[i].Pmaxkey = xml.getpar127("max_key", kit[i].Pmaxkey);

            kit[i].Psendtoparteffect = xml.getpar127(
                "send_to_instrument_effect",
                kit[i].Psendtoparteffect);

            kit[i].Padenabled = xml.getparbool("add_enabled",
                                                kit[i].Padenabled);
            if(xml.enterbranch("ADD_SYNTH_PARAMETERS")) {
                if(!kit[i].adpars)
                    kit[i].adpars = new ADnoteParameters(synth, fft, &time);
                kit[i].adpars->getfromXML(xml);
                xml.exitbranch();
            }

            kit[i].Psubenabled = xml.getparbool("sub_enabled",
                                                 kit[i].Psubenabled);
            if(xml.enterbranch("SUB_SYNTH_PARAMETERS")) {
                if(!kit[i].subpars)
                    kit[i].subpars = new SUBnoteParameters(&time);
                kit[i].subpars->getfromXML(xml);
                xml.exitbranch();
            }

            kit[i].Ppadenabled = xml.getparbool("pad_enabled",
                                                 kit[i].Ppadenabled);
            if(xml.enterbranch("PAD_SYNTH_PARAMETERS")) {
                if(!kit[i].padpars)
                    kit[i].padpars = new PADnoteParameters(synth, fft, &time);
                kit[i].padpars->getfromXML(xml);
                xml.exitbranch();
            }

            xml.exitbranch();
        }

        xml.exitbranch();
    }


    if(xml.enterbranch("INSTRUMENT_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
            if(xml.enterbranch("INSTRUMENT_EFFECT", nefx) == 0)
                continue;
            if(xml.enterbranch("EFFECT")) {
                partefx[nefx]->getfromXML(xml);
                xml.exitbranch();
            }

            Pefxroute[nefx] = xml.getpar("route",
                                          Pefxroute[nefx],
                                          0,
                                          NUM_PART_EFX);
            partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
            Pefxbypass[nefx] = xml.getparbool("bypass", Pefxbypass[nefx]);
            xml.exitbranch();
        }
        xml.exitbranch();
    }
}

void Part::getfromXML(XMLwrapper& xml)
{
    Penabled = xml.getparbool("enabled", Penabled);

    setPvolume(xml.getpar127("volume", Pvolume));
    setPpanning(xml.getpar127("panning", Ppanning));

    Pminkey   = xml.getpar127("min_key", Pminkey);
    Pmaxkey   = xml.getpar127("max_key", Pmaxkey);
    Pkeyshift = xml.getpar127("key_shift", Pkeyshift);
    Prcvchn   = xml.getpar127("rcv_chn", Prcvchn);

    Pvelsns  = xml.getpar127("velocity_sensing", Pvelsns);
    Pveloffs = xml.getpar127("velocity_offset", Pveloffs);

    Pnoteon     = xml.getparbool("note_on", Pnoteon);
    Ppolymode   = xml.getparbool("poly_mode", Ppolymode);
    Plegatomode = xml.getparbool("legato_mode", Plegatomode); //older versions
    if(!Plegatomode)
        Plegatomode = xml.getpar127("legato_mode", Plegatomode);
    Pkeylimit = xml.getpar127("key_limit", Pkeylimit);


    if(xml.enterbranch("INSTRUMENT")) {
        getfromXMLinstrument(xml);
        xml.exitbranch();
    }

    if(xml.enterbranch("CONTROLLER")) {
        ctl.getfromXML(xml);
        xml.exitbranch();
    }
}

bool Part::Kit::active(void) const
{
    return Padenabled || Psubenabled || Ppadenabled;
}

uint8_t Part::Kit::sendto(void) const
{
    return limit((int)Psendtoparteffect, 0, NUM_PART_EFX);

}

bool Part::Kit::validNote(char note) const
{
    return !Pmuted && inRange((uint8_t)note, Pminkey, Pmaxkey);
}
