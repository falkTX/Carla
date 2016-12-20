/*
  ZynAddSubFX - a software synthesizer

  Microtonal.h - Tuning settings and microtonal capabilities
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef MICROTONAL_H
#define MICROTONAL_H

#include <cstdio>
#include <stdint.h>
#include "../globals.h"

#define MAX_OCTAVE_SIZE 128
#define MICROTONAL_MAX_NAME_LEN 120
class XMLwrapper;

struct KbmInfo
{
    uint8_t   Pmapsize;
    uint8_t   Pfirstkey;
    uint8_t   Plastkey;
    uint8_t   Pmiddlenote;
    uint8_t   PAnote;
    float     PAfreq;
    uint8_t   Pmappingenabled;
    short int Pmapping[128];
};

struct OctaveTuning {
    unsigned char type; //1 for cents or 2 for division

    // the real tuning (eg. +1.05946f for one halftone)
    // or 2.0f for one octave
    float tuning;

    //the real tunning is x1/x2
    unsigned int x1, x2;
};

struct SclInfo
{
    char Pname[MICROTONAL_MAX_NAME_LEN];
    char Pcomment[MICROTONAL_MAX_NAME_LEN];
    unsigned char octavesize;
    OctaveTuning octave[MAX_OCTAVE_SIZE];
};

/**Tuning settings and microtonal capabilities*/
class Microtonal
{
    public:
        /**Constructor*/
        Microtonal(const int& gzip_compression);
        /**Destructor*/
        ~Microtonal();
        void defaults();
        /**Calculates the frequency for a given note
         */
        float getnotefreq(int note, int keyshift) const;


        //Parameters
        /**if the keys are inversed (the pitch is lower to keys from the right direction)*/
        unsigned char Pinvertupdown;

        /**the central key of the inversion*/
        unsigned char Pinvertupdowncenter;

        /**0 for 12 key temperate scale, 1 for microtonal*/
        unsigned char Penabled;

        /**the note of "A" key*/
        unsigned char PAnote;

        /**the frequency of the "A" note*/
        float PAfreq;

        /**if the scale is "tuned" to a note, you can tune to other note*/
        unsigned char Pscaleshift;

        //first and last key (to retune)
        unsigned char Pfirstkey;
        unsigned char Plastkey;

        /**The middle note where scale degree 0 is mapped to*/
        unsigned char Pmiddlenote;

        /**Map size*/
        unsigned char Pmapsize;

        /**Mapping ON/OFF*/
        unsigned char Pmappingenabled;
        /**Mapping (keys)*/
        short int Pmapping[128];

        /**Fine detune to be applied to all notes*/
        unsigned char Pglobalfinedetune;

        // Functions
        /** Return the current octave size*/
        unsigned char getoctavesize() const;
        /**Convert tunning to string*/
        void tuningtoline(int n, char *line, int maxn);
        /**load the tunnings from a .scl file*/
        static int loadscl(SclInfo &scl, const char *filename);
        /**load the mapping from .kbm file*/
        static int loadkbm(KbmInfo &kbm, const char *filename);
        /**Load text into the internal tunings
         *
         *\todo better description*/
        int texttotunings(const char *text);
        /**Load text into the internal mappings
         *
         *\todo better description*/
        void texttomapping(const char *text);

        /**Name of Microtonal tuning*/
        char Pname[MICROTONAL_MAX_NAME_LEN];
        /**Comment about the tuning*/
        char Pcomment[MICROTONAL_MAX_NAME_LEN];

        void add2XML(XMLwrapper& xml) const;
        void getfromXML(XMLwrapper& xml);
        int saveXML(const char *filename) const;
        int loadXML(const char *filename);

        //simple operators primarily for debug
        bool operator==(const Microtonal &micro) const;
        bool operator!=(const Microtonal &micro) const;

        void clone(Microtonal &m);

        static const rtosc::Ports ports;

        //only paste handler should access there (quasi-private)
        unsigned char octavesize;
        OctaveTuning octave[MAX_OCTAVE_SIZE];
    private:
        //loads a line from the text file, while ignoring the lines beggining with "!"
        static int loadline(FILE *file, char *line);
        //Grab a 0..127 integer from the provided descriptor

        static int linetotunings(struct OctaveTuning &tune, const char *line);
        void apply(void);

        const int& gzip_compression;
};

#endif
