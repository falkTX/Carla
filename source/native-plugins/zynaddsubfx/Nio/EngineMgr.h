/*
  ZynAddSubFX - a software synthesizer

  EngineMgr.h - MIDI/Audio Factory
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef ENGINE_MGR_H
#define ENGINE_MGR_H

#include <list>
#include <string>
#include "Engine.h"


class MidiIn;
class AudioOut;
class OutMgr;
struct SYNTH_T;
/**Container/Owner of the long lived Engines*/
class EngineMgr
{
    public:
        static EngineMgr &getInstance(
            const SYNTH_T *synth = nullptr,
            const class oss_devs_t* oss_devs = nullptr);
        ~EngineMgr();

        /**Gets requested engine
         * @param name case unsensitive name of engine
         * @return pointer to Engine or NULL
         */
        Engine *getEng(std::string name);

        /**Start up defaults*/
        bool start();

        /**Stop all engines*/
        void stop();

        std::list<Engine *> engines;

        //return false on failure
        bool setInDefault(std::string name);
        bool setOutDefault(std::string name);

        //default I/O
        AudioOut *defaultOut;
        MidiIn   *defaultIn;
    private:
        EngineMgr(const SYNTH_T *synth, const oss_devs_t &oss_devs);
};
#endif
