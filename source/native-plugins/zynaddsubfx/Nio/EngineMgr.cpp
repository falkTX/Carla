#include "EngineMgr.h"
#include <algorithm>
#include <iostream>
#include <cassert>
#include "Nio.h"
#include "InMgr.h"
#include "OutMgr.h"
#include "AudioOut.h"
#include "MidiIn.h"
#include "NulEngine.h"
#if OSS
#include "OssEngine.h"
#include "OssMultiEngine.h"
#endif
#if ALSA
#include "AlsaEngine.h"
#endif
#if JACK
#include "JackEngine.h"
#include "JackMultiEngine.h"
#endif
#if PORTAUDIO
#include "PaEngine.h"
#endif

using namespace std;

EngineMgr &EngineMgr::getInstance(const SYNTH_T *synth,
    const oss_devs_t *oss_devs)
{
    static EngineMgr instance(synth, *oss_devs);
    return instance;
}

EngineMgr::EngineMgr(const SYNTH_T *synth, const oss_devs_t& oss_devs)
{
    assert(synth);
    Engine *defaultEng = new NulEngine(*synth);

    //conditional compiling mess (but contained)
    engines.push_back(defaultEng);
#if OSS
    engines.push_back(new OssEngine(*synth, oss_devs));
    engines.push_back(new OssMultiEngine(*synth, oss_devs));
#endif
#if ALSA
    engines.push_back(new AlsaEngine(*synth));
#endif
#if JACK
    engines.push_back(new JackEngine(*synth));
    engines.push_back(new JackMultiEngine(*synth));
#endif
#if PORTAUDIO
    engines.push_back(new PaEngine(*synth));
#endif

    defaultOut = dynamic_cast<AudioOut *>(defaultEng);

    defaultIn = dynamic_cast<MidiIn *>(defaultEng);

    //Accept command line/compile time options
    if(!Nio::defaultSink.empty())
        setOutDefault(Nio::defaultSink);

    if(!Nio::defaultSource.empty())
        setInDefault(Nio::defaultSource);
}

EngineMgr::~EngineMgr()
{
    for(list<Engine *>::iterator itr = engines.begin();
        itr != engines.end(); ++itr)
        delete *itr;
}

Engine *EngineMgr::getEng(string name)
{
    transform(name.begin(), name.end(), name.begin(), ::toupper);
    for(list<Engine *>::iterator itr = engines.begin();
        itr != engines.end(); ++itr)
        if((*itr)->name == name)
            return *itr;
    return NULL;
}

bool EngineMgr::start()
{
    bool expected = true;
    if(!(defaultOut && defaultIn)) {
        cerr << "ERROR: It looks like someone broke the Nio Output\n"
             << "       Attempting to recover by defaulting to the\n"
             << "       Null Engine." << endl;
        defaultOut = dynamic_cast<AudioOut *>(getEng("NULL"));
        defaultIn  = dynamic_cast<MidiIn *>(getEng("NULL"));
    }

    OutMgr::getInstance(). currentOut = defaultOut;
    InMgr::getInstance().  current    = defaultIn;

    //open up the default output(s)
    cout << "Starting Audio: " << defaultOut->name << endl;
    defaultOut->setAudioEn(true);
    if(defaultOut->getAudioEn())
        cout << "Audio Started" << endl;
    else {
        expected = false;
        cerr << "ERROR: The default audio output failed to open!" << endl;
        OutMgr::getInstance(). currentOut =
            dynamic_cast<AudioOut *>(getEng("NULL"));
        OutMgr::getInstance(). currentOut->setAudioEn(true);
    }

    cout << "Starting MIDI: " << defaultIn->name << endl;
    defaultIn->setMidiEn(true);
    if(defaultIn->getMidiEn())
        cout << "MIDI Started" << endl;
    else { //recover
        expected = false;
        cerr << "ERROR: The default MIDI input failed to open!" << endl;
        InMgr::getInstance(). current = dynamic_cast<MidiIn *>(getEng("NULL"));
        InMgr::getInstance(). current->setMidiEn(true);
    }

    //Show if expected drivers were booted
    return expected;
}

void EngineMgr::stop()
{
    for(list<Engine *>::iterator itr = engines.begin();
        itr != engines.end(); ++itr)
        (*itr)->Stop();
}

bool EngineMgr::setInDefault(string name)
{
    MidiIn *chosen;
    if((chosen = dynamic_cast<MidiIn *>(getEng(name)))) {    //got the input
        defaultIn = chosen;
        return true;
    }

    //Warn user
    cerr << "Error: " << name << " is not a recognized MIDI input source"
         << endl;
    cerr << "       Defaulting to the NULL input source" << endl;

    return false;
}

bool EngineMgr::setOutDefault(string name)
{
    AudioOut *chosen;
    if((chosen = dynamic_cast<AudioOut *>(getEng(name)))) {    //got the output
        defaultOut = chosen;
        return true;
    }

    //Warn user
    cerr << "Error: " << name << " is not a recognized audio backend" << endl;
    cerr << "       Defaulting to the NULL audio backend" << endl;
    return false;
}
