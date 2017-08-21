/*
  ZynAddSubFX - a software synthesizer

  WavFile.h - Records sound to a file
  Copyright (C) 2008 Nasca Octavian Paul
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef WAVFILE_H
#define WAVFILE_H
#include <string>

namespace zyncarla {

class WavFile
{
    public:
        WavFile(std::string filename, int samplerate, int channels);
        ~WavFile();

        bool good() const;

        void writeMonoSamples(int nsmps, short int *smps);
        void writeStereoSamples(int nsmps, short int *smps);

    private:
        int   sampleswritten;
        int   samplerate;
        int   channels;
        FILE *file;
};

}

#endif
