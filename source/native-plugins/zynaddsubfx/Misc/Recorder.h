/*
  ZynAddSubFX - a software synthesizer

  Recorder.h - Records sound to a file
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef RECORDER_H
#define RECORDER_H
#include <string>

struct SYNTH_T;
/**Records sound to a file*/
class Recorder
{
    public:

        Recorder(const SYNTH_T &synth);
        ~Recorder();
        /**Prepare the given file.
         * @returns 1 if the file exists */
        int preparefile(std::string filename_, int overwrite);
        void start();
        void stop();
        void pause();
        int recording();
        void triggernow();

        /** Status:
         *  0 - not ready(no file selected),
         *  1 - ready
         *  2 - recording */
        int status;

    private:
        int notetrigger;
        const SYNTH_T &synth;
};

#endif
