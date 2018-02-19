/*
  ZynAddSubFX - a software synthesizer

  WavEngine - an Output To File Engine
  Copyright (C) 2006 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "WavEngine.h"
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "../Misc/WavFile.h"
#include "../Misc/Util.h"
using namespace std;

namespace zyncarla {

WavEngine::WavEngine(const SYNTH_T &synth_)
    :AudioOut(synth_), file(NULL), buffer(synth.samplerate * 4), pThread(NULL)
{
    work.init(PTHREAD_PROCESS_PRIVATE, 0);
}

WavEngine::~WavEngine()
{
    Stop();
    destroyFile();
}

bool WavEngine::openAudio()
{
    return file && file->good();
}

bool WavEngine::Start()
{
    if(pThread)
        return true;
    pThread = new pthread_t;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(pThread, &attr, _AudioThread, this);

    return true;
}

void WavEngine::Stop()
{
    if(!pThread)
        return;

    pthread_t *tmp = pThread;
    pThread = NULL;

    work.post();
    pthread_join(*tmp, NULL);
    delete pThread;
}

void WavEngine::push(Stereo<float *> smps, size_t len)
{
    if(!pThread)
        return;


    //copy the input [overflow when needed]
    for(size_t i = 0; i < len; ++i) {
        buffer.push(*smps.l++);
        buffer.push(*smps.r++);
    }
    work.post();
}

void WavEngine::newFile(WavFile *_file)
{
    //ensure system is clean
    destroyFile();
    file = _file;

    //check state
    if(!file->good())
        cerr
        << "ERROR: WavEngine handed bad file output WavEngine::newFile()"
        << endl;
}

void WavEngine::destroyFile()
{
    if(file)
        delete file;
    file = NULL;
}

void *WavEngine::_AudioThread(void *arg)
{
    return (static_cast<WavEngine *>(arg))->AudioThread();
}

void *WavEngine::AudioThread()
{
    short *recordbuf_16bit = new short[2 * synth.buffersize];

    while(!work.wait() && pThread) {
        for(int i = 0; i < synth.buffersize; ++i) {
            float left = 0.0f, right = 0.0f;
            buffer.pop(left);
            buffer.pop(right);
            recordbuf_16bit[2 * i] = limit((int)(left * 32767.0f),
                                           -32768,
                                           32767);
            recordbuf_16bit[2 * i + 1] = limit((int)(right * 32767.0f),
                                               -32768,
                                               32767);
        }
        if(file)
            file->writeStereoSamples(synth.buffersize, recordbuf_16bit);
    }

    delete[] recordbuf_16bit;

    return NULL;
}

}
