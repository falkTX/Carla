/*
  ZynAddSubFX - a software synthesizer

  Part.h - Part implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PART_H
#define PART_H

#define MAX_INFO_TEXT_SIZE 1000

#include "../globals.h"
#include "../Params/Controller.h"
#include "../Containers/NotePool.h"

#include <functional>

namespace zyncarla {

/** Part implementation*/
class Part
{
    public:
        /**Constructor
         * @param microtonal_ Pointer to the microtonal object
         * @param fft_ Pointer to the FFTwrapper*/
        Part(Allocator &alloc, const SYNTH_T &synth, const AbsTime &time,
             const int& gzip_compression, const int& interpolation,
             Microtonal *microtonal_, FFTwrapper *fft_, WatchManager *wm=0, const char *prefix=0);
        /**Destructor*/
        ~Part();

        // Copy misc parameters not stored in .xiz format
        void cloneTraits(Part &part) const REALTIME;

        // Midi commands implemented

        //returns true when note is successfully applied
        bool NoteOn(unsigned char note,
                    unsigned char velocity,
                    int masterkeyshift) REALTIME;
        void NoteOff(unsigned char note) REALTIME;
        void PolyphonicAftertouch(unsigned char note,
                                  unsigned char velocity,
                                  int masterkeyshift) REALTIME;
        void AllNotesOff() REALTIME; //panic
        void SetController(unsigned int type, int par) REALTIME;
        void ReleaseSustainedKeys() REALTIME; //this is called when the sustain pedal is released
        void ReleaseAllKeys() REALTIME; //this is called on AllNotesOff controller

        /* The synthesizer part output */
        void ComputePartSmps() REALTIME; //Part output


        //saves the instrument settings to a XML file
        //returns 0 for ok or <0 if there is an error
        int saveXML(const char *filename);
        int loadXMLinstrument(const char *filename);

        void add2XML(XMLwrapper& xml);
        void add2XMLinstrument(XMLwrapper& xml);

        void defaults();
        void defaultsinstrument();

        void applyparameters(void) NONREALTIME;
        void applyparameters(std::function<bool()> do_abort) NONREALTIME;

        void initialize_rt(void) REALTIME;
        void kill_rt(void) REALTIME;

        void getfromXML(XMLwrapper& xml);
        void getfromXMLinstrument(XMLwrapper& xml);

        void cleanup(bool final = false);

        //the part's kit
        struct Kit {
            Kit(void);
            Part              *parent;
            bool               firstkit;
            bool               Penabled, Pmuted;
            unsigned char      Pminkey, Pmaxkey;
            char              *Pname;
            bool               Padenabled, Psubenabled, Ppadenabled;
            unsigned char      Psendtoparteffect;
            ADnoteParameters  *adpars;
            SUBnoteParameters *subpars;
            PADnoteParameters *padpars;

            bool    active(void) const;
            uint8_t sendto(void) const;
            bool    validNote(char note) const;

            const static rtosc::Ports &ports;
        } kit[NUM_KIT_ITEMS];


        //Part parameters
        void setkeylimit(unsigned char Pkeylimit);
        void setkititemstatus(unsigned kititem, bool Penabled_);

        unsigned char partno; /**<if it's the Master's first part*/
        bool          Penabled; /**<if the part is enabled*/
        unsigned char Pvolume; /**<part volume*/
        unsigned char Pminkey; /**<the minimum key that the part receives noteon messages*/
        unsigned char Pmaxkey; //the maximum key that the part receives noteon messages
        void setPvolume(char Pvolume);
        unsigned char Pkeyshift; //Part keyshift
        unsigned char Prcvchn; //from what midi channel it receives commands
        unsigned char Ppanning; //part panning
        void setPpanning(char Ppanning);
        unsigned char Pvelsns; //velocity sensing (amplitude velocity scale)
        unsigned char Pveloffs; //velocity offset
        bool Pnoteon; //if the part receives NoteOn messages
        int Pkitmode; //if the kitmode is enabled

        //XXX consider deprecating drum mode
        bool Pdrummode; //if all keys are mapped and the system is 12tET (used for drums)

        bool Ppolymode; //Part mode - 0=monophonic , 1=polyphonic
        bool Plegatomode; // 0=normal, 1=legato
        unsigned char Pkeylimit; //how many keys are alowed to be played same time (0=off), the older will be released

        char *Pname; //name of the instrument
        struct { //instrument additional information
            unsigned char Ptype;
            char          Pauthor[MAX_INFO_TEXT_SIZE + 1];
            char          Pcomments[MAX_INFO_TEXT_SIZE + 1];
        } info;


        float *partoutl; //Left channel output of the part
        float *partoutr; //Right channel output of the part

        float *partfxinputl[NUM_PART_EFX + 1], //Left and right signal that pass thru part effects;
        *partfxinputr[NUM_PART_EFX + 1];          //partfxinput l/r [NUM_PART_EFX] is for "no effect" buffer


        float volume, oldvolumel, oldvolumer; //this is applied by Master
        float panning; //this is applied by Master, too

        Controller ctl; //Part controllers

        EffectMgr    *partefx[NUM_PART_EFX]; //insertion part effects (they are part of the instrument)
        unsigned char Pefxroute[NUM_PART_EFX]; //how the effect's output is routed(to next effect/to out)
        bool Pefxbypass[NUM_PART_EFX]; //if the effects are bypassed

        int lastnote;

        const static rtosc::Ports &ports;

    private:
        void MonoMemRenote(); // MonoMem stuff.
        float getBaseFreq(int note, int keyshift) const;
        float getVelocity(uint8_t velocity, uint8_t velocity_sense,
                uint8_t velocity_offset) const;
        void verifyKeyMode(void);
        bool isPolyMode(void)   const {return Ppolymode;}
        bool isMonoMode(void)   const {return !Ppolymode  && !Plegatomode;};
        bool isLegatoMode(void) const {return Plegatomode && !Pdrummode;}
        bool isNonKit(void)     const {return Pkitmode == 0;}
        bool isMultiKit(void)   const {return Pkitmode == 1;}
        bool isSingleKit(void)  const {return Pkitmode == 2;}

        bool killallnotes;

        NotePool notePool;

        bool lastlegatomodevalid; // To keep track of previous legatomodevalid.

        // MonoMem stuff
        void monomemPush(char note);
        void monomemPop(char note);
        char monomemBack(void) const;
        bool monomemEmpty(void) const;
        void monomemClear(void);

        short monomemnotes[256]; // A list to remember held notes.
        struct {
            unsigned char velocity;
            int mkeyshift; // I'm not sure masterkeyshift should be remembered.
        } monomem[256];
        /* 256 is to cover all possible note values.
           monomem[] is used in conjunction with the list to
           store the velocity and masterkeyshift values of a given note (the list only store note values).
           For example 'monomem[note].velocity' would be the velocity value of the note 'note'.*/

        float oldfreq;    //this is used for portamento
        Microtonal *microtonal;
        FFTwrapper *fft;
        WatchManager *wm;
        char prefix[64];
        Allocator  &memory;
        const SYNTH_T &synth;
        const AbsTime &time;
        const int &gzip_compression, &interpolation;
};

}

#endif
